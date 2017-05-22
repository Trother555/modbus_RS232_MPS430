#include "msp430x54xa.h"
#include "modbus.h"
#include "SMCLK_init.h"
#include "ADC_operation.h"
#include "float.h"

int main( void )
{
  //Останавливаем WDT
  WDTCTL = WDTPW + WDTHOLD; 
  //Выполняем инициализацию всех модулей     
  set_SMLK_20MHz();                
  MBS_init();                   
  ADC_init();
  //Включаем прерывания
  __bis_SR_register(GIE);    
  //Главный цикл  
  for(;;)
  { 
    //Получаем напряжение и пишем в регистры Modbus
    voltage_container vc = get_voltage();
    MBSB_write_registers(2, vc.i_volt, 2);
    //Выполняем функции протокола
    MBS_operation();
  }  
}