# Bluetooth LE Audio (Auracast) Transmitter
**Phase:** Proof of Concept (Simulation)

---

## Overview
This project focuses on the development of a **Bluetooth Low Energy (LE) Audio Transmitter** supporting the **Auracast** standard. The current phase is a **Proof of Concept (PoC) simulation**, demonstrating system functionality without physical hardware implementation. The project uses an **NXP NXH3675** transceiver integrated with an **Arduino-compatible controller** for simulation purposes.

---

## Objectives
- Simulate the transmission of BLE Audio streams using Auracast protocol.
- Validate communication between NXH3675 transceiver and controller firmware.
- Demonstrate Proof of Concept without creating physical hardware.
- Provide a basis for future hardware development and testing.

---

## Features (Simulation Phase)
- BLE Audio packet simulation
- Controller-to-transceiver communication over I²C/SPI (simulated)
- Basic audio stream configuration and management
- Logging of simulated data for analysis

---

## Hardware (Simulation Reference)
- **NXH3675** BLE Audio Transceiver (NXP)
- Arduino-compatible MCU (simulation platform)
> Note: Hardware is referenced for simulation purposes; no physical hardware is required in this phase.

---

## Software Requirements
- Arduino IDE or PlatformIO
- Python (for simulation scripts)
- Serial Monitor / Logging tool for output verification
- Git (for version control)

---

## Project Structure
/project
├─ /src # Source code for simulation
├─ /docs # Documentation and diagrams
├─ /test # Program for function Test
├─ README.md
└─ LICENSE

## How to simulate
1. Open the src folder.
2. The ESP32 program is located in src/main.
3. The Node.js backend program is located in src/web_side/read_serial.js.
4. The web interface is located in src/web_side/index.html.
5. Connect the ESP32 to your computer using a Serial-to-USB cable.
6. Open read_serial.js and adjust the ESP32 port based on the port shown in Device Manager.
7. Open a terminal and navigate to:
cd ../ble_auracast/src/web_side
8. Once the ESP32 is connected, run:
node read_serial.js
9. Open index.html in your browser.
10. Click the "Open Port" button.
11. After the port is open, all logs and BLE connection information will appear when the ESP32 connects to a BLE client.