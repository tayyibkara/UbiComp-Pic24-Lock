# UbiComp-Pic24-Lock
Application for the Pic24 Starter Kit implementing a identification system  using capacitive touch sensors and OLED display.
# PIC24F Secret Key Input

This project implements an identification application (Secret Key Lock) on the **PIC24F Starter Kit 1**. It utilizes the board's capacitive touch pads to capture a user PIN and provides feedback via the OLED display and LEDs.

## 🎯 Project Goal
Create a robust embedded C application using MPLAB X IDE that:
1. Detects touch inputs from the capacitive pads (1-5).
2. Validates the input sequence against a stored "Secret Key".
3. Indicates success (Unlock) or failure (Access Denied) using LEDs and the OLED screen.

## 🛠 Hardware
* **Board:** Microchip PIC24F Starter Kit 1
* **Microcontroller:** PIC24F (16-bit, low power)
* **Input:** 5 Capacitive Touch Pads (CTMU)
* **Output:** OLED Display, LEDs

## 💻 Software & Tools
* **IDE:** MPLAB X IDE
* **Compiler:** XC16 Compiler
* **Language:** C
