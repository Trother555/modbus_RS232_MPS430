#include "SMCLK_init.h"
#include "msp430x54x.h"
//Energy voltage level up
void SetVCoreUp (unsigned int level)
{
// Open PMM registers for write access
PMMCTL0_H = 0xA5;
// Set SVS/SVM high side new level
SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
// Set SVM low side to new level
SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;
// Wait till SVM is settled
while ((PMMIFG & SVSMLDLYIFG) == 0);
// Clear already set flags
PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
// Set VCore to new level
PMMCTL0_L = PMMCOREV0 * level;
// Wait till new level reached
if ((PMMIFG & SVMLIFG))
while ((PMMIFG & SVMLVLRIFG) == 0);
// Set SVS/SVM low side to new level
SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
// Lock PMM registers for write access
PMMCTL0_H = 0x00;
}

//Init XT1
void LFXT1_init()
{
  // Initialize LFXT1
  P7SEL |= 0x03;                            // Select XT1
  UCSCTL6 &= ~(XT1OFF);                     // XT1 On
  UCSCTL6 |= XCAP_1;                        // Internal load cap
  UCSCTL6 &= ~(XT1DRIVE0);
  UCSCTL6 &= ~(XT1DRIVE1);
  
  // Loop until XT1 fault flag is cleared
  do
  {
    UCSCTL7 &= ~XT1LFOFFG;                  // Clear XT1 fault flags
  }while (UCSCTL7&XT1LFOFFG);               // Test XT1 fault flag
}

//Set sufficient power level
void set_power()
{
   for(unsigned int i = 1; i<3; i++)
  {
    SetVCoreUp(i);
  }  
}

void DCO_init()
{
  __bis_SR_register(SCG0);                  // Disable the FLL control loop
  //Setting frequency range 
  UCSCTL0 = DCO0+DCO1+DCO2+DCO3+DCO4;                         
  UCSCTL1 = DCORSEL_5;                      
  UCSCTL2 = FLLD_1 + 305;	            //20 MHz
  UCSCTL3 &= ~(SELREF0+SELREF1+SELREF2);    //Setting XT1 to be the source for FFL
  __bic_SR_register(SCG0);                  // Enable the FLL control loop
  
  __delay_cycles(625000);
  //Fault-controll for generators
  do
  {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
                                            // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                      // Clear fault flags
  }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
  // Make DCO to be the source for SMCLK
  UCSCTL4 |= SELS0+SELS1;
  UCSCTL4 &= ~SELS2;
}


void SMLK_init()
{
  //Init SMLK with DCO (UART uses SMLK)
  UCSCTL4 |= SELS0+SELS1;
  UCSCTL4 &= ~SELS2; 
}

void set_SMLK_20MHz()
{
  LFXT1_init();                 //Init XT1
  set_power();                  //Set sufficient power level
  DCO_init();                   //Initialize DCO to 20MHz
  SMLK_init();                  //Init SMLK with DCO (UART uses SMLK) 
}