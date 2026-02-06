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
int loggedInUserIndex = -1; // Stores the index (0-4) of the current user
// --- Prototypes ---
void ShowMenu(void);
void ChangePassword(void);
void HandleLockout(void);
void EnterSleepMode(void);
void UserAction_Add(int userId);
void UserAction_EditDelete(int userIndex);
int main(void) {
    // 1. Initialization
    INIT_CLOCK(); 
    CTMUInit(); 
    RGBMapColorPins();
    RGBTurnOnLED();
    ResetDevice();
    
// --- STEP 2: LOAD FROM FLASH ---
    LoadUsersFromFlash(); // <--- CHANGE THIS LINE
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
            
            // Any button wake-up / activity reset
            lastActivityTime = globalTimer; 
            
            if (currentStep == 0 && !timerActive) {
                timerActive = 1;
                timerStart = globalTimer;
            }

            // --- CASE A: CENTER BUTTON (SUBMIT/VERIFY) ---
            if (currentButton == 4) {
                // User pressed Center ("5") -> This acts as "ENTER"
                
                timerActive = 0; 
                int foundUserIndex = -1;

                // Scan Database
                for(int i = 0; i < MAX_USERS; i++) {
                    if(userDB[i].isActive) {
                        // 1. Check Exact Length Match (Fixes 4 vs 8 conflict)
                        // If you typed 4 digits, it only checks 4-digit users.
                        // If you typed 8 digits, it only checks 8-digit users.
                        if(currentStep != userDB[i].codeLength) continue;

                        // 2. Check Code Digits
                        int match = 1;
                        for(int j = 0; j < currentStep; j++) {
                            if(enteredCode[j] != userDB[i].code[j]) { 
                                match = 0; 
                                break; 
                            }
                        }
                        if(match) {
                            foundUserIndex = i;
                            break; 
                        }
                    }
                }
                
                SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                
                if (foundUserIndex != -1) {
                    // --- SUCCESS ---
                    loggedInUserIndex = foundUserIndex; 
                    failedAttempts = 0;
                    
                    char msg[16];
                    sprintf(msg, "USER %d", userDB[foundUserIndex].id);
                    DrawString(10, 10, "WELCOME:");
                    DrawString(10, 25, msg);
                    SetRGBs(0, 255, 0); 
                    delay_ms(2000);
                    
                    ShowMenu(); // Enter Menu
                    
                    // Reset to Standby
                    isStandby = 1; 
                    lastActivityTime = globalTimer; 
                    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                    DrawString(10, 20, "STANDBY");
                    DrawString(10, 35, "PRESS CENTER");
                    SetRGBs(255, 0, 0); 
                    
                } else {
                    // --- FAILURE ---
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
            // --- CASE B: DIRECTION BUTTONS (ADD DIGIT) ---
            else {
                SetRGBs(0, 0, 255); // Blue flash on keypress
                
                // FIX: Allow typing up to 8 digits (or more if you change MAX_CODE_LEN)
                // We no longer rely on 'codeLength' variable here.
                if (currentStep < 8) { 
                    enteredCode[currentStep] = currentButton;
                    currentStep++; 
                    DisplayPasswordProgress(currentStep);
                    blinkingSteps = currentStep;
                    isBlinking = 1;
                    blinkStart = globalTimer;
                }
            }
            
            // Keep your existing debounce
            delay_ms(250); 
        }

        if (currentButton == -1 && lastButtonState != -1) {
            if (isLocked) SetRGBs(255, 0, 0); 
        }

        lastButtonState = currentButton;
        delay_ms(50); 
    }
    return 0;
}

