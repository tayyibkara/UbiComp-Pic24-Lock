#include "SystemUtils.h"
#include <string.h> // Needed for memcpy
volatile unsigned long globalTimer = 0;
User userDB[MAX_USERS]; // The actual database in RAM

//void delay_ms(unsigned int milliseconds) {
//    T1CONbits.TCKPS = 0b11; // Prescale 1:256
//    PR1 = 47; TMR1 = 0; 
//    T1CONbits.TON = 1; 
//    unsigned long count = 0; 
//    while (count < milliseconds) {
//        while (!IFS0bits.T1IF); 
//        IFS0bits.T1IF = 0; 
//        count++;
//        globalTimer += 1; // Keep track of system time
//    }
//    T1CONbits.TON = 0; 
//}
void delay_ms(unsigned int milliseconds) {
    T1CONbits.TCKPS = 0b11; // Prescale 1:256
    PR1 = 47; TMR1 = 0; 
    T1CONbits.TON = 1; 
    unsigned long count = 0; 
    while (count < milliseconds) {
        while (!IFS0bits.T1IF); 
        IFS0bits.T1IF = 0; 
        count++;
        globalTimer += 1; 
    }
    T1CONbits.TON = 0; 
}
void InitDummyDatabase(void) {
    // USER 1: Code is UP, UP, UP, UP (0, 0, 0, 0)
    userDB[0].id = 1;
    userDB[0].isActive = 1;
    userDB[0].codeLength = 4;
    userDB[0].code[0]=0; userDB[0].code[1]=0; userDB[0].code[2]=0; userDB[0].code[3]=0;
    
    // USER 2: Code is DOWN, DOWN, DOWN, DOWN (2, 2, 2, 2)
    userDB[1].id = 2;
    userDB[1].isActive = 1;
    userDB[1].codeLength = 4;
    userDB[1].code[0]=2; userDB[1].code[1]=2; userDB[1].code[2]=2; userDB[1].code[3]=2;

    // Others are empty
    for(int i=2; i<MAX_USERS; i++) {
        userDB[i].id = i + 1;
        userDB[i].isActive = 0; // Empty
        userDB[i].codeLength = 4;
    }
}
// ============================================
// FLASH STORAGE FUNCTIONS (Persistent Storage)
// ============================================

// Reserved Flash memory area for storing the code
// This is a page-aligned section of Flash memory we use to store data
// The compiler will not put program code here
const uint16_t __attribute__((space(prog), aligned(1024))) code_storage[512] = {0xFFFF};
const uint16_t __attribute__((space(prog), aligned(1024))) user_storage[512] = {0xFFFF};
// Save code to Flash memory
// Format: 
//   Address 0: code length (4 or 8)
//   Address 1-8: code digits
void SaveCodeToFlash(uint8_t* code, int length) {
    // Ensure length is valid
    uint8_t len = (uint8_t)length;
    if(len != 4 && len != 8) {
        len = 4;
    }
    
    // Create a buffer to hold the data we want to write
    uint16_t flash_data[9];
    flash_data[0] = (uint16_t)len;  // First word: length
    
    // Pack the 8 code bytes into the remaining words
    for(int i = 0; i < 8; i++) {
        flash_data[i + 1] = (uint16_t)code[i];
    }
    
    // Get a pointer to our flash storage area
    _prog_addressT prog_addr;
    _init_prog_address(prog_addr, code_storage);
    
    // Erase the entire page first (required before writing to Flash)
    _erase_flash(prog_addr);
    
    // Wait a bit for erase to complete
    delay_ms(5);
    
    // Write the data back to Flash
    // We write the length and all 8 code bytes
    for(int i = 0; i < 9; i++) {
        _write_flash_word16(prog_addr, flash_data[i]);
        prog_addr += 2;  // Move to next word
    }
    
    // Wait for write to complete
    delay_ms(5);
}

// Load code from Flash memory
void LoadCodeFromFlash(uint8_t* code, int* length) {
    // Create a pointer to our flash storage area
    _prog_addressT prog_addr;
    _init_prog_address(prog_addr, code_storage);
    
    // Read the length (first word)
    uint16_t stored_length = 0;
    _memcpy_p2d16(&stored_length, prog_addr, sizeof(uint16_t));
    
    // Check if Flash is empty (0xFFFF means uninitialized)
    if(stored_length == 0xFFFF) {
        // Flash is empty - use defaults
        *length = 4;
        code[0] = 3;  // UP
        code[1] = 0;  // LEFT
        code[2] = 1;  // RIGHT
        code[3] = 2;  // DOWN
        code[4] = 0;
        code[5] = 0;
        code[6] = 0;
        code[7] = 0;
        
        // Save defaults to Flash for next boot
        SaveCodeToFlash(code, *length);
        return;
    }
    
    // Sanity check: length must be 4 or 8
    if(stored_length != 4 && stored_length != 8) {
        // Invalid - use defaults
        *length = 4;
        code[0] = 3;
        code[1] = 0;
        code[2] = 1;
        code[3] = 2;
        code[4] = 0;
        code[5] = 0;
        code[6] = 0;
        code[7] = 0;
        return;
    }
    
    // Valid length found - load the code
    *length = (int)stored_length;
    
    // Read the code bytes (starting from the second word)
    prog_addr += 2;  // Skip the length word
    for(int i = 0; i < 8; i++) {
        uint16_t code_byte = 0;
        _memcpy_p2d16(&code_byte, prog_addr, sizeof(uint16_t));
        code[i] = (uint8_t)(code_byte & 0xFF);
        prog_addr += 2;  // Move to next word
    }
}
// --- FLASH STORAGE LOGIC ---

void SaveUsersToFlash(void) {
    _prog_addressT prog_addr;
    _init_prog_address(prog_addr, user_storage);
    
    // 1. Erase Page
    _erase_flash(prog_addr);
    delay_ms(5);
    
    // 2. Write Data
    // We treat the struct array as a raw buffer of 16-bit words
    uint16_t *dataPtr = (uint16_t*)userDB;
    int totalWords = sizeof(userDB) / 2;
    
    for(int i = 0; i < totalWords; i++) {
        _write_flash_word16(prog_addr, dataPtr[i]);
        prog_addr += 2;
    }
    delay_ms(5);
}

void LoadUsersFromFlash(void) {
    _prog_addressT prog_addr;
    _init_prog_address(prog_addr, user_storage);
    
    // Check if Flash is empty (First word 0xFFFF)
    uint16_t check = 0;
    _memcpy_p2d16(&check, prog_addr, 2);
    
    if (check == 0xFFFF) {
        // --- EMPTY FLASH: INIT DEFAULT USER ---
        memset(userDB, 0, sizeof(userDB)); // Clear RAM
        
        // Create Default Admin: User 1, Code 3012 (Up, Left, Right, Down)
        userDB[0].id = 1;
        userDB[0].isActive = 1;
        userDB[0].codeLength = 4;
        userDB[0].code[0]=3; userDB[0].code[1]=0; userDB[0].code[2]=1; userDB[0].code[3]=2;
        
        SaveUsersToFlash(); // Save this default immediately
    } 
    else {
        // --- LOAD DATA ---
        _memcpy_p2d16(userDB, prog_addr, sizeof(userDB));
    }
}