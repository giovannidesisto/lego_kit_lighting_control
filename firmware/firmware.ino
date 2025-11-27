/*************************************************************
 * Sistema di controllo illuminazione con ESP32
 * 
 * Funzionalità:
 * - Controllo striscia LED NeoPixel (3 segmenti indipendenti)
 * - Controllo lampada dimmerabile via TRIAC
 * - Interfaccia Blynk per controllo remoto
 * - Orologio NTP per automazione oraria
 * - Sensore di luminosità ambientale
 * - Supporto OTA (Over-The-Air updates)
 *************************************************************/

// Configurazione Blynk (Template ID, Device Name e Auth Token)
#define BLYNK_TEMPLATE_ID "xxxx"
#define BLYNK_TEMPLATE_NAME "yyyy"
#define BLYNK_AUTH_TOKEN "zzzz"
#define BLYNK_PRINT Serial  // Abilita debug seriale Blynk

// Inclusione librerie
#include <RBDdimmer.h>       // Controllo dimmer TRIAC
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <NTPClient_Generic.h>   // Client orario NTP
#include <BlynkSimpleEsp32.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>      // Aggiornamenti OTA

// Credenziali WiFi
char ssid[] = "xxxx";
char pass[] = "yyyy";

// Definizione pin
#define NEOPIN      22    // Pin dati NeoPixel
#define PHOTOPIN    34    // Pin fotoresistore (ADC)
#define PWMPIN      17    // Pin controllo dimmer
#define ZCPIN       16    // Pin zero-cross detection

// Inizializzazione oggetti
dimmerLamp dimmer(PWMPIN, ZCPIN);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(50, NEOPIN, NEO_GRB + NEO_KHZ800);

// Variabili globali
int sensorVal;          // Lettura fotoresistore
short r1,g1,b1;         // Colore primo segmento NeoPixel
short r2,g2,b2;         // Colore secondo segmento NeoPixel
short r3,g3,b3;         // Colore terzo segmento NeoPixel
short brightenss = 0;   // Luminosità striscia LED
int brightnessActivationLimit = 4095;  // Soglia attivazione luce ambiente

// Variabili controllo orario
WiFiUDP ntpUDP;
#define TIME_ZONE_OFFSET_HRS (+2)  // Offset orario (Italia +2)
NTPClient timeClient(ntpUDP);
short ntpCounter = 0;              // Contatore aggiornamenti NTP

// Programmazione oraria
short start_h = 19;    // Ora inizio automazione
short start_m = 0;     // Minuto inizio automazione
short end_h = 22;      // Ora fine automazione
short end_m = 30;      // Minuto fine automazione
short enable_clock = 1;// Abilita/disabilita controllo orario

int i = 0;             // Contatore generico
int dimmerValue = 0;   // Valore dimmer lampada (0-100)

BlynkTimer timer;      // Timer per esecuzione periodica

/*************************************************************
 * Funzione di controllo principale eseguita periodicamente
 *************************************************************/
void myTimer() 
{
  // Lettura sensore luminosità ambientale
  sensorVal = analogRead(PHOTOPIN); 

  // Gestione sincronizzazione orario NTP
  if (ntpCounter < 255) 
  {
    if(ntpCounter == 0){
      timeClient.update();
      
      if (timeClient.updated())
      {  
        Serial.println("********UPDATED********");
        ntpCounter++; 
      }
      else
        Serial.println("******NOT UPDATED******");
    
      // Debug informazioni orario
      Serial.println("UTC : " + timeClient.getFormattedUTCTime());
      Serial.println("UTC : " + timeClient.getFormattedUTCDateTime());
      Serial.println("LOC : " + timeClient.getFormattedTime());
      Serial.println("LOC : " + timeClient.getFormattedDateTime());
      Serial.println("UTC EPOCH : " + String(timeClient.getUTCEpochTime()));
      Serial.println("LOC EPOCH : " + String(timeClient.getEpochTime()));
    }
    else ntpCounter++; 
  }
  else 
  {
    ntpCounter = 0;  // Reset contatore dopo 255 cicli
  }

  // Controllo attivazione illuminazione
  bool shouldActivate = (
    sensorVal <= brightnessActivationLimit &&  // Controllo soglia luminosità
    (enable_clock == 0 || isInRange())         // Controllo orario se abilitato
  );

  if(shouldActivate)
  {
    // Accensione striscia NeoPixel (3 segmenti)
    for(i = 0; i < 17; i++)
      strip.setPixelColor(i, r1, g1, b1);
    for(; i < 17+17; i++)
      strip.setPixelColor(i, r2, g2, b2);
    for(; i < 17+17+16; i++)
      strip.setPixelColor(i, r3, g3, b3);
    strip.setBrightness(brightenss);

    // Controllo lampada dimmerabile
    if(dimmerValue > 0)
    {
      dimmer.setState(ON);
      dimmer.setPower(dimmerValue);  // Imposta intensità
    }
    else 
    {
      dimmer.setState(OFF);
    }
  }
  else
  {
    // Spegnimento completo
    strip.clear();
    dimmer.setPower(0);
    dimmer.setState(OFF);
  }
  
  strip.show();  // Applica le modifiche ai NeoPixel

  // Invio valore luminosità a Blynk
  Blynk.virtualWrite(V10, sensorVal); 
}

/*************************************************************
 * Verifica se l'orario corrente rientra nell'intervallo programmato
 *************************************************************/
