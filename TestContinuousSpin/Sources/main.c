#include <hidef.h>      /* common defines and macros */
#include <string.h>
#include "derivative.h"      /* derivative-specific definitions */

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
  unsigned char rc;
  int i;
  
  rc = SCI1SR1; /* dummy read to clear flags */
  rc = SCI1DRL;
  MSDelay(20);
  rc = SCI1DRL;  
  
  for(i = 0; i < 25; i++){
    PORTB=0b00001010;   
    MSDelay(50);  
    PORTB=0b00000110;    
    MSDelay(50);  
    PORTB=0b00000101;    
    MSDelay(50);  
    PORTB=0b00001001;  
    MSDelay(50);
  }
  
  //while (*stop != '\0'){
  //  while (!(SCI1SR1 & 0x80));  // wait for output buffer empty
  //  SCI1DRL = *stop++;    //transmit back to signal finished scanning
  //}
  
  MSDelay(200);
  PORTB=0b00000000;
  MSDelay(200);
  
  for(i = 0; i < 25; i++){
    PORTB=0b00001001;  
    MSDelay(20);
    PORTB=0b00000101;    
    MSDelay(20);
    PORTB=0b00000110;
    MSDelay(20);
    PORTB=0b00001010;
    MSDelay(20);    
  }
  
  PORTB=0b00000000;  
}

void main(void){
  const char *stop = "stop";
  unsigned char rc;
  
  DDRB = 0x0F;      //PORTB0-PORTB3  as output 
  DDRP = 0x03;      //PORTP0 and PORTP1 as output for 12EN=1 and 34EN=1
    
  PORTB=0b00000000; // start with all off
  PTP=0b00000011;   //Turn on both 12EN and 34EN Enables for 754410 chip
  
  /*PWMPRCLK=0x5; 	//ClockA=Fbus/2**5=24MHz/32=250000 Hz
  	PWMCLK=0x00; 	  //ClockA for Chan 0 and 1
  	PWMPOL=0x03; 	  //high and low for polarity
  	PWMCAE=0; 		  //Left aligned
  	PWMCTL=0x00;		//8-bit chan and PWM freeze during wait  
  	PWMCNT0=0;		 //start the PWMCount with 0 value
  	PWMCNT1=0;
  	PWME=0x03; 	   //Enable PWM chan 0 and 1
  
  PWMPER0=100;	  //PWM_Period Freq. = ClockA/100= 250000 Hz/100=2500 Hz
  PWMDTY0=60;*/     //Duty Cycle=60% (60% of 100). Change this value and experiment with duty cycle. 
  
  SCI1BDL = 0x34;
  SCI1BDH = 0x00;   //set options for SCI communications and interrupts
  
  SCI1CR1 = 0x00;
  SCI1CR2 = 0x24;
  
  asm("cli");       //enable interrupt globally
  
  rc = SCI1SR1; /* dummy read to clear flags and TDRE */
  SCI1DRH = 0x0000; /* data write to clear TDRE */
  
  for(;;);
  
  //while (*stop != '\0'){
    //while (!(SCI1SR1 & 0x80));  /* wait for output buffer empty */
    //SCI1DRL = *stop++;    //transmit back to signal finished scanning
  //}
    
  //while(1);         //Wait for the interrupt forever
}
