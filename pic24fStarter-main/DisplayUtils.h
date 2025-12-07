#ifndef DISPLAYUTILS__H
#define	DISPLAYUTILS__H

#include <xc.h>
#include "SH1101A.h"

void DrawChar(uint8_t x, uint8_t y, char c);
void DrawString(uint8_t x, uint8_t y, char* str);
void DisplayPasswordProgress(int stepsEntered);
void DisplayTimer(int secondsRemaining);
void ShowCalibrationScreen(void);

#endif