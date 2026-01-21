/* * Secret Key Project: Main Logic with Menu
 * Hardware: PIC24F Starter Kit
 * Logic: Standby -> Code Entry -> Menu -> (Lock OR Change Key)
 */

#include "PIC24FStarter.h"
#include <string.h>
#include <stdio.h> // Needed for sprintf
#include "DisplayUtils.h" 
#include "SystemUtils.h"  

// --- Configuration ---
// Button Mapping: 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT, 4=CENTER
// NOTE: Removed 'const' so we can change it. Size increased to 8.
uint8_t secretCode[8] = {3, 0, 1, 2, 0, 0, 0, 0}; 
int codeLength = 4; // Replaced #define w  ith variable
int failedAttempts = 0; 
int lastDisplayedTime = -1;
// --- Prototypes ---
void ShowMenu(void);
void ChangePassword(void);
void HandleLockout(void);
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
    SetColor(BLACK); ClearDevice();
    SetColor(WHITE); DrawString(10, 20, "CALIBRATING");
    
    for(int i = 0; i < 250; i++) { ReadCTMU(); }
    
    SetColor(BLACK); ClearDevice(); 
    // ============================================

    // App State Variables
    int currentStep = 0;
    int lastButtonState = -1;
    int isLocked = 1;
    uint8_t enteredCode[8] = {0}; // Increased to 8
    int isStandby = 1; 
    int timerActive = 0; 
    unsigned long timerStart = 0; 

    // Inactivity timer variables
    unsigned long lastActivityTime = 0; 
    const unsigned long INACTIVITY_TIMEOUT = 45000; 
    
    // Blinking asterisks variables
    int blinkingSteps = 0; 
    unsigned long blinkStart = 0; 
    int isBlinking = 0; 

    // Initial Screen Setup - Standby Screen
    SetColor(WHITE); 
    DrawString(10, 20, "STANDBY");
    DrawString(10, 35, "PRESS CENTER");
    SetRGBs(255, 0, 0); // Red LED = Locked

    while(1) { 
        // 2. Read Sensors
        ReadCTMU(); 
        int currentButton = GetPressedButton();
        
        // ============================================
        // STANDBY MODE
        // ============================================
        if (isStandby) {
            if (currentButton == 4 && lastButtonState == -1) { // CENTER button
                isStandby = 0;
                currentStep = 0;
                isBlinking = 0;
                blinkingSteps = 0;
                lastActivityTime = globalTimer; 
                
                SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                DrawString(10, 10, "ENTER KEY");
                SetRGBs(255, 0, 0); // Red LED
            }
            lastButtonState = currentButton;
            delay_ms(50);
            continue; 
        }
        
        // ============================================
        // INACTIVITY CHECK
        // ============================================
        unsigned long inactivityElapsed = globalTimer - lastActivityTime;
        if (inactivityElapsed >= INACTIVITY_TIMEOUT) {
            // Timeout -> Return to standby
            currentStep = 0;
            isBlinking = 0;
            timerActive = 0;
            isStandby = 1;
            
            SetColor(BLACK); ClearDevice(); SetColor(WHITE);
            DrawString(10, 20, "STANDBY");
            DrawString(10, 35, "PRESS CENTER");
            SetRGBs(255, 0, 0); 
            
            lastButtonState = currentButton;
            delay_ms(50);
            continue; 
        }
        
        // ============================================
        // KEY ENTRY MODE
        // ============================================
if (timerActive) {
    unsigned long elapsedTime = globalTimer - timerStart;
    int secondsRemaining = 15 - (elapsedTime / 1000);
    
    // Timeout Check
    if (secondsRemaining <= 0) {
        timerActive = 0;
        currentStep = 0;
        isBlinking = 0;
        lastDisplayedTime = -1; // Reset tracker
        
        SetColor(BLACK); ClearDevice(); SetColor(WHITE);
        DrawString(10, 15, "TIME OUT");
        delay_ms(2000); 
        
        SetColor(BLACK); ClearDevice(); SetColor(WHITE);
        DrawString(10, 10, "ENTER KEY");
        lastButtonState = currentButton;
        delay_ms(50);
        continue; 
    }
    
    // THE FIX: Only draw if the number changed!
    if (secondsRemaining != lastDisplayedTime) {
        DisplayTimer(secondsRemaining);
        lastDisplayedTime = secondsRemaining;
    }
}
        
        // Blink Logic
        if (isBlinking && blinkingSteps > 0) {
            unsigned long blinkElapsed = globalTimer - blinkStart;
            if (blinkElapsed >= 600) {
                isBlinking = 0;
                DisplayPasswordProgress(blinkingSteps);
            } else {
                if ((blinkElapsed / 300) % 2 == 0) DisplayPasswordProgress(blinkingSteps);
                else {
                    SetColor(BLACK);
                    for (int i = 0; i < 30; i++) for (int j = 0; j < 8; j++) PutPixel(10 + i, 30 + j);
                }
            }
        }

        // ============================================
        // BUTTON PRESS LOGIC
        // ============================================
        if (currentButton != -1 && lastButtonState == -1) {
            
            if (currentStep == 0 && !timerActive) {
                timerActive = 1;
                timerStart = globalTimer;
                lastActivityTime = globalTimer; 
            }
            lastActivityTime = globalTimer;
            
            SetRGBs(0, 0, 255); // Blue flash
            
            if (currentStep < codeLength) {
                enteredCode[currentStep] = currentButton;
                currentStep++; 
                DisplayPasswordProgress(currentStep);
                blinkingSteps = currentStep;
                isBlinking = 1;
                blinkStart = globalTimer;
            }
            
            // Check Code
            if (currentStep == codeLength) {
                timerActive = 0;
                
                int isCorrect = 1;
                for (int i = 0; i < codeLength; i++) {
                    if (enteredCode[i] != secretCode[i]) {
                        isCorrect = 0;
                        break;
                    }
                }
                
                SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                
                if (isCorrect) {
                    // --- SUCCESS ---
                    failedAttempts = 0;
                    DrawString(10, 20, "UNLOCKED");
                    SetRGBs(0, 255, 0); // Green
                    delay_ms(1000);
                    
                    // !!! ENTER MENU SCREEN !!!
                    ShowMenu();
                    
                    // Reset to Locked after Menu exit
                    isLocked = 1;
                    isStandby = 1;
                    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                    DrawString(10, 20, "STANDBY");
                    DrawString(10, 35, "PRESS CENTER");
                    SetRGBs(255, 0, 0); // Red
                    
                } else {
                    // --- WRONG CODE LOGIC ---
                    DrawString(10, 15, "KEY WRONG");
                    SetRGBs(255, 0, 0); // Red
                    
                    // 1. Increment Counter
                    failedAttempts++;
                    
                    // 2. Check Limit (3 attempts)
                    if (failedAttempts >= 3) {
                        HandleLockout(); // Trigger the 30s wait
                    } else {
                        delay_ms(2000); // Normal short delay
                    }
                    
                    // 3. Reset Screen
                    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                    DrawString(10, 10, "ENTER KEY");
                }
                currentStep = 0;
                isBlinking = 0;
            }
        }

        if (currentButton == -1 && lastButtonState != -1) {
            if (isLocked) SetRGBs(255, 0, 0); // Red
        }

        lastButtonState = currentButton;
        delay_ms(50); 
    }
    return 0;
}

