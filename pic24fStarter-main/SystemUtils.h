#ifndef SYSTEMUTILS__H
#define	SYSTEMUTILS__H

#include <xc.h>
#include <stdint.h>
#include <libpic30.h>

// Global timer to track system uptime (in milliseconds)
extern volatile unsigned long globalTimer;

// Blocking delay function that also updates the global timer
void delay_ms(unsigned int milliseconds);

// Flash storage functions for persistent code storage
void SaveCodeToFlash(uint8_t* code, int length);
void LoadCodeFromFlash(uint8_t* code, int* length);

#endif