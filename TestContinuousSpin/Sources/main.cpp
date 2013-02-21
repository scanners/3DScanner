#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "vectors12.h"

#define INTERRUPT __attribute__((interrupt))

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
  const char *stop = "stop";
  
  for(int i = 0; i < 150; i++){
    PORTB=0b00001010;   
    MSDelay(50);  
    PORTB=0b00000110;    
    MSDelay(50);  
    PORTB=0b00000101;    
    MSDelay(50);  
    PORTB=0b00001001;  
    MSDelay(50);
  }
  
  while (*stop != '\0'){
    while (!(SCI1SR1 & 0x80));  /* wait for output buffer empty */
    SCI1DRL = *stop++;    //transmit back to signal finished scanning
  }
  
  for(int i = 0; i < 150; i++){
    PORTB=0b00001010;
    MSDelay(50);
    PORTB=0b00001001;  
    MSDelay(50);
    PORTB=0b00000101;    
    MSDelay(50);
    PORTB=0b00000110;  
  }
}

int main(void){
  UserSCI1 = (unsigned short)&SCI1_ISR;
  const char *stop = "stop";
  
  DDRB = 0x0F;      //PORTB0-PORTB3  as output 
  DDRP = 0x03;      //PORTP0 and PORTP1 as output for 12EN=1 and 34EN=1
    
  PORTB=0b00000000; // start with all off
  PTP=0b00000011;   //Turn on both 12EN and 34EN Enables for 754410 chip  
  
  SCI1BDH = 0x00;   //set options for SCI communications and interrupts
  SCI1BDL = 0x34;
  SCI1CR1 = 0x00;
  SCI1CR2 = 0x2C;
  
  asm("cli");       //enable interrupt globally
  
  unsigned char rc = SCI1SR1; /* dummy read to clear flags and TDRE */
  SCI1DRH = 0x0000; /* data write to clear TDRE */
  
  while (*stop != '\0'){
    while (!(SCI1SR1 & 0x80));  /* wait for output buffer empty */
    SCI1DRL = *stop++;    //transmit back to signal finished scanning
  }
    
  while(1);         //Wait for the interrupt forever
  return 0;
}