// ============================================
// MENU LOGIC
// ============================================
void ShowMenu(void) {
    int selection = 0; 
    int inMenu = 1;
    int lastBtn = -1;
    int needsRedraw = 1; // Start true to draw the first frame

    // Wait for button release
    while(GetPressedButton() != -1) ReadCTMU();

    while(inMenu) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        // THE FIX: Only draw when 'needsRedraw' is true
        if (needsRedraw) {
            SetColor(BLACK); ClearDevice(); SetColor(WHITE);
            DrawString(10, 5, "MAIN MENU");
            
            if (selection == 0) DrawString(10, 25, "> LOCK SYSTEM");
            else DrawString(10, 25, "  LOCK SYSTEM");
            
            if (selection == 1) DrawString(10, 40, "> CHANGE KEY");
            else DrawString(10, 40, "  CHANGE KEY");
            
            needsRedraw = 0; // Stop drawing until next input
        }
        
        if (btn != -1 && btn != lastBtn) {
            if (btn == 0 || btn == 2) { // Scroll
                selection = !selection; 
                needsRedraw = 1; // Input happened -> Redraw next loop
            } 
            else if (btn == 4 || btn == 1) { // Select
                if (selection == 0) inMenu = 0; 
                else {
                    ChangePassword(); 
                    needsRedraw = 1; // Redraw when returning from sub-menu
                }
            }
            delay_ms(150);
        }
        lastBtn = btn;
        delay_ms(50);
    }
}

