#ifndef MODBUS_H
#define MODBUS_H


unsigned short CRC16(unsigned char* buffer, unsigned int size);


char IsValidBufferSizeFromMaster(unsigned char* buffer, unsigned int size);


//TODO: возможно, указатель надо по ссылке передавать
unsigned char* SlaveProcess(unsigned char* buffer,
                            unsigned int bufferSize,
                            unsigned int *resultBufferSize,
                            unsigned char slaveAddress,
                            unsigned char (*read)(unsigned char*, unsigned char*, unsigned char),
                            unsigned char (*write)(unsigned char*, unsigned char*, unsigned char),
                            unsigned char* firstRegister,
                            unsigned short totalRegistersSize = 0xFFFFU,
                            char isHighLowOrder = 0);

unsigned char* CreateBufferReadHoldingRegisters(unsigned char slaveAddress,
                                                unsigned short firstParamAddress,
                                                unsigned short countRegisters,
                                                unsigned int *bufferSize);

unsigned char* CreateBufferReadInputRegisters(unsigned char slaveAddress,
                                              unsigned short firstParamAddress,
                                              unsigned short countRegisters,
                                              unsigned int *bufferSize);

unsigned char* CreateBufferWriteSingleHoldingRegister(unsigned char slaveAddress,
                                                      unsigned short paramAddress,
                                                      unsigned short paramValue,
                                                      unsigned int* bufferSize,
                                                      char isHighLowOrder = 0);

unsigned char* CreateBufferWriteMultipleHoldingRegisters(unsigned char slaveAddress,
                                                         unsigned short firstParamAddress,
                                                         unsigned short countRegisters,
                                                         unsigned char countBytes,
                                                         unsigned short* values,
                                                         unsigned int* bufferSize,
                                                         char isHighLowOrder = 0);


char IsValidBufferSizeFromSlave(unsigned char* buffer, unsigned int size);


char* RecvBufferToString(unsigned char* buffer, unsigned int size, char isHighLowOrder);


char* StringOfBufferBytes(unsigned char* buffer, unsigned int size);

#endif // MODBUS_H
