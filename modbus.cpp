#include "modbus.h"
#include "msp430x54x.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ADC_operation.h"

unsigned char mbs_frame_buff[256];
unsigned char mbs_buf_offset = 0;      
bool eof = 0;                               //Взводится, когда кадр полностью прочитан

void set_timer(int t)
{
   TA0CCR0 = 20057*t;
}

primary_table_s primary_table;

//Функция для отправки готового кадра
void send_buff(unsigned char* out_buf, unsigned int size)
{
  for(int i = 0; i<size; i++)
  {
    while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
    UCA0TXBUF = out_buf[i];          			 // TX -> RXed character 
  }
  mbs_buf_offset = 0;
  eof = 0;
}

void timer_init()
{
  TA0CCTL0 = CCIE;                        					// CCR0 interrupt enabled
  TA0CCR0 = 0;                              					// Stops timer
  TA0CTL = TASSEL_2 + MC_1 + TACLR;          // SMCLK, upmode, clear TAR
}

//Initialize UART
void UART_init()
{
  P3DIR |= (1<<6);
  P3SEL = 0x30;                             		// P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1 |= UCSWRST;                       // **Put state machine in reset**
  UCA0CTL1 = UCSSEL__SMCLK;               // SMCLK
  UCA0BR0 = 0x23;                           	    // 20 MHz
  UCA0BR1 = 0x8;                             
  UCA0MCTL |= UCBRS_2 + UCBRF_0;      // Modulation UCBRSx=1, UCBRFx=0
  UCA0CTL1 &= ~UCSWRST;                    // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;                         		// Enable USCI_A0 RX interrupt
}

void MBS_init()
{
  UART_init();
  timer_init();
  primary_table.all_memory[0] = SLAVE_ADDRESS;
}

unsigned char MBSB_read(unsigned char* dest, unsigned char* src, unsigned char reg_cnt)
{
	if((reg_cnt<1)||(reg_cnt>NUMBER_OF_REGISTERS))
		return 3;
	if((src<primary_table.all_memory)||(src>=primary_table.end_memory))
		return 2;
	memcpy(dest, src, reg_cnt*2);
	return 0;
}

unsigned char MBSB_write(unsigned char* dest, unsigned char* src, unsigned char reg_cnt) 
{
        if((reg_cnt<1)||(reg_cnt>NUMBER_OF_HOLD_REGISTERS))
              return 3;
        if((dest>=primary_table.input_registers)||
           (dest<primary_table.holding_registers)||
             (dest+2*reg_cnt-1>=primary_table.input_registers))
        {
                return 2;
        }
	memcpy(dest, src, 2*reg_cnt);
	return 0;
}

void MBSB_write_registers(int offset, unsigned char* src, unsigned char reg_cnt)
{
      memcpy(primary_table.all_memory+offset, src, 2*reg_cnt);
}

void MBS_operation()
{
  //Кадр получен?
  if(eof)
  {
    unsigned int res_buf_size = 0;
	//Обрабатываем кадр
    unsigned char* out_buf = 
     SlaveProcess(mbs_frame_buff, mbs_buf_offset, &res_buf_size, SLAVE_ADDRESS,
                   MBSB_read, MBSB_write, primary_table.holding_registers,
                   NUMBER_OF_REGISTERS*2);
	//Посылаем ответ
    send_buff(out_buf, res_buf_size);
    free(out_buf);
  }
}

//Прерывание для чтения по modbus
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,4))
  {
  case 0:break;                        
  case 2:                              // Прерывание на принятие сообщения
    if(eof)                            // Кадр принят, не обработан
    {
      /*Нужно послать ошибку, что устройство занято */
      return; 
    }
    mbs_frame_buff[mbs_buf_offset++]
      = UCA0RXBUF;                     // Читаем очередной байт
    set_timer(5);                      // Заводим таймер на 5мс.
    TA0R = 0;
    break;
  case 4:break;                        
  default: break;
  }
}

// Таймер
int sec = 0;
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
  if(sec++ >=1000)
  {
    ADC_init();
    sec = 0;
  }
  eof = 1;
}
