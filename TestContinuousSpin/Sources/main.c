#include <hidef.h>      /* common defines and macros */
#include <string.h>
#include "derivative.h"      /* derivative-specific definitions */

volatile unsigned int isr_flag = 0;

//millisecond delay for XTAL=8MHz, PLL=48MHz
//The HCS12 Serial Monitor is used to download and run the program.
//Serial Monitor uses PLL=48MHz

void MSDelay(unsigned int itime){
  unsigned int i; unsigned int j;
  for(i=0;i<itime;i++)
    for(j=0;j<4000;j++);    //1 msec. tested using Scope
}

#pragma CODE_SEG __NEAR_SEG NON_BANKED
void interrupt SCI1_ISR(void){
  unsigned char data;
  data = SCI1SR1; /* dummy read to clear flags */
  
  while (SCI1SR1 & 0x20){
    data = SCI1DRL;
  }
  
  isr_flag = 1;
}

void main(void){
  unsigned char rc;
  
  PWMPRCLK=0x04; //ClockA=Fbus/2**4=24MHz/16=1.5MHz	
	PWMSCLA=125; 	 //ClockSA=1.5MHz/2x125=6000 Hz
	PWMCLK=0b00010000; 	 //ClockSA for chan 4
	PWMPOL=0x10; 		     //high then low for polarity
	PWMCAE=0x0; 		     //left aligned
	PWMCTL=0x0;	         //8-bit chan, PWM during freeze and wait
	PWMPER4=120; 	 //PWM_Freq=ClockSA/120=6000Hz/100=50Hz.
	PWMCNT4=10;		 //clear initial counter.
  
  SCI1BDL = 0x34;
  SCI1BDH = 0x00;   //set options for SCI communications and interrupts
  
  SCI1CR1 = 0x00;
  SCI1CR2 = 0x2C;
  
  asm("cli");       //enable interrupt globally
  
  rc = SCI1SR1; /* dummy read to clear flags and TDRE */
  SCI1DRH = 0x0000; /* data write to clear TDRE */
  SCI1DRL = 0x0000; /* data write to clear TDRE */
  
  while(1){
    if(isr_flag){      
      PWMDTY4=7.5;
      PWME=0x10; 	   //Enable chan 4 PWM
      MSDelay(18000);
      
      while (!(SCI1SR1 & 0x80));  // wait for output buffer empty
      SCI1DRL = 'S';    //transmit back to signal finished scanning
      
     	PWMDTY4=8; 
      MSDelay(4600);
      
      PWME=0x00; 	   //Enable chan 4 PWM
      isr_flag = 0; 
    }
  }
  
  //while (*stop != '\0'){
    //while (!(SCI1SR1 & 0x80));  /* wait for output buffer empty */
    //SCI1DRL = *stop++;    //transmit back to signal finished scanning
  //}
    
  //while(1);         //Wait for the interrupt forever
}
