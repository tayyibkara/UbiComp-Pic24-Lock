/* * Secret Key Project with Corrected Display Logic
 * Hardware: PIC24F Starter Kit
 * Logic: Detects sequence UP -> DOWN -> LEFT -> RIGHT
 */

#include "PIC24FStarter.h"
#include <string.h>
#include "DisplayUtils.h" // New helper for OLED drawing
#include "SystemUtils.h"  
// --- Configuration ---
// Button Mapping: 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT, 4=CENTER
// Secret Code Sequence: UP, DOWN, LEFT, RIGHT
const uint8_t SECRET_CODE[4] = {3, 0, 1, 2}; 
#define CODE_LENGTH 4

// --- Simple 5x7 Font Data for Text Display ---
//const uint8_t FONT[][5] = {
//    {0x00, 0x00, 0x00, 0x00, 0x00}, // SPACE (0)
//    {0x08, 0x1C, 0x3E, 0x1C, 0x08}, // * (ASTERISK) (1)
//    {0x00, 0x00, 0x06, 0x00, 0x00}, // : (COLON) (2)
//    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0 (3)
//    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 1 (4)
//    {0x71, 0x49, 0x49, 0x49, 0x46}, // 2 (5)
//    {0x41, 0x49, 0x49, 0x49, 0x36}, // 3 (6)
//    {0x0F, 0x08, 0x08, 0x08, 0x7F}, // 4 (7)
//    {0x4F, 0x49, 0x49, 0x49, 0x31}, // 5 (8)
//    {0x3E, 0x49, 0x49, 0x49, 0x32}, // 6 (9)
//    {0x01, 0x01, 0x79, 0x05, 0x03}, // 7 (10)
//    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8 (11)
//    {0x26, 0x49, 0x49, 0x49, 0x3E}, // 9 (12)
//    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A (13)
//    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B (14)
//    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C (15)
//    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D (16)
//    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E (17)
//    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F (18)
//    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G (19)
//    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H (20)
//    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I (21)
//    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J (22)
//    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K (23)
//    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L (24)
//    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M (25)
//    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N (26)
//    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O (27)
//    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P (28)
//    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q (29)
//    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R (30)
//    {0x46, 0x49, 0x49, 0x49, 0x31}, // S (31)
//    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T (32)
//    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U (33)
//    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V (34)
//    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W (35)
//    {0x63, 0x14, 0x08, 0x14, 0x63}, // X (36)
//    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y (37)
//    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z (38)
//};

// --- Helper Functions ---

// Global timer variable to track elapsed time
//static unsigned long globalTimer = 0;

