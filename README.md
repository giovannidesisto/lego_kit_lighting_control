# ESP32 Smart Lighting Control System

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![ESP32](https://img.shields.io/badge/ESP32-Platform-blue)
![Arduino](https://img.shields.io/badge/Arduino-Framework-orange)



An advanced IoT lighting control system for exhibitions based on ESP32, combining NeoPixel LED strips and dimmable halogen lamps with remote control via Blynk IoT.

## 🚀 Features

- **NeoPixel LED Control**: 3 independent segments with customizable colors
- **TRIAC Dimmer**: PWM control of halogen lamps via RBDdimmer module
- **Blynk IoT Interface**: Remote control from smartphone/tablet
- **Time Automation**: Schedule-based operation with NTP synchronization
- **Light Sensor**: Intelligent activation based on ambient light
- **OTA Updates**: Firmware updates without physical connection
- **Operation Modes**: Manual and Automatic

## 🛠 Required Hardware

### Main Components

- ESP32 Dev Board
- NeoPixel LED Strip (50 LEDs)
- RBDdimmer Module for TRIAC control
- Dimmable Halogen Lamp
- Photoresistor (LDR)
- Appropriate Power Supply for LEDs and lamp

## Connections

| Component | ESP32 Pin | Notes |
|-----------|-----------|-------|
| NeoPixel Data | GPIO 22 | |
| Photoresistor | GPIO 34 (ADC) | With pull-down resistor |
| Dimmer PWM | GPIO 17 | |
| Zero-Cross | GPIO 16 | |

## ⚙️ Software Configuration

### Required Libraries
```cpp
#include <RBDdimmer.h>
#include <WiFi.h>
#include <NTPClient_Generic.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
```

### Blynk Configuration
    Template ID: TMPLHiqq3ISk
    Device Name: LOLIND32LITELED
    Insert your Auth Token in the code
### WiFi Credentials
Modify in code:
```cpp
    char ssid[] = "Your_SSID";
    char pass[] = "Your_Password";
```
# 🎮 Blynk Control
## Virtual Pin Mapping
| Pin      | Function                   | Type         |
|----------|----------------------------|--------------|
| V0       | LED Brightness             | Slider       |
| V1-V3    | Segment 1 Color (RGB)      | Color Picker |
| V4-V6    | Segment 2 Color (RGB)      | Color Picker |
| V7-V9    | Segment 3 Color (RGB)      | Color Picker |
| V10      | Photoresistor Reading      | Display      |
| V11      | Light Activation Threshold | Slider       |
| V12-V13  | Start Time (HH:MM)         | Input        |
| V14-V15  | End Time (HH:MM)           | Input        |
| V16      | Enable Time Control        | Switch       |
| V17      | Dimmer Lamp Value          | Slider       |

## 🔧 Operation
### Automatic Mode
    Time Control: Activation within scheduled interval
    Light Control: Activation only below ambient light threshold
    NTP Sync: Precise time with automatic updates
### Activation Logic
```cpp
bool shouldActivate = (
sensorVal <= brightnessActivationLimit && // Light threshold
(enable_clock == 0 || isInRange()) // Time control
);
```

### LED Segments Management
    Segment 1: LED 0-16
    Segment 2: LED 17-33
    Segment 3: LED 34-49

###  📋 Programmable Parameters
#### Activation Times
    Start Time: start_h : start_m
    End Time: end_h : end_m
    Enable: enable_clock
#### Thresholds
    Ambient Light: brightnessActivationLimit (0-4095)
    Dimmer Intensity: dimmerValue (0-100%)

### 🔄 OTA Updates
System supports Over-The-Air updates:
    Connect device to same WiFi network
    Use Arduino IDE or perform network upload
    Monitor progress via Serial Monitor

## 🐛 Troubleshooting
### NTP Synchronization
    Check Internet connection
    Verify correct time offset (+2 for Italy)
    Monitor Serial Monitor for error messages
### Blynk Control
    Verify correct Auth Token
    Check WiFi connection
    Use Blynk Console to monitor device
### Dimmer Not Working
    Check zero-cross detection connection
    Verify compatible load (resistive)
    Ensure adequate power supply

## 📝 Important Notes
    Electrical Safety: Dimmer module controls high voltage
    Heat Dissipation: Ensure adequate cooling for TRIAC
    LED Power: Use dedicated power supply for NeoPixels
    Manual Backup: System can be manually controlled via Blynk

## 📄 License
Open-source project for personal and educational use.

## UI
<img src="/images/mobile_ui.jpg" height="400"/>
<img src="/images/web_ui.png" height="400"/>

