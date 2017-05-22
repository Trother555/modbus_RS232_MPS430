#ifndef _ADC_init
#define _ADC_init

union voltage_container
{
  float f_volt;
  unsigned char i_volt[4];
};  

//Инициализация АЦП.
void ADC_init();

//Возвращает считанное с АЦП напряжение
voltage_container get_voltage();

#endif