// Simple delay using Timer1
//void delay_ms(unsigned int milliseconds) {
//    T1CONbits.TCKPS = 0b11; // Prescale 1:256
//    PR1 = 47; TMR1 = 0; 
//    T1CONbits.TON = 1; 
//    unsigned long count = 0; // Changed from int to unsigned long to prevent overflow
//    while (count < milliseconds) {
//        while (!IFS0bits.T1IF); 
//        IFS0bits.T1IF = 0; 
//        count++;
//        globalTimer += 1; // Increment global timer by 1ms for each interrupt
//    }
//    T1CONbits.TON = 0; 
//}
//
//// Helper to draw a single character
//void DrawChar(uint8_t x, uint8_t y, char c) {
//    int index;
//    if (c == '*') {
//        index = 1; // Asterisk at index 1
//    } else if (c == ':') {
//        index = 2; // Colon at index 2
//    } else if (c >= '0' && c <= '9') {
//        index = 3 + (c - '0'); // Digits 0-9 at indices 3-12
//    } else if (c >= 'A' && c <= 'Z') {
//        index = 13 + (c - 'A'); // Letters A-Z at indices 13-38
//    } else {
//        index = 0; // Space for unsupported chars
//    }
//    
//    for (int col = 0; col < 5; col++) {
//        uint8_t columnData = FONT[index][col];
//        for (int row = 0; row < 8; row++) {
//            if ((columnData >> row) & 0x01) {
//                PutPixel(x + col, y + row);
//            }
//        }
//    }
//}
//
//// Helper to draw a string
//void DrawString(uint8_t x, uint8_t y, char* str) {
//    int cursorX = x;
//    while (*str) {
//        DrawChar(cursorX, y, *str);
//        cursorX += 6; // Move cursor (5 width + 1 spacing)
//        str++;
//    }
//}
//
//// Helper to display entered password asterisks with blinking
//void DisplayPasswordProgress(int stepsEntered) {
//    // Clear the password display area first
//    SetColor(BLACK);
//    for (int i = 0; i < 30; i++) {
//        for (int j = 0; j < 8; j++) {
//            PutPixel(10 + i, 30 + j);
//        }
//    }
//    
//    // Draw entered asterisks in WHITE
//    SetColor(WHITE);
//    for (int i = 0; i < stepsEntered; i++) {
//        DrawChar(10 + (i * 6), 30, '*');
//    }
//}
//
//// Helper to display timer on screen
//void DisplayTimer(int secondsRemaining) {
//    // Clear the timer area (right side of screen)
//    SetColor(BLACK);
//    for (int i = 50; i < 128; i++) {
//        for (int j = 0; j < 16; j++) {
//            PutPixel(i, 20 + j);
//        }
//    }
//    
//    // Draw timer in WHITE with "SEC" label on the same line
//    SetColor(WHITE);
//    
//    // Draw the seconds number
//    if (secondsRemaining < 10) {
//        DrawChar(70, 20, '0' + secondsRemaining);
//        DrawString(76, 20, "SEC");
//    } else {
//        DrawChar(70, 20, '0' + (secondsRemaining / 10));
//        DrawChar(76, 20, '0' + (secondsRemaining % 10));
//        DrawString(82, 20, "SEC");
//    }
//}
//
//// Identify which button is currently pressed
//// Returns -1 if none, 0..4 if pressed
//int GetPressedButton() {
//    // Check buttons array from TouchSense.c
//    // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT, 4=CENTER
//    for(int i = 0; i < NUM_TOUCHPADS; i++) {
//        if(buttons[i] == 1) { 
//            return i;
//        }
//    }
//    return -1;
//}
//
//// --- Main Application ---

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
    uint8_t enteredCode[4] = {0, 0, 0, 0}; // Store entered code
    int isStandby = 1; // Standby mode flag
    int timerActive = 0; // Timer not active initially
    unsigned long timerStart = 0; // Will store the time when first input is made
    const int TIMER_DURATION = 15000; // 15 seconds in milliseconds
    
    // Inactivity timer variables (45 seconds)
    unsigned long lastActivityTime = 0; // Track last button press time
    const unsigned long INACTIVITY_TIMEOUT = 45000; // 45 seconds in milliseconds
    
    // Blinking asterisks variables
    int blinkingSteps = 0; // Number of steps that should blink
    unsigned long blinkStart = 0; // When blinking started
    int isBlinking = 0; // Is currently blinking

    // Initial Screen Setup - Standby Screen
    SetColor(WHITE); // Switch back to WHITE to draw text
    DrawString(10, 20, "STANDBY");
    DrawString(10, 35, "PRESS CENTER");
    SetRGBs(255, 0, 0); // Red LED = Locked

    while(1) { 
        // 2. Read Sensors
        ReadCTMU(); 
        int currentButton = GetPressedButton();
        
        // ============================================
        // STANDBY MODE - Wait for CENTER button press
        // ============================================
        if (isStandby) {
            if (currentButton == 4 && lastButtonState == -1) { // CENTER button (index 4)
                // Exit standby mode, go to key entry
                isStandby = 0;
                currentStep = 0;
                isBlinking = 0;
                blinkingSteps = 0;
                lastActivityTime = globalTimer; // Initialize inactivity timer
                
                // Clear screen and show key entry prompt
                SetColor(BLACK);
                ClearDevice();
                SetColor(WHITE);
                DrawString(10, 10, "ENTER KEY");
                DrawString(10, 20, "CODE 4");
                SetRGBs(255, 0, 0); // Red LED = Locked
            }
            
            lastButtonState = currentButton;
            delay_ms(50);
            continue; // Skip to next iteration while in standby
        }
        
        // ============================================
        // CHECK FOR INACTIVITY TIMEOUT (45 seconds)
        // This must run EVERY loop when not in standby
        // ============================================
        if (!isStandby) {
            unsigned long inactivityElapsed = globalTimer - lastActivityTime;
            
            // DEBUG: Display elapsed time on screen
            SetColor(BLACK);
            for (int i = 0; i < 50; i++) {
                for (int j = 0; j < 8; j++) {
                    PutPixel(i, 50 + j);
                }
            }
            SetColor(WHITE);
            DrawString(10, 50, "INACT:");
            unsigned long debugSeconds = inactivityElapsed / 1000;
            if (debugSeconds < 10) {
                DrawChar(40, 50, '0' + debugSeconds);
            } else {
                DrawChar(40, 50, '0' + (debugSeconds / 10));
                DrawChar(46, 50, '0' + (debugSeconds % 10));
            }
            
            if (inactivityElapsed >= INACTIVITY_TIMEOUT) {
                // Inactivity timeout! Return to standby
                currentStep = 0;
                isBlinking = 0;
                blinkingSteps = 0;
                timerActive = 0;
                isStandby = 1;
                
                SetColor(BLACK);
                ClearDevice();
                SetColor(WHITE);
                DrawString(10, 20, "STANDBY");
                DrawString(10, 35, "PRESS CENTER");
                SetRGBs(255, 0, 0); // Red LED = Locked
                
                lastButtonState = currentButton;
                delay_ms(50);
                continue; // Skip to next iteration
            }
        }
        
        // ============================================
        // KEY ENTRY MODE - Handle blinking asterisks
        // ============================================
        
        // Display timer if active and not expired
        if (timerActive) {
            unsigned long elapsedTime = globalTimer - timerStart;
            int secondsRemaining = 15 - (elapsedTime / 1000);
            if (secondsRemaining < 0) secondsRemaining = 0;
            
            // Check if timer expired
            if (secondsRemaining <= 0) {
                // Timeout! Reset everything
                timerActive = 0;
                currentStep = 0;
                isBlinking = 0;
                blinkingSteps = 0;
                
                // Clear screen and show timeout message
                SetColor(BLACK);
                ClearDevice();
                SetColor(WHITE);
                DrawString(10, 15, "TIME OUT");
                DrawString(10, 30, "TRY AGAIN");
                SetRGBs(255, 0, 0); // Red LED
                
                delay_ms(2000); // Show message for 2 seconds
                
                // Return to key entry screen (not standby)
                SetColor(BLACK);
                ClearDevice();
                SetColor(WHITE);
                DrawString(10, 10, "ENTER KEY");
                DrawString(10, 20, "CODE 4");
                SetRGBs(255, 0, 0); // Red LED = Locked
                
                // DO NOT reset inactivity timer here! Let it continue counting
                
                lastButtonState = currentButton;
                delay_ms(50);
                continue; // Skip button detection and try again
            }
            
            // Timer still running - show countdown
            DisplayTimer(secondsRemaining);
            
            // Also display asterisks if any were entered
            if (currentStep > 0) {
                DisplayPasswordProgress(currentStep);
            }
        }
        
        // Handle blinking asterisks display
        if (isBlinking && blinkingSteps > 0) {
            unsigned long blinkElapsed = globalTimer - blinkStart;
            // Blink for 600ms total (300ms on, 300ms off pattern)
            if (blinkElapsed >= 600) {
                // Blinking done
                isBlinking = 0;
                DisplayPasswordProgress(blinkingSteps);
            } else {
                // Show blinking effect (on for 300ms, off for 300ms)
                if ((blinkElapsed / 300) % 2 == 0) {
                    // Show asterisks
                    DisplayPasswordProgress(blinkingSteps);
                } else {
                    // Hide asterisks (clear the area)
                    SetColor(BLACK);
                    for (int i = 0; i < 30; i++) {
                        for (int j = 0; j < 8; j++) {
                            PutPixel(10 + i, 30 + j);
                        }
                    }
                }
            }
        }

        // 3. Button Press Detection (State Machine)
        // We only act on a "New Press" (when button goes from -1 to something else)
        if (currentButton != -1 && lastButtonState == -1) {
            
            // Start timer on first input
            if (currentStep == 0 && !timerActive) {
                timerActive = 1;
                timerStart = globalTimer;
                lastActivityTime = globalTimer; // Reset inactivity timer
            }
            
            // Update activity time on every button press
            lastActivityTime = globalTimer;
            
            // Visual Feedback for press (Blue flash)
            SetRGBs(0, 0, 255); 
            
            // Store the button press if we haven't reached CODE_LENGTH yet
            if (currentStep < CODE_LENGTH) {
                enteredCode[currentStep] = currentButton; // Store the button press
                currentStep++; // Increment step
                // Display the progress with entered asterisks
                DisplayPasswordProgress(currentStep);
                // Start blinking for this new asterisk
                blinkingSteps = currentStep;
                isBlinking = 1;
                blinkStart = globalTimer;
            }
            
            // Check if we have collected all CODE_LENGTH inputs
            if (currentStep == CODE_LENGTH) {
                // Stop timer
                timerActive = 0;
                
                // Now verify if the entire sequence is correct
                int isCorrect = 1;
                for (int i = 0; i < CODE_LENGTH; i++) {
                    if (enteredCode[i] != SECRET_CODE[i]) {
                        isCorrect = 0;
                        break;
                    }
                }
                
                // Display result based on correctness
                SetColor(BLACK);
                ClearDevice();
                SetColor(WHITE);
                
                if (isCorrect) {
                    // Correct code!
                    currentStep = 0; // Reset current step
                    isLocked = !isLocked; // Toggle Lock State
                    if (!isLocked) {
                        DrawString(10, 20, "ACCESS");
                        DrawString(10, 30, "GRANTED");
                        SetRGBs(0, 255, 0); // Green
                    } else {
                        DrawString(10, 20, "LOCKED");
                        SetRGBs(255, 0, 0); // Red
                    }
                } else {
                    // Wrong code!
                    DrawString(10, 15, "KEY WRONG");
                    DrawString(10, 30, "TRY AGAIN");
                    SetRGBs(255, 0, 0); // Red
                }
                
                delay_ms(2000); // Show message for 2 seconds
                
                // Reset and go back to key entry (not standby)
                currentStep = 0;
                isBlinking = 0;
                blinkingSteps = 0;
                timerActive = 0;
                // DO NOT reset inactivity timer! Let it continue counting
                
                SetColor(BLACK);
                ClearDevice();
                SetColor(WHITE);
                DrawString(10, 10, "ENTER KEY");
                DrawString(10, 20, "CODE 4");
                SetRGBs(255, 0, 0); // Red LED = Locked
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