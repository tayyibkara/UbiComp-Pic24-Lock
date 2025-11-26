/* * Secret Key Project with Corrected Display Logic
 * Hardware: PIC24F Starter Kit
 * Logic: Detects sequence UP -> DOWN -> LEFT -> RIGHT
 */

#include "PIC24FStarter.h"
#include <string.h>

// --- Configuration ---
// Button Mapping: 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT, 4=CENTER
// Secret Code Sequence: UP, DOWN, LEFT, RIGHT
const uint8_t SECRET_CODE[4] = {3, 0, 1, 2}; 
#define CODE_LENGTH 4

// --- Simple 5x7 Font Data for Text Display ---
const uint8_t FONT[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // SPACE
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

// --- Helper Functions ---

// Simple delay using Timer1
void delay_ms(unsigned int milliseconds) {
    T1CONbits.TCKPS = 0b11; // Prescale 1:256
    PR1 = 47; TMR1 = 0; 
    T1CONbits.TON = 1; 
    unsigned int count = 0;
    while (count < milliseconds) {
        while (!IFS0bits.T1IF); 
        IFS0bits.T1IF = 0; 
        count++;
    }
    T1CONbits.TON = 0; 
}

// Helper to draw a single character
void DrawChar(uint8_t x, uint8_t y, char c) {
    if (c < 'A' || c > 'Z') c = ' '; // Handle unsupported chars as space
    int index = (c == ' ') ? 0 : (c - 'A' + 1);
    
    for (int col = 0; col < 5; col++) {
        uint8_t columnData = FONT[index][col];
        for (int row = 0; row < 8; row++) {
            if ((columnData >> row) & 0x01) {
                PutPixel(x + col, y + row);
            }
        }
    }
}

// Helper to draw a string
void DrawString(uint8_t x, uint8_t y, char* str) {
    int cursorX = x;
    while (*str) {
        DrawChar(cursorX, y, *str);
        cursorX += 6; // Move cursor (5 width + 1 spacing)
        str++;
    }
}

// Identify which button is currently pressed
// Returns -1 if none, 0..4 if pressed
int GetPressedButton() {
    // Check buttons array from TouchSense.c
    // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT, 4=CENTER
    for(int i = 0; i < NUM_TOUCHPADS; i++) {
        if(buttons[i] == 1) { 
            return i;
        }
    }
    return -1;
}

// --- Main Application ---

int main(void) {
    // 1. Initialization
    INIT_CLOCK(); 
    CTMUInit(); 
    RGBMapColorPins();
    RGBTurnOnLED();
    ResetDevice();
    
    // ============================================
    // SENSOR WARM-UP & CALIBRATION
    // ============================================
    
    // Step A: Ensure screen is cleared to BLACK first
    SetColor(BLACK);
    ClearDevice();

    // Step B: Write Calibration Message in WHITE
    SetColor(WHITE);
    DrawString(10, 20, "CALIBRATING");
    
    // Step C: Run the sensor read loop ~250 times
    // The library discards the first 160 readings to establish a baseline.
    for(int i = 0; i < 250; i++) {
        ReadCTMU();
    }
    
    // Step D: Clear the screen again (Fill with BLACK)
    SetColor(BLACK); 
    ClearDevice(); 
    // ============================================

    // App State Variables
    int currentStep = 0;
    int lastButtonState = -1;
    int isLocked = 1;

    // Initial Screen Setup
    SetColor(WHITE); // Switch back to WHITE to draw text
    DrawString(10, 10, "ENTER KEY");
    SetRGBs(255, 0, 0); // Red LED = Locked

    while(1) { 
        // 2. Read Sensors
        ReadCTMU(); 
        int currentButton = GetPressedButton();

        // 3. Button Press Detection (State Machine)
        // We only act on a "New Press" (when button goes from -1 to something else)
        if (currentButton != -1 && lastButtonState == -1) {
            
            // Visual Feedback for press (Blue flash)
            SetRGBs(0, 0, 255); 
            
            // Check if the pressed button matches the expected secret code
            if (currentButton == SECRET_CODE[currentStep]) {
                currentStep++; // Correct! Advance to next step
            } else {
                currentStep = 0; // Wrong! Reset sequence
                // Optional: Flash Red to indicate error
                SetRGBs(255, 0, 0);
                delay_ms(100);
                SetRGBs(0, 0, 255); // Back to Blue for press feedback
            }

            // If we reached the end of the code
            if (currentStep == CODE_LENGTH) {
                isLocked = !isLocked; // Toggle Lock State
                
                // Clear screen (Set to black, clear, then set to white for text)
                SetColor(BLACK);
                ClearDevice();
                SetColor(WHITE);

                if (!isLocked) {
                    DrawString(10, 20, "ACCESS");
                    DrawString(10, 30, "GRANTED");
                    SetRGBs(0, 255, 0); // Green
                } else {
                    DrawString(10, 20, "LOCKED");
                    SetRGBs(255, 0, 0); // Red
                }
                currentStep = 0; // Reset sequence counter
            }
        }

        // 4. Button Release Logic
        // When button is released, restore LED to status color
        if (currentButton == -1 && lastButtonState != -1) {
            if (!isLocked) {
                SetRGBs(0, 255, 0); // Green
            } else {
                SetRGBs(255, 0, 0); // Red
            }
        }

        lastButtonState = currentButton;
        delay_ms(50); // Small debounce delay
    }
    
    RGBTurnOffLED();
    return 0;
}