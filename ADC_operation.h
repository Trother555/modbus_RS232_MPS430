#ifndef _ADC_init
#define _ADC_init

union voltage_container
{
  float f_volt;
  unsigned char i_volt[4];
};  

void ADC_init();
voltage_container get_voltage();

#endif