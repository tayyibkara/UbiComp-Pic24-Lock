#ifndef SYSTEMUTILS__H
#define	SYSTEMUTILS__H

#include <xc.h>
#include <stdint.h>
#include <libpic30.h>

// --- DATABASE CONSTANTS ---
#define MAX_USERS 5
#define MAX_CODE_LEN 8

// --- THE USER STRUCTURE ---
typedef struct {
    uint8_t id;           
    uint8_t isActive;     
    uint8_t codeLength;   
    uint8_t code[MAX_CODE_LEN]; 
} User;

// Global timer to track system uptime (in milliseconds)
extern User userDB[MAX_USERS];
extern volatile unsigned long globalTimer;

// Blocking delay function that also updates the global timer
void delay_ms(unsigned int milliseconds);

// Flash storage functions for persistent code storage
//void SaveCodeToFlash(uint8_t* code, int length);
//void LoadCodeFromFlash(uint8_t* code, int* length);
// --- FLASH FUNCTIONS ---
void LoadUsersFromFlash(void);
void SaveUsersToFlash(void);
#endif