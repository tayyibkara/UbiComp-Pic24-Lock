#ifndef SYSTEMUTILS__H
#define	SYSTEMUTILS__H

#include <xc.h>

// Global timer to track system uptime (in milliseconds)
extern volatile unsigned long globalTimer;

// Blocking delay function that also updates the global timer
void delay_ms(unsigned int milliseconds);

#endif