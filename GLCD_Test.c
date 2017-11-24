/****** ASEN 4/5519 Lab 6 ******************************************************
 * Author: Keegan Sotebeer
 * Date  : 10/25/16
 *
 * Description
 * "GLCD_Test"
 * The following occurs forever:
 *      RD4 blinks: 500ms +/- 10ms ON, then 500ms +/- 10ms OFF
 *
 *******************************************************************************
 *
 * Program hierarchy 
 *
 * Mainline
 *   Initial
 *
 * HiPriISR (included just to show structure)
 *
 * LoPriISR
 *   TMR0handler
 ******************************************************************************/

 
#include <p18cxxx.h>
#include <delays.h>
#include "GLCDroutinesEasyPic.h"

#pragma config FOSC=HS1, PWRTEN=ON, BOREN=ON, BORV=2, PLLCFG=OFF
#pragma config WDTEN=OFF, CCP2MX=PORTC, XINST=OFF

/******************************************************************************
 * Global variables
 ******************************************************************************/
unsigned int Alive_count = 0;
char H[]={0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00};

/******************************************************************************
 * Function prototypes
 ******************************************************************************/
void Initial(void);         // Function to initialize hardware and interrupts
void HiPriISR(void);
void LoPriISR(void);
void TMR0handler(void);     // Interrupt handler for TMR1

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

/******************************************************************************
 * main()
 ******************************************************************************/
void main() {
     Initial();                 // Initialize everything
      while(1) {
          if(PORTCbits.RC2){
              ClearGLCD();
              LATEbits.LATE0 = 1;
          }
          else{
              LATEbits.LATE0 = 0;
          }
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
    // Configure the IO ports
    unsigned char i;
    unsigned char j;
    TRISC  = 0b00000100;
    LATC = 0x00;
    TRISE = 0x00;
    LATE = 0x00;
    
    GLCD_CS0_TRIS = 0;
    GLCD_CS1_TRIS = 0;
    GLCD_RS_TRIS = 0;
    GLCD_RW_TRIS = 0;
    GLCD_E_TRIS = 0;
    GLCD_RST_TRIS = 0;
    GLCD_DATA_TRIS = 0;
    
    InitGLCD();
    ClearGLCD();
    for(i=0;i<128;i++){
        for(j=0;j<8;j++){
            SetCursor(i, j);
            if (j == 0){
                WriteData(0xAF);   // use to be 0x08
            }
            else{
                WriteData(0x00);
            }
        }
    }
    //i = 50;
    //j = 4;
    //SetCursor(i,j);
    //WriteData(0x10);
//    DisplayHexArray(H,sizeof(H));

    // Initializing TMR0
    T0CON = 0x04;                   // 00000100: 16 bit with 32 prescaler
    TMR0L = 0;                      // Clearing TMR0 registers
    TMR0H = 0;                      // Clear high register if used

    // Configuring Interrupts
    RCONbits.IPEN = 1;              // Enable priority levels
    INTCON2bits.TMR0IP = 0;         // Assign low priority to TMR0 interrupt

    INTCONbits.TMR0IE = 1;          // Enable TMR0 interrupts
    INTCONbits.GIEL = 1;            // Enable low-priority interrupts to CPU
    INTCONbits.GIEH = 1;            // Enable all interrupts

    T0CONbits.TMR0ON = 1;           // Turning on TMR0
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
        if( INTCONbits.TMR0IF ) { //Check if TMR0 interrupt flag is set
            TMR0handler();
            continue;
        }
        break;
    }
}


/******************************************************************************
 * TMR1handler interrupt service routine.
 *
 * Handles Alive LED Blinking via counter
 ******************************************************************************/
void TMR0handler() {
    LATCbits.LATC4 = ~LATCbits.LATC4; //Toggle RD4
    
    TMR0H = 0x0B;               // Reset TMR0H
    TMR0L = 0xDC;               // Reset TMR0L     

    INTCONbits.TMR0IF = 0;      //Clear flag and return to polling routine
}
