#include "modbus_buffer_functionality.h"

unsigned char MBSB_read(unsigned char* dest, unsigned char* src, unsigned char reg_cnt)
{
	if((reg_cnt<1)||(reg_cnt>NUMBER_OF_REGISTERS))
		return 3;
	if((src<all_memory)||(src>=end_memory))
		return 2;
	memcpy(dest, src, reg_cnt*2);
	return 0;
}
unsigned char MBSB_write(unsigned char* dest, unsigned char* src, unsigned char reg_cnt)
{
	if((reg_cnt<1)||(reg_cnt>NUMBER_OF_HOLD_REGISTERS)
		return 3;
	if((dest>=input_registers)||(dest<holding_registers)||(dest+2*reg_cnt>=input_registers))
	{
		return 2;
	}
	memcpy(dest, src, 2*reg_cnt);
	return 0;
}