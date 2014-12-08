#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "types.h"
#include <stdio.h>

// Definitions

// Change this value to change the frequency of the output compare signal.
// The value is in Hz.
#define OC_FREQ_HZ    ((UINT16)10)

// Change this value to change the frequency of the output compare signal.
// The value is in Hz.
#define OC_FREQ_HZ    ((UINT16)10)

// Macro definitions for determining the TC1 value for the desired frequency
// in Hz (OC_FREQ_HZ). The formula is:
//
// TC1_VAL = ((Bus Clock Frequency / Prescaler value) / 2) / Desired Freq in Hz
//
// Where:
//        Bus Clock Frequency     = 2 MHz
//        Prescaler Value         = 2 (Effectively giving us a 1 MHz timer)
//        2 --> Since we want to toggle the output at half of the period
//        Desired Frequency in Hz = The value you put in OC_FREQ_HZ
//
#define BUS_CLK_FREQ  ((UINT32) 2000000)   
#define PRESCALE      ((UINT16)  2)         
#define TC1_VAL       ((UINT16)  (((BUS_CLK_FREQ / PRESCALE) / 2) / OC_FREQ_HZ))

// Value to set PWMDTY0 to for the different positions
int dutyPositions[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
//unsigned int leftRecipe = testRecipe;
//unsigned int rightRecipe = testRecipe;
int leftIndex = 0;
int leftPosition = 0;
char leftCommand = '\0';
volatile INT8 lsb = 0;

// Initializes SCI0 for 8N1, 9600 baud, polled I/O
// The value for the baud selection registers is determined
// using the formula:
//
// SCI0 Baud Rate = ( 2 MHz Bus Clock ) / ( 16 * SCI0BD[12:0] )
//--------------------------------------------------------------
void InitializeSerialPort(void)
{
    // Set baud rate to ~9600 (See above formula)
    SCI0BD = 13;          
    
    // 8N1 is default, so we don't have to touch SCI0CR1.
    // Enable the transmitter and receiver.
    SCI0CR2_TE = 1;
    SCI0CR2_RE = 1;
}

void Initialize(void)
{
  int i = 0;
  int bigNumber = 35000;
  // Set the timer prescaler to %128, and the Scale A register to %78 
  // since the bus clock is at 2 MHz,and we want the timer 
  // running at 50 Hz
  PWMCLK_PCLK0 = 1;
  PWMCLK_PCLK1 = 1;
  PWMPRCLK_PCKA0 = 1;
  PWMPRCLK_PCKA1 = 1;
  PWMPRCLK_PCKA2 = 1;
  PWMSCLA = 1;
  PWMPER0 = 160;
  PWMDTY0 = 3;
  PWMPER1 = 160;
  PWMDTY1 = 3;
  
  // Set the Polarity bit to one to ensure the beginning of the cycle is high
  PWMPOL = 3;
  
  // Enable Pulse Width Channel One
  PWME_PWME0 = 1;
  PWME_PWME1 = 1;
  //PostExecute();
}




void InitializeTimer(void)
{
  // Set the timer prescaler to %4, since the bus clock is at 2 MHz,
  // and we want the timer running at 500 KHz
  TSCR2_PR0 = 0;
  TSCR2_PR1 = 1;
  TSCR2_PR2 = 0;
  
  // Enable output compare on Channel 1
  TIOS_IOS1 = 1;
  
  // Set up output compare action to toggle Port T, bit 1
  TCTL2_OM1 = 0;
  TCTL2_OL1 = 1;

  
  // Set up timer compare value
  TC1 = TC1_VAL;
  
  // Clear the Output Compare Interrupt Flag (Channel 1) 
  TFLG1 = TFLG1_C1F_MASK;
  
  // Enable the output compare interrupt on Channel 1;
  TIE_C1I = 1;  
  
  //
  // Enable the timer
  // 
  TSCR1_TEN = 1;
   
  //
  // Enable interrupts via macro provided by hidef.h
  //
  EnableInterrupts;
}

// Output Compare Channel 1 Interrupt Service Routine
// Refreshes TC1 and clears the interrupt flag.
//          
// The first CODE_SEG pragma is needed to ensure that the ISR
// is placed in non-banked memory. The following CODE_SEG
// pragma returns to the default scheme. This is neccessary
// when non-ISR code follows. 
//
// The TRAP_PROC tells the compiler to implement an
// interrupt funcion. Alternitively, one could use
// the __interrupt keyword instead.
// 
// The following line must be added to the Project.prm
// file in order for this ISR to be placed in the correct
// location:
//		VECTOR ADDRESS 0xFFEC OC1_isr 
#pragma push
#pragma CODE_SEG __SHORT_SEG NON_BANKED
//--------------------------------------------------------------       
void interrupt 9 OC1_isr( void )
{     
	TFLG1 = TFLG1_C1F_MASK;
	
	lsb = PORTA;      
}
#pragma pop

// This function is called by printf in order to
// output data. Our implementation will use polled
// serial I/O on SCI0 to output the character.
//
// Remember to call InitializeSerialPort() before using printf!
//
// Parameters: character to output
//--------------------------------------------------------------       
void TERMIO_PutChar(INT8 ch)
{
    // Poll for the last transmit to be complete
    do
    {
      // Nothing  
    } while (SCI0SR1_TC == 0);
    
    // write the data to the output shift register
    SCI0DRL = ch;
}

void processInput(void){

	switch(leftCommand) {
        case 'l':
        case 'L': {
          
            if(leftPosition < 15){
            	PWMDTY0 = dutyPositions[leftPosition + 1];
            		
            	leftPosition++;
           	}
          	
        	break;     
        }
        case 'r':
        case 'R':{
        
        	if(leftPosition > 0){
           		PWMDTY0 = dutyPositions[leftPosition - 1];
                	
            	leftPosition--;
        	}
        	
            
            break;
        }
        
          
        default:{
        	break;
    	}
          
	};
		
	leftCommand = '\0';		
}


// Polls for a character on the serial port.
// Need to change
// Returns: Received character
//--------------------------------------------------------------       
void GetChar(void)
{ 
   //Poll for data
  //do
  //{
    // Nothing
  //} while(SCI0SR1_RDRF == 0);
  
 if(SCI0SR1_RDRF != 0){
 	if(SCI0DRL == 'x' || SCI0DRL == 'X'){
 		leftCommand = '\0';
 		(void)printf("\n\r>");
 	}
 	
 	else if(leftCommand == '\0'){
 		leftCommand = SCI0DRL;
 		TERMIO_PutChar(leftCommand);
 	}
 	
 	else if(SCI0DRL == '\r'){
 		DisableInterrupts;
 		processInput();
 		(void)printf("\r\n>");
 		EnableInterrupts;
 	}
 }
}

void main(void) {
  /* put your own code here */ 
  int ServoIndex=0;
  InitializeSerialPort();
  Initialize();
  InitializeTimer();
  
  DDRA = 0x00;

  while(1 == 1) {
  
        //char temp= GetChar();
        GetChar();
        
      
  } /* loop forever */
}
