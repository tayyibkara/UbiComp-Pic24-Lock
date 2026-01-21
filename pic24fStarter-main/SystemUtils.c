#include "SystemUtils.h"

volatile unsigned long globalTimer = 0;

void delay_ms(unsigned int milliseconds) {
    T1CONbits.TCKPS = 0b11; // Prescale 1:256
    PR1 = 47; TMR1 = 0; 
    T1CONbits.TON = 1; 
    unsigned long count = 0; 
    while (count < milliseconds) {
        while (!IFS0bits.T1IF); 
        IFS0bits.T1IF = 0; 
        count++;
        globalTimer += 1; // Keep track of system time
    }
    T1CONbits.TON = 0; 
}
//#include "SystemUtils.h"
//
//volatile unsigned long globalTimer = 0;
//
//void delay_ms(unsigned int milliseconds) {
//    // Simple blocking delay calibrated for ~16 MIPS
//    unsigned long cycles = milliseconds * 3200; 
//    while(cycles > 0) {
//        Nop();
//        cycles--;
//    }
//}