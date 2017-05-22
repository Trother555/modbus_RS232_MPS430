#ifndef __MODBUS
#define __MODBUS

#include "modbus_slave.h"

//Начальный адрес текущего устройства(1-247)
#define SLAVE_ADDRESS (1)                
#define NUMBER_OF_REGISTERS (3)         
#define NUMBER_OF_HOLD_REGISTERS (2)


//Таблица памяти Modbus 
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

//Основная функция Modbus, выполняемая в цикле работы протокола. 
void MBS_operation();
//Начальная инициализация
void MBS_init();
//Запись в память Modbus по смещению offset от src регистров в количестве reg_cnt
void MBSB_write_registers(int offset, unsigned char* src, unsigned char reg_cnt);

#endif
