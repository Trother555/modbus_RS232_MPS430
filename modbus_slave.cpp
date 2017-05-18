#include "modbus_slave.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

#define SLAVE_ADDRESS buffer[0]
#define COMMAND buffer[1]


unsigned short CRC16(unsigned char* buffer, unsigned int size)
{
    union
    {
        unsigned char arr[2];
        unsigned short value;
    } sum;
    unsigned char shiftCount, *ptr;
    ptr = buffer;
    sum.value=0xFFFFU;
    for(; size>0; size--)
    {
        sum.value=(unsigned short)((sum.value/256U)*256U+((sum.value%256U)^(*ptr++)));
        for(shiftCount=0; shiftCount<8; shiftCount++)
        {
            if((sum.value&0x1)==1)
                sum.value=(unsigned short)((sum.value>>1)^0xA001U);
            else
                sum.value>>=1;
        }
    }
    return sum.value;
}


//Изменение порядка следования байтов
void ChangeByteOrder(unsigned char* buffer, unsigned int size)
{
    for (unsigned int i=0; i < size-1; i+=2)
    {
        unsigned char tmp = buffer[i];
        buffer[i] = buffer[i+1];
        buffer[i+1] = tmp;
    }
}


unsigned char* CreateErrorBuffer(unsigned char address, unsigned char command, unsigned char errorCode)
{
    unsigned char* result = (unsigned char*)malloc(5);
    result[0] = address;
    result[1] = command | (1U<<7);
    result[2] = errorCode;

    unsigned short crc = CRC16(result, 3);

    result[3] = crc & 0xFF;
    result[4] = crc >> 8;

    return result;
}


//Проверка корректности размера буфера, принятого
//от slave-устройства и правильности CRC
char IsValidBufferSizeFromMaster(unsigned char* buffer, unsigned int size)
{
    if (size<2)
        return 0;
    switch(COMMAND)
    {
    case 0x03:
    case 0x04:
    case 0x06:
        if (size!=8)
            return 0;

        break;

    case 0x10:
        if (size<9)
            return 0;

        if (size != 9+buffer[6])
            return 0;

        break;

    default:
        return 0;
    }

    if (CRC16(buffer,size-2) != (unsigned short)(buffer[size-1] << 8 | buffer[size - 2]))
        return 0;

    return 1;
}


unsigned char* SlaveProcess(unsigned char* buffer,
                            unsigned int bufferSize,
                            unsigned int* resultBufferSize,
                            unsigned char slaveAddress,
                            unsigned char (*read)(unsigned char*, unsigned char*, unsigned char),
                            unsigned char (*write)(unsigned char*, unsigned char*, unsigned char),
                            unsigned char* firstRegister,
                            unsigned short totalRegistersSize,
                            char isHighLowOrder)
{
    //проверка длины и CRC
    if (!IsValidBufferSizeFromMaster(buffer, bufferSize))
    {
        *resultBufferSize = 0;
        return NULL;
    }


    //Неправильный адрес устройства
    if (SLAVE_ADDRESS != slaveAddress && SLAVE_ADDRESS != 0)
    {
        *resultBufferSize = 0;
        return NULL;
    }

    unsigned char* result;

    //Неизвестная функция - error 0x01
    if (COMMAND != 0x03 && COMMAND != 0x04 && COMMAND != 0x06 && COMMAND != 0x10)
    {
        *resultBufferSize = 5;
        result = CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, 0x01);

        return result;
    }

    //Неверный адрес регистра - error 0x02
    if ((buffer[2]<<8 | buffer[3] > totalRegistersSize)
            || (COMMAND == 0x10 && (buffer[2]<<8|buffer[3]) + buffer[6] > totalRegistersSize)
            || ((COMMAND == 0x03 || COMMAND == 0x04) && (buffer[2]<<8|buffer[3]) + (buffer[4]<<1|buffer[5])*2 > totalRegistersSize))
    {
        *resultBufferSize = 5;
        result = CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, 0x02);

        return result;
    }

    //Все ошибки, что мог, обработал
    unsigned char errorCode;
    switch(COMMAND)
    {
    case 0x03:
    case 0x04:
        *resultBufferSize = (buffer[4]<<8 | buffer[5]) * 2 + 5;

        result = (unsigned char*)malloc(*resultBufferSize);
        errorCode = read(result+3, firstRegister+(buffer[2]<<8 | buffer[3]), buffer[4]<<8 | buffer[5]);

        if (errorCode)
        {
            free(result);

            *resultBufferSize = 5;

            return CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, errorCode);
        }

        if(isHighLowOrder)
        {
            ChangeByteOrder(result+3, (buffer[4]<<8 | buffer[5])*2);
        }
        result[0] = SLAVE_ADDRESS;
        result[1] = COMMAND;
        result[2] = (buffer[4]<<8 | buffer[5]) * 2;

        break;

    case 0x06:
        errorCode;
        if (isHighLowOrder)
        {
            unsigned short tmp = buffer[4] << 8 | buffer[5];
            errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), (unsigned char*)&tmp, 1);
        }
        else
        {
            errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), buffer+4, 1);
        }

        if (errorCode)
        {
            *resultBufferSize = 5;

            return CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, errorCode);
        }

        *resultBufferSize = bufferSize;
        result = (unsigned char*)malloc(*resultBufferSize);
        memcpy(result, buffer, 6);

        break;

    case 0x10:
        if (isHighLowOrder)
        {
            ChangeByteOrder(buffer+7, buffer[6]);
        }

        unsigned char errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), buffer+7, buffer[6]/2);

        if (errorCode)
        {
            *resultBufferSize = 5;

            return CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, errorCode);
        }

        *resultBufferSize = 8;
        result = (unsigned char*)malloc(*resultBufferSize);

        memcpy(result, buffer, 6);

        break;
    }

    //*(unsigned short*)(result + *resultBufferSize - 2) = CRC16(result, *resultBufferSize-2);
    unsigned short crc = CRC16(result, *resultBufferSize - 2);
    result[*resultBufferSize-2] = crc & 0xFF;
    result[*resultBufferSize-1] = crc >> 8;
    
    return result;
}