boolean isInRange(){
  // Caso: stesso orario inizio/fine (es. 12:30 - 12:45)
  if(timeClient.getHours() == start_h && 
     timeClient.getMinutes() >= start_m && 
     timeClient.getMinutes() <= end_m && 
     start_h == end_h) return true;
  
  // Caso: inizio giornata (es. 12:30 - 13:30, controllo minuti)
  if(timeClient.getHours() == start_h && 
     timeClient.getMinutes() >= start_m && 
     start_h != end_h) return true;
  
  // Caso: ore intermedie (es. 13:35 in range 12:30 - 14:45)
  if(timeClient.getHours() > start_h && 
     timeClient.getHours() < end_h) return true;
  
  // Caso: fine giornata (es. 14:35 in range 12:30 - 14:45)
  if(timeClient.getHours() == end_h && 
     timeClient.getMinutes() <= end_m) return true;
  
  return false;
}

/*************************************************************
 * Handler sincronizzazione valori Blynk
 * Viene chiamato al collegamento del dispositivo
 *************************************************************/
BLYNK_CONNECTED()
{
  // Sincronizza tutti i virtual pin
  Blynk.syncVirtual(V0);   // Luminosità LED
  Blynk.syncVirtual(V1);   // Rosso segmento 1
  Blynk.syncVirtual(V2);   // Verde segmento 1
  Blynk.syncVirtual(V3);   // Blu segmento 1
  Blynk.syncVirtual(V4);   // Rosso segmento 2
  Blynk.syncVirtual(V5);   // Verde segmento 2
  Blynk.syncVirtual(V6);   // Blu segmento 2
  Blynk.syncVirtual(V7);   // Rosso segmento 3
  Blynk.syncVirtual(V8);   // Verde segmento 3
  Blynk.syncVirtual(V9);   // Blu segmento 3
  Blynk.syncVirtual(V11);  // Soglia attivazione fotoresistore
  Blynk.syncVirtual(V12);  // Ora inizio
  Blynk.syncVirtual(V13);  // Minuto inizio
  Blynk.syncVirtual(V14);  // Ora fine
  Blynk.syncVirtual(V15);  // Minuto fine
  Blynk.syncVirtual(V16);  // Abilita orario
  Blynk.syncVirtual(V17);  // Valore dimmer
}

/*************************************************************
 * Handler per i Virtual Pin Blynk
 * Ogni funzione gestisce l'aggiornamento di un parametro
 *************************************************************/

// Luminosità striscia LED
BLYNK_WRITE(V0) { brightenss = param.asInt(); }

// Primo segmento NeoPixel (Rosso, Verde, Blu)
BLYNK_WRITE(V1) { r1 = param.asInt(); }
BLYNK_WRITE(V2) { g1 = param.asInt(); }
BLYNK_WRITE(V3) { b1 = param.asInt(); }

// Secondo segmento NeoPixel (Rosso, Verde, Blu)
BLYNK_WRITE(V4) { r2 = param.asInt(); }
BLYNK_WRITE(V5) { g2 = param.asInt(); }
BLYNK_WRITE(V6) { b2 = param.asInt(); }

// Terzo segmento NeoPixel (Rosso, Verde, Blu)
BLYNK_WRITE(V7) { r3 = param.asInt(); }
BLYNK_WRITE(V8) { g3 = param.asInt(); }
BLYNK_WRITE(V9) { b3 = param.asInt(); }

// Soglia attivazione fotoresistore
BLYNK_WRITE(V11) { brightnessActivationLimit = param.asInt(); }

// Programmazione oraria (Inizio e Fine)
BLYNK_WRITE(V12) { start_h = param.asInt(); }
BLYNK_WRITE(V13) { start_m = param.asInt(); }
BLYNK_WRITE(V14) { end_h = param.asInt(); }
BLYNK_WRITE(V15) { end_m = param.asInt(); }

// Abilita/disabilita controllo orario
BLYNK_WRITE(V16) { enable_clock = param.asInt(); }

// Valore dimmer lampada
BLYNK_WRITE(V17) { dimmerValue = param.asInt(); }

/*************************************************************
 * INIZIALIZZAZIONE
 *************************************************************/
void setup()
{
  // Debug console
  Serial.begin(115200);

  // Connessione Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  // Timer esecuzione funzione principale (ogni secondo)
  timer.setInterval(1000L, myTimer); 

  // Configurazione client NTP
  timeClient.begin();
  timeClient.setTimeOffset(3600 * TIME_ZONE_OFFSET_HRS);
  timeClient.setUpdateInterval(60 * 60000);  // Aggiornamento ogni ora
  
  Serial.println("Using NTP Server " + timeClient.getPoolServerName());

  // Inizializzazione NeoPixel
  strip.begin();
  strip.clear();
  strip.show();

  // Inizializzazione dimmer
  dimmer.begin(NORMAL_MODE, OFF);
  dimmer.setPower(0);

  // Configurazione OTA (Over-The-Air updates)
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  // Configurazione LED integrato
  //pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, LOW);
}

/*************************************************************
 * LOOP PRINCIPALE
 *************************************************************/
void loop()
{
  Blynk.run();        // Gestisce comunicazione Blynk
  timer.run();        // Esegue myTimer periodicamente
  ArduinoOTA.handle(); // Gestisce aggiornamenti OTA
}
