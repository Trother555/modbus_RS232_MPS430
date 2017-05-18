#ifndef __MODBUS
#define __MODBUS

#include "modbus_slave.h"


#define SLAVE_ADDRESS (1)                 //Адрес текущего устройства(1-247)
#define NUMBER_OF_REGISTERS (3)         
#define NUMBER_OF_HOLD_REGISTERS (2)


//Modbus primary table
struct primary_table_s
{
  unsigned char all_memory[2*NUMBER_OF_REGISTERS];
  unsigned char* holding_registers;
  unsigned char* input_registers;
  unsigned char* end_memory;
  primary_table_s():
    holding_registers(all_memory), 
    input_registers(all_memory + 2*NUMBER_OF_HOLD_REGISTERS),
	end_memory(all_memory+2*NUMBER_OF_REGISTERS){}
};

/*
struct mb_req_pdu_s
{
  unsigned char function_code;
  unsigned char data[252];
};
*/

void write_buff();
void MBS_operation();
void MBS_init();
unsigned char MBSB_read(unsigned char*, unsigned char*, unsigned char);
unsigned char MBSB_write(unsigned char*, unsigned char*, unsigned char);
void MBSB_write_registers(int offset, unsigned char*, unsigned char);
#endif
