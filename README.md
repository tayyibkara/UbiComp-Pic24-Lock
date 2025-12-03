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

## ✨ Implemented Features

### 🔐 Security System
- **4-Digit Code:** Exactly 4 button presses required (Sequence: LEFT → UP → RIGHT → DOWN)
- **Code Verification:** Compares entered sequence against stored secret code
- **State Management:** Lock/Unlock toggle on correct code entry

### 📱 User Interface
- **Standby Mode:** Initial state displaying "STANDBY" / "PRESS CENTER"
- **Key-Entry Phase:** "ENTER KEY" / "CODE 4" display for user guidance
- **Visual Input Feedback:** Blinking asterisks (600ms cycle: 300ms on/off) for each button press
- **Real-time Code Length Display:** Shows required code length (4 characters)

### ⏱️ Timing Systems
- **15-Second Entry Timer:** Countdown display top-right ("15 SEC" → "0 SEC")
- **Timer Expiration Handling:** Displays "TIME OUT" / "TRY AGAIN" on expiration (2 seconds)
- **45-Second Inactivity Timeout:** Returns to Standby after 45 seconds without button press
- **Inactivity Counter:** Runs continuously throughout multiple entry attempts

### 💬 Error Messages & Feedback
- **Wrong Entry:** "KEY WRONG" / "TRY AGAIN" (Red, 2 seconds)
- **Correct Entry:** "ACCESS GRANTED" (Green, with lock toggle)
- **Lock State:** "LOCKED" displayed (Red)

### 🎨 LED Feedback
- **Red (255,0,0):** Locked or error state
- **Green (0,255,0):** Unlocked/Access Granted
- **Blue (0,0,255):** Button press in progress
- **Status Retention:** LED shows current lock state between attempts

### 🔤 Character Set
- **Extended Font:** 39 characters (space, *, :, digits 0-9, uppercase A-Z)
- **5x7 Bitmap Rendering:** Individual pixel rendering for each character on OLED display

### 🕐 Timing Behavior
- **Non-Blocking Timers:** Uses globalTimer with 1ms granularity
- **Concurrent Timer Management:** 15-second entry timer runs in parallel with 45-second inactivity timer
- **Continuous Inactivity Tracking:** Timer resets only on actual button presses, not on timeouts or errors

### 🔄 State Machine
- **2 Main Modes:** Standby (awaits CENTER button) and Key-Entry (expects code input)
- **Automatic Mode Switching:**
  - Standby → Key-Entry: On CENTER button press
  - Key-Entry → Standby: After 45 seconds inactivity
  - Key-Entry remains active: After wrong entry or timer expiration
