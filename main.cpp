#include "msp430x54xa.h"
#include "modbus.h"
#include "SMCLK_init.h"
#include "ADC_operation.h"
#include "float.h"

int main( void )
{
  //������������� WDT
  WDTCTL = WDTPW + WDTHOLD; 
  //��������� ������������� ���� �������     
  set_SMLK_20MHz();                
  MBS_init();                   
  ADC_init();
  //�������� ����������
  __bis_SR_register(GIE);    
  //������� ����  
  for(;;)
  { 
    //�������� ���������� � ����� � �������� Modbus
    voltage_container vc = get_voltage();
    MBSB_write_registers(2, vc.i_volt, 2);
    //��������� ������� ���������
    MBS_operation();
  }  
}