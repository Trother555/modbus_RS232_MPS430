#ifndef MODBUS_BUFFER_H
#define MODBUS_BUFFER_H

unsigned short CRC16(unsigned char* buffer, unsigned int size);

unsigned char* SlaveProcess(unsigned char* buffer,
                            unsigned int size,
                            unsigned int *resultBufferSize,
                            unsigned char slaveAddress,
                            unsigned char (*read)(unsigned char*, unsigned char*, unsigned char),
                            unsigned char (*write)(unsigned char*, unsigned char*, unsigned char),
                            unsigned char* firstRegister,
                            unsigned short totalRegistersSize = 0xFFFFU,
                            char isHighLowOrder = 0);
#endif // MODBUS_BUFFER_H
