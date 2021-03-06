#include "ADC_operation.h"
#include "msp430x54xa.h"

#define sample_count (50)

unsigned int results[sample_count];   
bool voltage_ready = 0;

void ADC_init()
{
	//Enable refference generator
    REFCTL0 |= REFON;
	 // Enable A/D channel A0
    P6SEL |= 0x01;         
    // Turn on ADC12, set sampling time, set multiple sample conversion, set ref on, set ADC clock divider to 8
    ADC12CTL0 = ADC12ON+ADC12SHT0_14+ADC12MSC + ADC12REFON_L + ADC12DIV_7;
    // Use sampling timer, repeat single channel
    ADC12CTL1 = ADC12SHP + ADC12CONSEQ_2;
	// Vr+=Vref+ and Vr-=AVss
    ADC12MCTL0 = ADC12SREF_1;
	// Enable ADC12IFG.0	
    ADC12IE = 0x01;               
	// Enable conversions	
    ADC12CTL0 |= ADC12ENC;                    		
}

//Вспомогательная функция для get_voltage
float get_volt()
{
    unsigned long int res = 0;
    for(int i =0;i<sample_count;i++)
    {
     res+=results[i];
    }       
	//Формула из документации к MSP430, параграф про АЦП.
    return (float)res/sample_count*3/4095;
}

voltage_container get_voltage()
{
    voltage_container vc;
	// Start ADC conversion
    ADC12CTL0 |= ADC12SC;       
    while(!voltage_ready);
    vc.f_volt = get_volt();
    return vc;
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
    static unsigned char index = 0;
  
    switch(__even_in_range(ADC12IV,34))
    {
    case  0: break;                           // Vector  0:  No interrupt
    case  2: break;                           // Vector  2:  ADC overflow
    case  4: break;                           // Vector  4:  ADC timing overflow
    case  6:                                  		// Vector  6:  ADC12IFG0
      if (index == sample_count)
      {
        index = 0;
        voltage_ready = true;
        ADC12IE = 0x00;
        return;
      }
      results[index] = ADC12MEM0;  
      index++;                               

    case  8: break;                           // Vector  8:  ADC12IFG1
    case 10: break;                           // Vector 10:  ADC12IFG2
    case 12: break;                           // Vector 12:  ADC12IFG3
    case 14: break;                           // Vector 14:  ADC12IFG4
    case 16: break;                           // Vector 16:  ADC12IFG5
    case 18: break;                           // Vector 18:  ADC12IFG6
    case 20: break;                           // Vector 20:  ADC12IFG7
    case 22: break;                           // Vector 22:  ADC12IFG8
    case 24: break;                           // Vector 24:  ADC12IFG9
    case 26: break;                           // Vector 26:  ADC12IFG10
    case 28: break;                           // Vector 28:  ADC12IFG11
    case 30: break;                           // Vector 30:  ADC12IFG12
    case 32: break;                           // Vector 32:  ADC12IFG13
    case 34: break;                           // Vector 34:  ADC12IFG14
    default: break; 
    }
}