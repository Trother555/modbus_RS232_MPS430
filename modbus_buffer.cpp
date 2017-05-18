#include "modbus_buffer.h"
#include "string.h"

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

    crc = CRC16(result, 3);

    result[3] = crc & 0xFF;
    result[4] = crc >> 8;

    return result;
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
    unsigned short crc = CRC16(buffer, bufferSize-2);
    //CRC принятая и вычисленная не совпадают
    if (crc != buffer[bufferSize-1] << 8 | buffer[bufferSize - 2])
    {
        *resultBufferSize = 0;
        free(buffer);

        return NULL;
    }

    //Неправильный адрес устройства
    if (buffer[0] != slaveAddress && buffer[0] != 0)
    {
        *resultBufferSize = 0;
        free(buffer);

        return NULL;
    }

    unsigned char* result;

    //Неизвестная функция - error 0x01
    if (buffer[1] != 0x03 && buffer[1] != 0x04 && buffer[1] != 0x06 && buffer[1] != 0x10)
    {
        *resultBufferSize = 5;
        result = CreateErrorBuffer(buffer[0], buffer[1], 0x01);
        free(buffer);

        return result;
    }

    //Неверный адрес регистра
    if ((buffer[2]<<8 | buffer[3] > totalRegistersSize)
            || (buffer[1] == 0x10 && (buffer[2]<<8|buffer[3]) + buffer[6] > totalRegistersSize)
            || ((buffer[1] == 0x03 || buffer[1] == 0x04) && (buffer[2]<<8|buffer[3]) + (buffer[4]<<1|buffer[5])*2 > totalRegistersSize))
    {
        *resultBufferSize = 5;
        result = CreateErrorBuffer(buffer[0], buffer[1], 0x02);
        free(buffer);

        return result;
    }

    //Все ошибки, что мог, обработал

    switch(buffer[1])
    {
    case 0x03:
    case 0x04:
        *resultBufferSize = (buffer[4]<<8 | buffer[5]) * 2 + 5;

        result = (unsigned char*)malloc(*resultBufferSize);
        unsigned char errorCode = read(result+3, firstRegister+(buffer[2]<<8 | buffer[3]), buffer[4]<<8 | buffer[5]);

        if (errorCode)
        {
            free(result);

            *resultBufferSize = 5;
            result = CreateErrorBuffer(buffer[0], buffer[1], errorCode);
            free(buffer);

            return result;
        }

        result[0] = buffer[0];
        result[1] = buffer[1];
        result[2] = (buffer[4]<<8 | buffer[5]) * 2;

        free(buffer);
        break;

    case 0x06:
        unsigned char errorCode;
        if (isHighLowOrder)
        {
            unsigned short tmp = buffer[4] << 8 | buffer[5];
            errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), (unsigned char*)&tmp, 2);
        }
        else
        {
            errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), buffer+4, 2);
        }

        if (errorCode)
        {
            *resultBufferSize = 5;
            free(buffer);
            return CreateErrorBuffer(buffer[0], buffer[1], errorCode);
        }

        result = buffer;
        *resultBufferSize = bufferSize;

        break;

    case 0x10:
        if (isHighLowOrder)
        {
            ChangeByteOrder(buffer+7, buffer[6]);
        }

        unsigned char errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), buffer+7, buffer[6]);

        if (errorCode)
        {
            *resultBufferSize = 5;
            free(buffer);
            return CreateErrorBuffer(buffer[0], buffer[1], errorCode);
        }

        *resultBufferSize = 8;
        result = (unsigned char*)malloc(*resultBufferSize);

        memcpy(result, buffer, 6);
        free(buffer);
        break;
    }

    *(unsigned short*)(result + *resultBufferSize - 2) = CRC16(result, *resultBufferSize-2);

    return result;
}