unsigned char* CreateBufferReadHoldingRegisters(unsigned char slaveAddress,
                                                unsigned short firstParamAddress,
                                                unsigned short countRegisters,
                                                unsigned int* bufferSize)
{
    *bufferSize = 8;

    unsigned char* buffer = (unsigned char*)malloc(*bufferSize);
    buffer[0] = slaveAddress;
    buffer[1] = 0x03;
    buffer[2] = firstParamAddress >> 8;
    buffer[3] = firstParamAddress & 0xFFU;
    buffer[4] = countRegisters >> 8;
    buffer[5] = countRegisters & 0xFFU;

    unsigned short crc = CRC16(buffer, 6);

    buffer[6] = crc & 0xFFU;
    buffer[7] = crc >> 8;

    return buffer;
}


unsigned char* CreateBufferReadInputRegisters(unsigned char slaveAddress,
                                              unsigned short firstParamAddress,
                                              unsigned short countRegisters,
                                              unsigned int* bufferSize)
{
    *bufferSize = 8;

    unsigned char* buffer = (unsigned char*)malloc(*bufferSize);
    buffer[0] = slaveAddress;
    buffer[1] = 0x04;
    buffer[2] = firstParamAddress >> 8;
    buffer[3] = firstParamAddress & 0xFFU;
    buffer[4] = countRegisters >> 8;
    buffer[5] = countRegisters & 0xFFU;

    unsigned short crc = CRC16(buffer, 6);

    buffer[6] = crc & 0xFFU;
    buffer[7] = crc >> 8;

    return buffer;
}


unsigned char* CreateBufferWriteSingleHoldingRegister(unsigned char slaveAddress,
                                                      unsigned short paramAddress,
                                                      unsigned short paramValue,
                                                      unsigned int* bufferSize,
                                                      char isHighLowOrder)
{
    *bufferSize = 8;

    unsigned char* buffer = (unsigned char*)malloc(*bufferSize);
    buffer[0] = slaveAddress;
    buffer[1] = 0x06;
    buffer[2] = paramAddress >> 8;
    buffer[3] = paramAddress & 0xFFU;
    if (isHighLowOrder)
    {
        buffer[4] = paramValue >> 8;
        buffer[5] = paramValue & 0xFFU;
    }
    else
    {
        buffer[4] = paramValue & 0xFFU;
        buffer[5] = paramValue >> 8;
    }

    unsigned short crc = CRC16(buffer, 6);

    buffer[6] = crc & 0xFFU;
    buffer[7] = crc >> 8;

    return buffer;
}


unsigned char* CreateBufferWriteMultipleHoldingRegisters(unsigned char slaveAddress,
                                                         unsigned short firstParamAddress,
                                                         unsigned short countRegisters,
                                                         unsigned char countBytes,
                                                         unsigned short* values,
                                                         unsigned int* bufferSize,
                                                         char isHighLowOrder)
{
    *bufferSize = 9 + countBytes;

    unsigned char* buffer = (unsigned char*)malloc(*bufferSize);
    buffer[0] = slaveAddress;
    buffer[1] = 0x10;
    buffer[2] = firstParamAddress >> 8;
    buffer[3] = firstParamAddress & 0xFFU;
    buffer[4] = countRegisters & 0xFFU;
    buffer[5] = countRegisters >> 8;
    buffer[6] = countBytes;

    for (unsigned char i=0; i < countBytes/2; ++i)
    {
        if (isHighLowOrder)
        {
            buffer[7+2*i] = (unsigned char)(values[i] >> 8);
            buffer[8+2*i] = (unsigned char)(values[i] & 0xFF);
        }
        else
        {
            buffer[7+2*i] = (unsigned char)(values[i] & 0xFF);
            buffer[8+2*i] = (unsigned char)(values[i] >> 8);
        }
    }

    unsigned short crc = CRC16(buffer, *bufferSize - 2);

    buffer[*bufferSize - 2] = crc & 0xFFU;
    buffer[*bufferSize - 1] = crc >> 8;

    return buffer;
}