void ChangePassword(void) {
    int newLen = 4;
    int choosing = 1;
    int lastBtn = -1;
    int needsRedraw = 1; // Fix flag
    
    while(GetPressedButton() != -1) ReadCTMU();
    
    // --- STEP 1: CHOOSE LENGTH ---
    while(choosing) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        // THE FIX: Only draw on change
        if (needsRedraw) {
            SetColor(BLACK); ClearDevice(); SetColor(WHITE);
            DrawString(10, 5, "LENGTH?");
            
            if (newLen == 4) DrawString(10, 25, "> 4 DIGITS");
            else DrawString(10, 25, "  4 DIGITS");
            
            if (newLen == 8) DrawString(10, 40, "> 8 DIGITS");
            else DrawString(10, 40, "  8 DIGITS");
            
            needsRedraw = 0;
        }
        
        if (btn != -1 && btn != lastBtn) {
            if (btn == 0 || btn == 2) { 
                newLen = (newLen == 4) ? 8 : 4;
                needsRedraw = 1; // Trigger redraw
            }
            else if (btn == 4 || btn == 1) choosing = 0;
            delay_ms(150);
        }
        lastBtn = btn;
        delay_ms(50);
    }
    
    // --- STEP 2: ENTER NEW CODE ---
    int count = 0;
    uint8_t temp[8];
    
    // Draw background ONCE
    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
    DrawString(10, 10, "ENTER NEW:");
    
    while(GetPressedButton() != -1) ReadCTMU();
    lastBtn = -1;
    
    while(count < newLen) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        if (btn != -1 && btn != lastBtn) {
            if (btn == 4) continue; 
            
            temp[count] = btn;
            count++;
            
            // THE FIX: Just draw the new star, don't clear the screen
            SetColor(WHITE);
            for(int i=0; i<count; i++) DrawChar(10 + (i*6), 30, '*');
            
            SetRGBs(0, 0, 255); delay_ms(100); SetRGBs(0, 255, 0);
        }
        lastBtn = btn;
    }
    
    // Save Logic
    codeLength = newLen;
    for(int i=0; i<8; i++) {
        if(i < newLen) secretCode[i] = temp[i];
        else secretCode[i] = 0;
    }
    
    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
    DrawString(10, 25, "SAVED!");
    delay_ms(1500);
}
void HandleLockout(void) {
    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
    DrawString(5, 10, "SYSTEM LOCKED");
    DrawString(5, 25, "WAIT...");
    
    // 30 Second Countdown Loop
    for (int i = 30; i > 0; i--) {
        DisplayTimer(i); // Reuses your existing timer display
        
        // Alarm Effect: Flash Red/Blue
        SetRGBs(255, 0, 0); delay_ms(500);
        SetRGBs(0, 0, 255); delay_ms(500);
    }
    
    // Reset Logic
    failedAttempts = 0;
    SetRGBs(255, 0, 0); // Back to solid Red
    
    // Restore Screen
    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
}