/*
Test program for just reciving data from master
*/

#include "msp430x54xa.h"
#include "modbus.h"
#include "SMLK_init.h"
#include "ADC_operation.h"
#include "float.h"

//Initialize UART
void UART_init()
{
  P3DIR |= (1<<6);
  P3SEL = 0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA0CTL1 = UCSSEL__SMCLK;                 // SMCLK
  UCA0BR0 = 0x23;                           // 20 MHz
  UCA0BR1 = 0x8;                            // 
  UCA0MCTL |= UCBRS_2 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}


int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;     //Stop WDT
  set_SMLK_20MHz();
  UART_init();                  //Initialize UART (smlk 20 MHz is needed)
  MBS_init();                   //Initialize MODBUS
  ADC_init();
  __bis_SR_register(GIE);       //Interrupts enabled
  for(;;)
  { 
    //Получаем напряжение и пишем в регистры модбас
    voltage_container vc = get_voltage();
    MBSB_write_registers(2, vc.i_volt, 2);
    MBS_operation();
  }  
}