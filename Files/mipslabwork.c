/* mipslabwork.c
   Original file written 2015 by F Lundevall
   Modified by Thony Price for course IS1500, KTH 
   Last revision: 2016-12-07          

*/

/***** Include and define *****/

#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */
#define TMR2PERIOD ((80000000 / 256) / 10)  // Set time-out-period to 10 ms
#if TMR2PERIOD > 0xffff                     // Fail-safe TMR2PERIOD size
#error "Timer period is too big."
#endif

int timeoutcount = 0; // Counter it interrupt timeouts 
int plantMode = 2;    // Operate between plants -> Preset Flower mode
int leds;             // Initial value of LEDs
int inAirVal = 745;   // Value of sensor measurement in air
int waterVal = 200;   // Value of sensor measurement in water

volatile int* address;                      // Pointer to value in register
                                            
char* kaktus = "> Kaktus mode";             // String for display
char* flower = "> Flower mode";             // String for display
char* no_val = "> No measurement";          // String for display
char* tooDry = "> I'm thirsty!";            // String for display

/***** Verify and process sensor value *****/

void verifyValue(int value){
  if (value < waterVal){                    // Invalid -> Prompt no measurement
    PORTE = 0;
    display_string(1, no_val);
  } if (value > waterVal){                  // Valid -> Convert val to leds
    PORTE = getLeds(value, inAirVal, waterVal, plantMode);
  }                                         // getLeds located in time4io file
}

/***** Interrupt Service Routine *****/

void user_isr( void ) {

  address   = (volatile int*) 0xbf809070;   // Reg. ADC1BUF0 (A1 Read)
  // display_debug(address);                // Display analog input from A1 
  int value = *address;                     // Extract value from address
  char* strVal = itoaconv(value);           // Convert value to string
  display_string(3, strVal);                // Display current value
  verifyValue(value);                       // Handle value of sensor
  display_update();                     
  
  // Check which flag is up
  int t2_flag   = IFS(0) & 256;             // Get status of T2 flag  
  int int1_flag = IFS(0) & 128;             // Get status of INT1 flag
  if (t2_flag == 256){                      // If TMR flas is up
      IFS(0) ^= 256;                        // Clear TMR flag
      if (IFS(0) ^= 128){                   // If other flag raised too
          timeoutcount++;                   // Increase counter
          if (timeoutcount == 3){           
              timeoutcount = 0;         
              PORTE = 0;                    // This makes LEDs "blink"
      }                                     // When their value is low
    }
  }
  if (int1_flag == 128){                    // If SW flag is up    
      display_string(0, tooDry);            // Prompt plant's too dry
      if (getLeds(value, inAirVal, waterVal, plantMode) > 3){
        IFS(0) ^= 128;                      // Clear SW flag
        timeoutcount = 0;                   
        display_string(0, "~*Flowerpower*~");
      }
  }
}

/***** Initialize *****/

void labinit( void )
{ 
                                // Initialize ADC
  AD1CON1CLR= 0x00008000;       // Turn off the ADC before operating on it
  AD1PCFG   = 0x0000;           // Enable all analog inputs
  TRISASET  = 4;                // Set coresponding TRISbit (A2)

  AD1CHSSET = 0x00040000;       // Set A2 to positive analog input in MUX' for CH0SB and CH0SA 
  AD1CON1CLR= 0x00000700;       // Set output format as 16 bit integer
  AD1CON1SET= 0x000000e0;       // Chose auto-convert for SSRC
  // AD1CON3SET= 0x00000c00;       // Rate of auto-convert SAMC, 12 TAD <- Increase?
  AD1CON3SET= 0x00000300;       // Rate of auto-convert SAMC, 3 TAD 
  AD1CHSCLR = 0x00800000;       // Make sure scanning mode is off (read multiple pins)
  AD1CON2CLR= 0x0000003c;       // Set SMPI-bits to 0 -> All ADC convertions writes to BUF0
                                // Also sets convertions per interrupt to 1
  AD1CON2CLR= 0x00000001;       // Clr ALTS bit i.e. only one input source
  AD1CON3SET= 0x00008000;       // Set internal clock, ADRC, as RC ocilator clock <- Change?
  AD1CON3CLR= 0x00008000;       // Set internal clock, ADRC, as PBCLK
  AD1CON3SET= 0x00000002;       // Set ADCS to 2
  // AD1CON3SER= 0x0000000f;      // Set aquisiton period
  
  AD1CON1SET= 0x00008000;       // Turn on the ADC
  AD1CON1SET= 0x00000004;       // Enable auto sampling

                                // Inititalize timer
  T2CON   = 0x8070;             // Set prescale to 1:256
  PR2     = TMR2PERIOD;         // Initialize the timer
  TMR2    = 0;                  // Reset timer 2
  T2CONSET = 0x8000;            // Start the timer 
  
  IPC(2) = 4;                   // Interrupt priority control for TMR2
  IPC(1) = 28;                  // Set IPC1 to same priority
  IFS(0) = 0;                   // Reset all event flags 
  IEC(0) = 384;
  enable_interrupts();          // Call funtion in labwork.S

}

/***** Repetitively called from the main program *****/

void labwork( void ) {
  
  int btns  = getbtns();        // getbtns located in time4io
  if (btns == 4) {              // Depending on which button is pressed 
      plantMode = 1;            // user sets plantMode
      display_string(1, kaktus);
  }
  if (btns == 2) {
      plantMode = 2;
      display_string(1, flower);
  }
  
}
