/* Author: kvl@eti.uni-siegen.de
 * Created on July 27, 2025, 7:24 PM
 * Read out the capacitive touch sense pads for the PIC24F Starter Kit
 * CTMU (Charge Time Measurement Unit) uses capacitive touch switches (pads
 * on the PCB), to measure the relative capacitance of switches by AD converter
 */
#include "TouchSense.h"

// CTMU Constants (Manually defined to fix compilation errors)
#define CTMU_OFF                        0x0000
#define CTMU_CONTINUE_IN_IDLE           0x0000
#define CTMU_EDGE_DELAY_DISABLED        0x0000
#define CTMU_EDGES_BLOCKED              0x0000
#define CTMU_NO_EDGE_SEQUENCE           0x0000
#define CTMU_CURRENT_NOT_GROUNDED       0x0000
#define CTMU_TRIGGER_OUT_DISABLED       0x0000
#define CTMU_EDGE2_NEGATIVE             0x0000
#define CTMU_EDGE2_CTED1                0x0060
#define CTMU_EDGE2_CTED2                0x0040
#define CTMU_EDGE1_POSITIVE             0x0010
#define CTMU_EDGE1_CTED1                0x000C
#define CTMU_EDGE1_CTED2                0x0008
#define CTMU_EDGE_MASK                  0x0003
#define CTMU_EDGE2                      0x0002
#define CTMU_EDGE1                      0x0001

#define AVG_DELAY                       64 // Increased averaging for stability
#define CHARGE_TIME_COUNT               90 // Tuned charge time

uint8_t buttons[NUM_TOUCHPADS];
uint16_t _potADC;

// Global variables used in reading and averaging touch pad's values
uint16_t rawCTMU[NUM_TOUCHPADS];   
uint16_t average[NUM_TOUCHPADS];   
uint16_t trip   [NUM_TOUCHPADS];   
uint16_t hyst   [NUM_TOUCHPADS];   
uint8_t first;          
uint8_t buttonInd;      
uint16_t value, bigVal, smallAvg; 
uint16_t AvgIndex;

// Read potentiometer and store value in global variable
void ReadPotentiometer() {
    AD1CON1 = 0x00E4;   // Off, Auto sample start, auto-convert
    AD1CON2 = 0;        // AVdd, AVss, int every conversion, MUXA only
    AD1CON3 = 0x1F05;   // 31 Tad auto-sample, Tad = 5*Tcy
    AD1CHS  = 0x0;      // MUXA uses AN0
    AD1CSSL = 0;        // No scanned inputs
    AD1CON1bits.ADON = 1;        // turn on ADC module
    while(!AD1CON1bits.DONE);    // wait for conversion to complete
    _potADC = ADC1BUF0;
    AD1CON1bits.ADON = 0;        // turn off ADC module
}

// Routine to set up CTMU for capacitive touch sensing
void CTMUInit( void ) {
    TRISB    = 0x1F01;   // RB0, RB8, RB9, RB10, RB11, RB12 in tri-state
    AD1PCFGL &= ~0x1F01;
    
    // Manual Register Config to avoid Library Errors
    CTMUCON = CTMU_OFF | CTMU_CONTINUE_IN_IDLE | CTMU_EDGE_DELAY_DISABLED |
              CTMU_EDGES_BLOCKED | CTMU_NO_EDGE_SEQUENCE |
              CTMU_CURRENT_NOT_GROUNDED | CTMU_TRIGGER_OUT_DISABLED |
              CTMU_EDGE2_NEGATIVE | CTMU_EDGE2_CTED1 | CTMU_EDGE1_POSITIVE |
              CTMU_EDGE1_CTED1;  
              
    CTMUICONbits.IRNG = 2;   // 5.5uA
    CTMUICONbits.ITRIM = 0;  // 0%
    
    // Set up the ADC:
    AD1CON1            = 0x0000;
    AD1CHS             = STARTING_ADC_CHANNEL; 
    AD1CSSL            = 0x0000;
    AD1CON1bits.FORM   = 0x0;                  
    AD1CON3            = 0x0002;
    AD1CON2            = 0x0000;
    AD1CON1bits.ADON   = 1;           
    CTMUCONbits.CTMUEN = 1;           
    
    for (uint8_t i = 0; i < NUM_TOUCHPADS; i++ ) {
        trip[i] = TRIP_VALUE; hyst[i] = HYSTERESIS_VALUE;
    }
    buttonInd = 0;
    first = 160;  // Warm up delay
}

