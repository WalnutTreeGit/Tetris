# Embedded Tetris Table (ATmega328P + WS2812B)

![C](https://img.shields.io/badge/C-Embedded-blue) ![Hardware](https://img.shields.io/badge/Hardware-ATmega328P-red) ![Android](https://img.shields.io/badge/App-Android%20(Java)-green) ![Memory](https://img.shields.io/badge/RAM-2KB-orange)

## Project Overview
This project is a hardware-based implementation of the classic Tetris game logic, running natively on an **ATmega328P** microcontroller. The system drives a **300-pixel WS2812B LED matrix** in real-time while managing user input via a custom-built Android Bluetooth controller.

The core challenge was to implement collision detection, game state management, and high-frequency LED timing protocols within the strict **2KB SRAM** limit of the 8-bit microcontroller.

## System Architecture

### 1. Hardware Layer
* **MCU:** ATmega328P (running bare-metal C).
* **Display:** 10x20 Matrix of WS2812B (Neopixel) LEDs.
* **Comms:** HC-05 Bluetooth Module (UART interface).
* **Power:** 5V/10A PSU with capacitor smoothing for current spikes.

### 2. Embedded Firmware (C)
The firmware handles the game loop and critical timing:
* **State Management:** The game grid is stored as a flattened bit-array to minimize memory footprint.
* **Timing:** Bit-banging the WS2812B protocol (800kHz) requires precise instruction cycles. Interrupts are temporarily disabled during frame updates to prevent timing violations.
* **Input Handling:** Asynchronous UART interrupts capture commands from the mobile app without blocking the rendering loop.

### 3. Control Layer (Android)
A companion Android application serves as the controller. It connects via Bluetooth RFCOMM and serializes inputs (`LEFT`, `RIGHT`, `ROTATE`, `DROP`) into single-byte packets for low-latency transmission.

## Key Technical Challenges

### Optimization for 2KB RAM
With only 2048 bytes of memory, storing the 300 LEDs (3 bytes per pixel) takes up nearly 50% of available RAM. To leave room for the stack and game variables:
* **Bit-Packing:** The collision grid is decoupled from the color buffer, using bitwise operations to check occupancy.
* **Double Buffering:** Avoided full double-buffering; instead, we render line-by-line during the "latch" phase to save space.

### Concurrency
The system must handle three competing tasks:
1.  **Game Logic:** (Gravity, Line clearing, Scoring) ~60Hz.
2.  **Display Refresh:** (Pushing 24 bits x 300 LEDs) ~30ms.
3.  **Input Interrupts:** (Bluetooth packets) unpredictable.
*Solution:* implemented a non-blocking state machine where input interrupts set volatile flags that are consumed during the "idle" time between frame updates.


## Build Instructions

1.  **Flash Firmware:**
    ```bash
    avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o main.o main.c
    avr-objcopy -O ihex -R .eeprom main.elf main.hex
    avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:main.hex
    ```

2.  **Android App:**
    * Open `Android/` in Android Studio.
    * Build APK and install on device.

## Author
**João Paulo Fontoura Nogueira**