//Проверка корректности размера буфера, принятого
//от slave-устройства и правильности CRC
char IsValidBufferSizeFromSlave(unsigned char* buffer, unsigned int size)
{
    if(size<2)
        return 0;

    switch(COMMAND)
    {
    case 0x03:
    case 0x04:
        if (size<3)
            return 0;

        if (size != 5+buffer[2])
            return 0;

        break;

    case 0x06:
    case 0x10:
        if (size!=8)
            return 0;

        break;
    case 0x83:
    case 0x84:
    case 0x86:
    case 0x90:
        if (size != 5)
            return 0;

        break;

    default:
        return 0;
    }

    return 1;
}


//Построение информации о принятом кадре
char* RecvBufferToString(unsigned char* buffer, unsigned int size, char isHighLowOrder)
{
    char *result = (char*)malloc(1024), error[100];
    int pos = 0;

    if (!IsValidBufferSizeFromSlave(buffer, size))
    {
        sprintf(result, "Пришло сообщение некорректной длины %u", size);
        return result;
    }

    unsigned short crc = CRC16(buffer, size-2);

    if (crc != (unsigned short)(buffer[size-1] << 8 | buffer[size-2]))
    {
        sprintf(result, "Несовпадение CRC\nПринято: %04hX, вычислено: %04hX",
                (unsigned short)(buffer[size-1] << 8 | buffer[size-2]), crc);
        return result;
    }

    switch (COMMAND) {
    case 0x03:
    case 0x04:
        pos = sprintf(result, "Адрес устройства: 0x%02hhX\nКоманда: 0x%02hhX\nКоличество байт: %hhu",
                      SLAVE_ADDRESS, COMMAND, buffer[2]);

        for(unsigned char i=0; i < buffer[2]/2; ++i)
        {
            if (isHighLowOrder)
            {
                pos += sprintf(result+pos, "\nValue[%hhu] = 0x%04hX", i, buffer[3+2*i]<<8|buffer[4+2*i]);
            }
            else
            {
                pos += sprintf(result+pos, "\nValue[%hhu] = 0x%04hX", i, buffer[4+2*i]<<8|buffer[3+2*i]);
            }
        }

        break;

    case 0x06:
        pos = sprintf(result,"Адрес устройства: 0x%02hhX\nКоманда: 0x%02hhX\nАдрес параметра: 0x%04hX\n",
                      SLAVE_ADDRESS, COMMAND, buffer[2]<<8|buffer[3]);

        if (isHighLowOrder)
        {
            sprintf(result+pos, "Значение параметра: 0x%04hX", buffer[4]<<8|buffer[5]);
        }
        else
        {
            sprintf(result+pos, "Значение параметра: 0x%04hX", buffer[5]<<8|buffer[4]);
        }

        break;

    case 0x10:
        sprintf(result, "Адрес устройства: 0x%02hhX\nКоманда: 0x%02hhX\nАдрес первого параметра: 0x%04hX\nКоличество параметров: %hu",
                SLAVE_ADDRESS, COMMAND, buffer[2]<<8|buffer[3], buffer[4]<<8|buffer[5]);

        break;


    default:
        if (COMMAND & 0x80)
        {
            switch(buffer[2])
            {
            case 0x00:
                sprintf(error, "нет ошибки");
                break;
            case 0x01:
                sprintf(error, "неизвестная функция");
                break;
            case 0x02:
                sprintf(error, "неверный адрес регистра");
                break;
            case 0x03:
                sprintf(error, "неверный формат данных");
                break;
            case 0x04:
                sprintf(error, "неисправность оборудования");
                break;
            case 0x05:
                sprintf(error, "устройство приняло запрос и занято его обработкой");
                break;
            case 0x06:
                sprintf(error, "устройство занято обработкой предыдущей команды");
                break;
            case 0x08:
                sprintf(error, "ошибка при работе с памятью");
                break;
            default:
                sprintf(error, "неизвестная ошибка");
                break;
            }

            sprintf(result, "Команда: 0x%02hhX, Ошибка: %s (код: 0x%02hhX)", buffer[1], error, buffer[2]);
        }
        else
        {
            sprintf(result, "Неизвестная команда с кодом 0x%02hhX", buffer[1]);
        }
    }

    return result;
}


//Построение информации о байтах отправленного/принятого кадра
char* StringOfBufferBytes(unsigned char* buffer, unsigned int size)
{
    char *str = new char[size*3+1];
    int pos = 0;

    for (unsigned short i = 0; i < size; ++i)
    {
        pos+=sprintf(str+pos, "%02hhX ", buffer[i]);
    }
    return str;
}