// Professor's "Zero Noise" Read Routine
void ReadCTMU() {
    uint16_t current_ipl;
    volatile unsigned int tempADch;
    tempADch            = AD1CHS;  
    AD1CON1             = 0x0000;  
    AD1CSSL             = 0x0000;
    AD1CON3             = 0x0002;
    AD1CON2             = 0x0000;
    AD1CON1bits.ADON    = 1;            
    
    for(uint8_t i=0; i<NUM_TOUCHPADS; i++) {
        AD1CHS = STARTING_ADC_CHANNEL + buttonInd; 
        IFS0bits.AD1IF = 0;  
        AD1CON1bits.DONE = 0;
        AD1CON1bits.SAMP = 1;        
        
        // Manual Delays for stability
        Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop();
        CTMUCONbits.IDISSEN = 1;  
        Nop(); Nop(); Nop(); Nop(); Nop();
        CTMUCONbits.IDISSEN = 0;
        Nop(); Nop(); Nop(); Nop(); Nop();
        IFS0bits.AD1IF = 0;
        AD1CON1bits.SAMP = 0;  
        while(!IFS0bits.AD1IF);  

        SET_AND_SAVE_CPU_IPL( current_ipl, 7 );  // Disable Interrupts for timing
        IFS0bits.AD1IF = 0;
        AD1CON1bits.SAMP = 1;      
        CTMUCONbits.EDG2STAT = 0;  
        CTMUCONbits.EDG1STAT = 1;   // Start Charge
        for (uint8_t j=0; j<CHARGE_TIME_COUNT; j++); // Precise Charge Time
        CTMUCONbits.EDG1STAT = 0;  // Stop Charge
        RESTORE_CPU_IPL( current_ipl );  // Enable Interrupts

        IFS0bits.AD1IF = 0;
        AD1CON1bits.SAMP = 0;
        while(!IFS0bits.AD1IF);  
        value = ADC1BUF0;
        
        IFS0bits.AD1IF = 0;    
        AD1CON1bits.SAMP = 1;  

        Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop();
        CTMUCONbits.IDISSEN = 1;        
        Nop(); Nop(); Nop(); Nop(); Nop(); 
        CTMUCONbits.IDISSEN = 0;        
        Nop(); Nop(); Nop(); Nop();
        IFS0bits.AD1IF = 0;
        AD1CON1bits.SAMP = 0;    
        while(!IFS0bits.AD1IF);  
        IFS0bits.AD1IF = 0;
        AD1CON1bits.DONE = 0;  
        
        bigVal = value  * 16; 
        
        smallAvg = average[buttonInd]/16;  
        rawCTMU[buttonInd] = bigVal;       
        if (first > 0) {  
            first--;
            average[buttonInd] = bigVal;
            if (++buttonInd == NUM_TOUCHPADS) buttonInd = 0;
            break;
        }
        
        // Hysteresis Logic
        if (bigVal > (average[buttonInd]-trip[buttonInd]+hyst[buttonInd])) {
            buttons[buttonInd] = 0;
        } else if (bigVal < (average[buttonInd] - trip[buttonInd])) {
            buttons[buttonInd] = 1;
        }
        
        // Noise Handling
        if (bigVal > average[buttonInd]) {  
            average[buttonInd] = bigVal;    
        }
        
        // Moving Average
        if(buttonInd == 0) {
            if (AvgIndex < AVG_DELAY) AvgIndex++; else AvgIndex = 0;
        }
        if (AvgIndex == AVG_DELAY) {  
            average[buttonInd] = average[buttonInd] + (value - smallAvg);
        }
        if (++buttonInd == NUM_TOUCHPADS) buttonInd = 0;  
    }
    ReadPotentiometer();  
    AD1CHS = tempADch;    
}

// =========================================================
// REQUIRED HELPER FOR MAIN.C
// (This was missing from the professor's code but is REQUIRED for your menu)
// =========================================================
int GetPressedButton(void) {
    // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT, 4=CENTER
    for(int i = 0; i < NUM_TOUCHPADS; i++) {
        if(buttons[i] == 1) return i;
    }
    return -1;
}