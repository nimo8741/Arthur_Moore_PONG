/* 
 * File:   main.c
 * Author: Nick
 *
 * Created on November 18, 2017, 2:16 PM
 */

#include <p18cxxx.h>
#include <delays.h>
#include <p18f87k22.h>
#include "GLCDroutinesEasyPic.h"
#include "Pong_funcs.h"
#include <delays.h>
#include "adc.h"


#pragma config FOSC=HS1, PWRTEN=ON, BOREN=ON, BORV=2, PLLCFG=OFF
#pragma config WDTEN=OFF, CCP2MX=PORTC, XINST=OFF


/******************************************************************************
 * Function prototypes
 ******************************************************************************/
void Initial(void);         // Function to initialize hardware and interrupts
void HiPriISR(void);        // this is just here for completeness
void LoPriISR(void);

#pragma code highVector=0x08
void atHighVector(void)
{
 _asm GOTO HiPriISR _endasm
}
#pragma code

#pragma code lowVector=0x18
void atLowVector(void)
{
 _asm GOTO LoPriISR _endasm
}
#pragma code


void main() {
     Initial();                 // Initialize everything
     
     while(1){   // this is the loop for the game
         check_reset();
         check_reset();
         update_paddles();
         update_ball();
         
         
         //ADCTemperature();
         // check for reset request down here
         
     }
}



/******************************************************************************
 * Initial()
 *
 * This subroutine performs all initializations of variables and registers.
 * It enables TMR0 and sets CCP0 for compare if desired, and enables LoPri 
 * interrupts for both.
 ******************************************************************************/
void Initial() {
    
    unsigned char i;
    unsigned char j;
    unsigned char k;
   
    // Configure the IO ports
    TRISC  = 0b00000100;
    LATC = 0x00;
    TRISE = 0x00;
    LATE = 0x00;
    TRISEbits.TRISE3 = 1;    // start button 
    TRISEbits.TRISE4 = 1;    // make the "up" button for player 1 set to input
    TRISEbits.TRISE5 = 1;    // make the "down" button for player 1 set to input 
    TRISJbits.TRISJ0 = 1;    // make the "up" button for player 2 set to input
    TRISJbits.TRISJ1 = 1;    // make the "down" button for player 2 set to input
    TRISFbits.TRISF4 = 1;    // make the start reset button set to input
    
    
    GLCD_CS0_TRIS = 0;
    GLCD_CS1_TRIS = 0;
    GLCD_RS_TRIS = 0;
    GLCD_RW_TRIS = 0;
    GLCD_E_TRIS = 0;
    GLCD_RST_TRIS = 0;
    GLCD_DATA_TRIS = 0;
    
    
    
    InitGLCD();
    ClearGLCD();
    Initial_ball();
    
    // interrupt stuff
    RCONbits.IPEN = 1;              // Enable priority levels
    
    // Now set up Timer1, This will be the delay timer in between ball position updates
    PIR1bits.TMR1IF = 0;   // clear the interrupt flag just to be sure
    PIE1bits.TMR1IE = 1;   // enable interrupts for timer1
    IPR1bits.TMR1IP = 0;  // assign low priority to timer1 interrupts
    T1CONbits.T1CKPS0 = 1;
    T1CONbits.T1CKPS1 = 1;  // load a prescalar of 8 into the timer
    TMR1H = 0x00;
    TMR1L = 0x00;   // clear the registers, High and then Low
    INTCON = 0;    // clear INTCON
    INTCONbits.GIEL = 1;            // Enable low-priority interrupts to CPU
    INTCONbits.GIEH = 1;            // Enable all interrupts
    T1CONbits.TMR1ON = 1;   // turn on the timer
    
    
    // PLAYER 1 PADDLE TIMER
    T3CONbits.T3CKPS0 = 1;
    T3CONbits.T3CKPS1 = 1;   // load prescalar of 8
    TMR3H = 0;
    TMR3L = 0;
    PIE2bits.TMR3IE = 1;     // enable interrupts for timer3
    PIR2bits.TMR3IF = 0;     // clear the interrupt flag, just to be sure
    IPR2bits.TMR3IP = 0;     // assign low priority to timer3
    
    
    // PLAYER 2 PADDLE TIMER
    T5CONbits.T5CKPS0 = 1;
    T5CONbits.T5CKPS1 = 1;   // load prescalar of 8
    TMR5H = 0;
    TMR5L = 0;
    PIE5bits.TMR5IE = 1;     // enable interrupts for timer3
    PIR5bits.TMR5IF = 0;     // clear the interrupt flag, just to be sure
    IPR5bits.TMR5IP = 0;     // assign low priority to timer3
    
    
    // set up timer0
    T0CON = 0b00000101;     // this sets the timer with a prescalar of 64
    //INTCONbits.TMR0IE = 1;   // enable interrupts for timer0
    INTCONbits.TMR0IF = 0;   // make sure the interrupt flag is cleared
    INTCON2bits.TMR0IP = 0;   // assign low priority to timer0
    
    
    PIE5bits.TMR7IE = 0;   // make sure there are no interrupts associated with timer7
    T7CONbits.TMR7ON = 1;   // turn on timer7.  This will be used for generating the random number
    
    Initial_screen();
    Initial_ball();
    
    
}


/******************************************************************************
 * HiPriISR interrupt service routine
 *
 * Included to show form, does nothing
 ******************************************************************************/
#pragma interrupt HiPriISR
void HiPriISR() {
    
}	// Supports retfie FAST automatically

/******************************************************************************
 * LoPriISR interrupt service routine
 *
 * Calls the individual interrupt routines. It sits in a loop calling the required
 * handler functions until until TMR1IF and CCP1IF are clear.
 ******************************************************************************/
#pragma interruptlow LoPriISR 
void LoPriISR() {
    while(1) {
        if( PIR1bits.TMR1IF ) { //Check if ball movement timer flag is set
            TMR1handler();
            continue;
        }
        if ( PIR2bits.TMR3IF){  // check if player 1 paddle timer is set
            TMR3handler();
            continue;
        }
        if (PIR5bits.TMR5IF) {  // check if player 2 paddle timer flag is set
            TMR5handler();
            continue;
        }
        if (INTCONbits.TMR0IF){  // check if the starting timer has finished a second.
            TMR0handler();
            break;
        }
        
        break;
    }
}








