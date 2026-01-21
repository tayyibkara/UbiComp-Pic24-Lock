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
void EnterSleepMode(void);
int main(void) {
    // 1. Initialization
    INIT_CLOCK(); 
    CTMUInit(); 
    RGBMapColorPins();
    RGBTurnOnLED();
    ResetDevice();
    
    // Load Code from Flash
    LoadCodeFromFlash(secretCode, &codeLength);
    
    // Calibration
    SetColor(BLACK); ClearDevice();
    SetColor(WHITE); DrawString(10, 20, "CALIBRATING");
    for(int i = 0; i < 250; i++) { ReadCTMU(); }
    SetColor(BLACK); ClearDevice(); 

    // App State Variables
    int currentStep = 0;
    int lastButtonState = -1;
    int isLocked = 1;
    uint8_t enteredCode[8] = {0}; 
    int isStandby = 1; 
    int timerActive = 0; 
    unsigned long timerStart = 0; 
    
    // Inactivity Variables
    unsigned long lastActivityTime = globalTimer; 
    int lastDisplayedTime = -1;

    // Blinking variables
    int blinkingSteps = 0; 
    unsigned long blinkStart = 0; 
    int isBlinking = 0; 

    // Initial Screen
    SetColor(WHITE); 
    DrawString(10, 20, "STANDBY");
    DrawString(10, 35, "PRESS CENTER");
    SetRGBs(255, 0, 0); 

    while(1) { 
        // 2. Read Sensors
        ReadCTMU(); 
        int currentButton = GetPressedButton();
        
        // ============================================
        // 1. GLOBAL INACTIVITY CHECK (Moved to TOP)
        // ============================================
        // We check this FIRST so it works even in Standby
        if (globalTimer - lastActivityTime >= 30000) {
            
            // CASE A: Already in Standby -> Go to SLEEP
            if (isStandby) {
                EnterSleepMode(); 
                
                // --- WAKE UP LOGIC ---
                // Code resumes here after waking up
                isStandby = 0;      // Go straight to "Enter Key"
                currentStep = 0;
                lastActivityTime = globalTimer;
                lastButtonState = 4; // Debounce Center button
                delay_ms(50);
                continue;
            }
            
            // CASE B: Active -> Go to STANDBY
            else {
                isStandby = 1;
                currentStep = 0;
                isBlinking = 0;
                timerActive = 0;
                lastActivityTime = globalTimer; // RESET TIMER for Stage 2
                
                SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                DrawString(10, 20, "STANDBY");
                DrawString(10, 35, "PRESS CENTER");
                SetRGBs(255, 0, 0); 
                
                lastButtonState = currentButton;
                delay_ms(50);
                continue;
            }
        }
        
        // ============================================
        // 2. STANDBY MODE LOGIC
        // ============================================
        if (isStandby) {
            if (currentButton == 4 && lastButtonState == -1) { // Wake on CENTER
                isStandby = 0;
                currentStep = 0;
                lastActivityTime = globalTimer; 
                
                SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                DrawString(10, 10, "ENTER KEY");
                SetRGBs(255, 0, 0); 
            }
            lastButtonState = currentButton;
            delay_ms(50);
            continue; // This 'continue' is now safe because we checked timer above
        }
        
        // ============================================
        // 3. KEY ENTRY MODE (Active)
        // ============================================
        if (timerActive) {
            unsigned long elapsedTime = globalTimer - timerStart;
            int secondsRemaining = 15 - (elapsedTime / 1000);
            
            if (secondsRemaining <= 0) {
                // Input Timeout
                timerActive = 0;
                currentStep = 0;
                isBlinking = 0;
                lastDisplayedTime = -1;
                
                SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                DrawString(10, 15, "TIME OUT");
                delay_ms(2000); 
                
                SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                DrawString(10, 10, "ENTER KEY");
                lastActivityTime = globalTimer; // Reset inactivity timer
                lastButtonState = currentButton;
                delay_ms(50);
                continue; 
            }
            
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
        // 4. BUTTON PRESS LOGIC
        // ============================================
        if (currentButton != -1 && lastButtonState == -1) {
            
            // Any button press resets the inactivity timer
            lastActivityTime = globalTimer; 
            
            if (currentStep == 0 && !timerActive) {
                timerActive = 1;
                timerStart = globalTimer;
            }
            
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
                    failedAttempts = 0;
                    DrawString(10, 20, "UNLOCKED");
                    SetRGBs(0, 255, 0); 
                    delay_ms(1000);
                    
                    ShowMenu(); // Enter Menu
                    
                    // Reset after Menu exit
                    isStandby = 1; 
                    lastActivityTime = globalTimer; // Reset timer for standby
                    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                    DrawString(10, 20, "STANDBY");
                    DrawString(10, 35, "PRESS CENTER");
                    SetRGBs(255, 0, 0); 
                    
                } else {
                    DrawString(10, 15, "KEY WRONG");
                    SetRGBs(255, 0, 0); 
                    
                    failedAttempts++;
                    if (failedAttempts >= 3) {
                        HandleLockout(); 
                    } else {
                        delay_ms(2000); 
                    }
                    
                    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                    DrawString(10, 10, "ENTER KEY");
                    lastActivityTime = globalTimer;
                }
                currentStep = 0;
                isBlinking = 0;
            }
        }

        if (currentButton == -1 && lastButtonState != -1) {
            if (isLocked) SetRGBs(255, 0, 0); 
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
    int needsRedraw = 1; 
    
    // Local Timer for Menu Timeout
    unsigned long menuTimer = globalTimer;

    while(GetPressedButton() != -1) ReadCTMU();

    while(inMenu) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        // --- TIMEOUT CHECK ---
        if (globalTimer - menuTimer > 30000) {
            return; // Exit to Main -> Main puts us in Standby
        }
        
        if (needsRedraw) {
            SetColor(BLACK); ClearDevice(); SetColor(WHITE);
            DrawString(10, 5, "MAIN MENU");
            if (selection == 0) DrawString(10, 25, "> LOCK SYSTEM");
            else DrawString(10, 25, "  LOCK SYSTEM");
            if (selection == 1) DrawString(10, 40, "> CHANGE KEY");
            else DrawString(10, 40, "  CHANGE KEY");
            needsRedraw = 0; 
        }
        
        if (btn != -1 && btn != lastBtn) {
            menuTimer = globalTimer; // Reset Timer on Input
            
            if (btn == 0 || btn == 2) { 
                selection = !selection; 
                needsRedraw = 1; 
            } 
            else if (btn == 4 || btn == 1) { 
                if (selection == 0) inMenu = 0; 
                else {
                    ChangePassword(); 
                    needsRedraw = 1; 
                    menuTimer = globalTimer; // Reset when returning
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
    int needsRedraw = 1; 
    
    unsigned long subMenuTimer = globalTimer; // Local Timer
    
    while(GetPressedButton() != -1) ReadCTMU();
    
    // --- STEP 1: CHOOSE LENGTH ---
    while(choosing) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        // Timeout Check
        if (globalTimer - subMenuTimer > 30000) return;
        
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
            subMenuTimer = globalTimer; // Reset Timer
            if (btn == 0 || btn == 2) { 
                newLen = (newLen == 4) ? 8 : 4;
                needsRedraw = 1; 
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
    subMenuTimer = globalTimer; // Reset Timer for Step 2
    
    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
    DrawString(10, 10, "ENTER NEW:");
    
    while(GetPressedButton() != -1) ReadCTMU();
    lastBtn = -1;
    
    while(count < newLen) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        // Timeout Check
        if (globalTimer - subMenuTimer > 30000) return;
        
        if (btn != -1 && btn != lastBtn) {
            subMenuTimer = globalTimer; // Reset Timer
            if (btn == 4) continue; 
            
            temp[count] = btn;
            count++;
            
            SetColor(WHITE);
            for(int i=0; i<count; i++) DrawChar(10 + (i*6), 30, '*');
            SetRGBs(0, 0, 255); delay_ms(100); SetRGBs(0, 255, 0);
        }
        lastBtn = btn;
    }
    
    // Save Logic (No changes here)
    codeLength = newLen;
    for(int i=0; i<8; i++) {
        if(i < newLen) secretCode[i] = temp[i];
        else secretCode[i] = 0;
    }
    SaveCodeToFlash(secretCode, codeLength);
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
// Interrupt Service Routine for Timer 1
// This is required to wake the CPU from "Idle" mode
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
    IFS0bits.T1IF = 0; // Clear the interrupt flag
}

void EnterSleepMode(void) {
    // 1. Turn OFF High Power Components
    SetColor(BLACK); ClearDevice(); // OLED Off
    SetRGBs(0,0,0);                 // LEDs Off
    
    // 2. Configure Timer 1 as our "Wake-up Alarm"
    // We want it to interrupt every ~250ms
    T1CON = 0;              // Stop Timer
    TMR1 = 0;               // Clear Count
    T1CONbits.TCKPS = 0b11; // 1:256 Prescale
    PR1 = 0x4000;           // ~260ms Wakeup Interval
    
    IFS0bits.T1IF = 0;      // Clear Flag
    IEC0bits.T1IE = 1;      // Enable Interrupts (Crucial for Idle wake-up)
    T1CONbits.TON = 1;      // Start Timer
    
    // 3. Enter Low Power Loop
    while(1) {
        // [REAL POWER SAVE] 
        // This instruction stops the CPU clock!
        // The code HALTS here until Timer 1 fires an interrupt (every 250ms).
        __builtin_pwrsav(1); // 1 = Idle Mode
        
        // --- CPU WAKES UP HERE ---
        // (ISR has run and cleared the flag)
        
        // Check for User Input
        ReadCTMU(); 
        
        // If Center Button (4) is pressed, fully wake up
        if (GetPressedButton() == 4) {
            break; 
        }
    }
    
    // 4. Cleanup & Restore
    T1CONbits.TON = 0;      // Stop Timer
    IEC0bits.T1IE = 0;      // Disable Interrupts (So delay_ms works again)
    
    // Restore Screen
    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
    DrawString(10, 10, "ENTER KEY");
    SetRGBs(255, 0, 0);     // Red LED
}