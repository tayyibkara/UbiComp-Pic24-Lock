#ifndef DISPLAYUTILS__H
#define	DISPLAYUTILS__H

#include <xc.h>
#include <string.h>
#include "SH1101A.h" // Needs low-level pixel functions

// Draws a single character at (x,y)
void DrawChar(uint8_t x, uint8_t y, char c);

// Draws a string starting at (x,y)
void DrawString(uint8_t x, uint8_t y, char* str);

// Visualizes entered password steps (e.g. "***")
void DisplayPasswordProgress(int stepsEntered);

// Displays the countdown timer
void DisplayTimer(int secondsRemaining);

// Shows the "CALIBRATING" screen sequence
void ShowCalibrationScreen(void);

#endif