void ShowMenu(void) {
    int selection = 0; 
    int inMenu = 1;
    int lastBtn = -1;
    int needsRedraw = 1; 
    unsigned long menuTimer = globalTimer;

    while(GetPressedButton() != -1) ReadCTMU();

    while(inMenu) {
        ReadCTMU();
        int btn = GetPressedButton();
        if (globalTimer - menuTimer > 30000) return; 
        
        if (needsRedraw) {
            SetColor(BLACK); ClearDevice(); SetColor(WHITE);
            DrawString(10, 5, "MAIN MENU");
            
            // OPTION 0: LOCK SYSTEM (Everyone sees this)
            if (selection == 0) DrawString(10, 25, "> LOCK SYSTEM");
            else DrawString(10, 25, "  LOCK SYSTEM");
            
            // OPTION 1: VARIES BY USER
            if (userDB[loggedInUserIndex].id == 1) {
                // ADMIN (User 1) SEES:
                if (selection == 1) DrawString(10, 40, "> MANAGE USERS");
                else DrawString(10, 40, "  MANAGE USERS");
            } else {
                // STANDARD USER SEES:
                if (selection == 1) DrawString(10, 40, "> CHANGE MY KEY");
                else DrawString(10, 40, "  CHANGE MY KEY");
            }
            needsRedraw = 0; 
        }
        
        if (btn != -1 && btn != lastBtn) {
            menuTimer = globalTimer; 
            if (btn == 0 || btn == 2) { 
                selection = !selection; 
                needsRedraw = 1; 
            } 
            else if (btn == 4 || btn == 1) { // Select
                if (selection == 0) {
                    inMenu = 0; // Lock System
                } 
                else {
                    // SELECTION 1 LOGIC
                    if (userDB[loggedInUserIndex].id == 1) {
                        // Admin -> Go to Full List
                        ManageUsers();
                    } else {
     // Standard User -> Go to Edit Logic
                        // We reuse "Add" because it handles entering a new code perfectly
                        UserAction_Add(userDB[loggedInUserIndex].id); 
                        
                        SaveUsersToFlash(); //
                    }
                    needsRedraw = 1; 
                    menuTimer = globalTimer; 
                }
            }
            delay_ms(150);
        }
        lastBtn = btn;
        delay_ms(50);
    }
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


void ManageUsers(void) {
    int selectedUser = 0; // Index 0 to 4 (User 1 to 5)
    int inSubMenu = 1;
    int lastBtn = -1;
    int needsRedraw = 1;
    unsigned long subTimer = globalTimer;
    char buffer[20];

    while(GetPressedButton() != -1) ReadCTMU();

    while(inSubMenu) {
        ReadCTMU();
        int btn = GetPressedButton();

        // Timeout
        if (globalTimer - subTimer > 30000) return;

        if (needsRedraw) {
            SetColor(BLACK); ClearDevice(); SetColor(WHITE);
            DrawString(10, 5, "USER LIST");

            // --- FIX: CALCULATE ID DYNAMICALLY ---
            // Use (selectedUser + 1) instead of .id to ensure it's always 1-5
            sprintf(buffer, "< USER %02d >", selectedUser + 1); 
            DrawString(10, 25, buffer);

            if (userDB[selectedUser].isActive) {
                DrawString(10, 40, "STATUS: ACTIVE");
            } else {
                DrawString(10, 40, "STATUS: EMPTY");
            }

            needsRedraw = 0;
        }

        if (btn != -1 && btn != lastBtn) {
            subTimer = globalTimer;

            if (btn == 0) { // UP -> Next
                selectedUser++;
                if (selectedUser >= MAX_USERS) selectedUser = 0;
                needsRedraw = 1;
            } 
            else if (btn == 2) { // DOWN -> Prev
                selectedUser--;
                if (selectedUser < 0) selectedUser = MAX_USERS - 1;
                needsRedraw = 1;
            }
            else if (btn == 3) { // LEFT -> Back
                inSubMenu = 0;
            }
            else if (btn == 4) { // CENTER -> Select User
                // Check if Empty or Active
                if (userDB[selectedUser].isActive) {
                    // Active -> Go to Edit/Delete
                    UserAction_EditDelete(selectedUser);
                } else {
                    // Empty -> Go to Add New User
                    // FIX: Pass (selectedUser + 1) as the ID
                    UserAction_Add(selectedUser + 1); 
                }
                needsRedraw = 1; // Redraw list when coming back
                SaveUsersToFlash(); // Save any changes immediately
            }
            
            delay_ms(150);
        }
        lastBtn = btn;
        delay_ms(100);
    }
}
// --- LOGIC FOR ADDING A NEW USER ---
// Used for both creating new users and editing existing ones

// [REPLACE UserAction_Add WITH THIS FIXED VERSION]
void UserAction_Add(int userId) {
    int newLen = 4;
    int choosing = 1;
    int lastBtn = -1;
    int needsRedraw = 1; 
    
    // 1. Choose Length
    while(GetPressedButton() != -1) ReadCTMU();
    
    while(choosing) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        if (needsRedraw) {
            SetColor(BLACK); ClearDevice(); SetColor(WHITE);
            char title[20];
            sprintf(title, "NEW USER %02d", userId);
            DrawString(10, 5, title);
            
            if (newLen == 4) DrawString(10, 25, "> 4 DIGITS");
            else DrawString(10, 25, "  4 DIGITS");
            
            if (newLen == 8) DrawString(10, 40, "> 8 DIGITS");
            else DrawString(10, 40, "  8 DIGITS");
            needsRedraw = 0;
        }
        
        if (btn != -1 && btn != lastBtn) {
            if (btn == 0 || btn == 2) { 
                newLen = (newLen == 4) ? 8 : 4; 
                needsRedraw = 1; 
            }
            else if (btn == 4 || btn == 1) choosing = 0;
            
            // DEBOUNCE HERE (Only when button pressed)
            delay_ms(250); 
        }
        lastBtn = btn;
        delay_ms(50); // Fast polling when idle
    }
    
    // 2. Enter Code
    int count = 0;
    uint8_t temp[8] = {0}; // Initialize to zeros
    
    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
    DrawString(10, 10, "ENTER CODE:");
    
    while(GetPressedButton() != -1) ReadCTMU();
    lastBtn = -1;
    
    while(count < newLen) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        if (btn != -1 && btn != lastBtn) {
            if (btn == 4) continue; // Skip Center
            
            temp[count] = btn;
            count++;
            
            SetColor(WHITE);
            for(int i=0; i<count; i++) DrawChar(10 + (i*6), 30, '*');
            SetRGBs(0, 0, 255); delay_ms(100); SetRGBs(0, 255, 0);
            
            // [CRITICAL FIX]
            // Delay ONLY happens after a valid press to stop bouncing
            delay_ms(250); 
        }
        
        lastBtn = btn;
        delay_ms(50); // Keep system responsive (check 20 times/sec)
    }
    
    // 3. Save to RAM
    int idx = userId - 1; 
    userDB[idx].id = userId; 
    userDB[idx].isActive = 1;
    userDB[idx].codeLength = newLen;
    for(int i=0; i<8; i++) userDB[idx].code[i] = temp[i];
    
    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
    DrawString(10, 25, "USER ADDED!");
    delay_ms(1500);
}
// --- LOGIC FOR EDITING (CHANGE PASS) OR DELETING ---
void UserAction_EditDelete(int userIndex) {
    int selection = 0; // 0=EDIT, 1=DELETE
    int choosing = 1;
    int lastBtn = -1;
    int needsRedraw = 1;
    
    while(GetPressedButton() != -1) ReadCTMU();
    
    while(choosing) {
        ReadCTMU();
        int btn = GetPressedButton();
        
        if (needsRedraw) {
            SetColor(BLACK); ClearDevice(); SetColor(WHITE);
            char title[20];
            
            // FIX: Use (userIndex + 1) to calculate ID
            sprintf(title, "USER %02d", userIndex + 1); 
            DrawString(10, 5, title);
            
            if (selection == 0) DrawString(10, 25, "> EDIT KEY");
            else DrawString(10, 25, "  EDIT KEY");
            
            if (selection == 1) DrawString(10, 40, "> DELETE USER");
            else DrawString(10, 40, "  DELETE USER");
            needsRedraw = 0;
        }
        
        if (btn != -1 && btn != lastBtn) {
            if (btn == 0 || btn == 2) { // Up/Down
                selection = !selection; 
                needsRedraw = 1; 
            }
            else if (btn == 4 || btn == 1) { // Select
                if (selection == 0) {
                    // EDIT -> Call Add logic to overwrite
                    // FIX: Pass correct ID
                    UserAction_Add(userIndex + 1); 
                } else {
                    // DELETE -> Mark as empty
                    userDB[userIndex].isActive = 0;
                    SetColor(BLACK); ClearDevice(); SetColor(WHITE);
                    DrawString(10, 25, "DELETED!");
                    delay_ms(1500);
                }
                choosing = 0;
            }
            else if (btn == 3) choosing = 0; // Left = Back
            delay_ms(150);
        }
        lastBtn = btn;
        delay_ms(100);
    }
}