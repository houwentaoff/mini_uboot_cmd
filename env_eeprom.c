 /******************************************************************************
  * @file    env_eeprom.c
  * @author  joy
  * @version V1.0.0
  * @date    16-Nov-2016
  * @brief   uboot cmd env eeprom implements
  ******************************************************************************
*/
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "hw_eeprom.h"
#include "env_eeprom.h"


static const char HexTable[]="0123456789ABCDEF";
static const char Sep = ':';

static unsigned long SimpleStrtoul(const char *cp, char **endp,
                unsigned int Base)
{
    unsigned long Result = 0;
    unsigned long Value;

    if (*cp == '0') 
    {
        cp++;
        if ((*cp == 'x') && isxdigit(cp[1])) 
        {
            Base = 16;
            cp++;
        }

        if (!Base)
        {
            Base = 8;
        }
    }

    if (!Base)
    {
        Base = 10;
    }

    while (isxdigit(*cp) && (Value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
        ? toupper(*cp) : *cp)-'A'+10) < Base) 
    {
        Result = Result*Base + Value;
        cp++;
    }

    if (endp)
    {
        *endp = (char *)cp;
    }

    return Result;
}

static void Hex2Str(const unsigned char *Hex, char *Str, int Num)
{
    int i;
    int j;

    for (i=0, j=0; i < Num; i++)
    {
        Str[j++] = HexTable[(Hex[i]&0xf0) >> 4];
        Str[j++] = HexTable[(Hex[i]&0x0f)];
        Str[j++]   = Sep;
    }
    if (j > 0)
    {
        Str[j-1] = '\0';
    }
}
void Str2Hex(const char *Str, unsigned char *Hex, int Num)
{
    char *End;
    int i;

    for (i = 0; i < Num; ++i) 
    {
        Hex[i] = Str ? SimpleStrtoul(Str, &End, 16) : 0;
        if (Str)
        {
            Str = (*End) ? End + 1 : End;
        }
    }
}
DataIndex_t GetIndex(const char *Key)
{
    DataIndex_t Index = ENDINDEX;

    if (strcmp(Key, "DEVEUI") == 0)
    {
        Index = DEVEUI;
    }
    else if (strcmp(Key, "APPEUI") == 0)
    {
        Index = APPEUI;
    }
    else if (strcmp(Key, "APPKEY") == 0)
    {
        Index = APPKEY;
    }
    else if (strcmp(Key, "NETID") == 0)
    {
        Index = NETID;
    }
    else if (strcmp(Key, "DEVADDR") == 0)
    {
        Index = DEVADDR;
    }
    else if (strcmp(Key, "NWKSKEY") == 0)
    {
        Index = NWKSKEY;
    }
    else if (strcmp(Key, "APPSKEY") == 0)
    {
        Index = APPSKEY;
    }

    return Index;
}
int32_t EnvRead(const char *Name, char *Value)
{
    int32_t Ret = -1;
    DataIndex_t Index = ENDINDEX;
    uint8_t Hex[17] = {0};

    if (!Name || !Value)
    {
        Ret = -1;
        return Ret;
    }
    Index = GetIndex(Name);
    if (ENDINDEX != Index)
    {
        if (HAL_OK == HW_Read_CFG(Index, (void *)Hex))
        {
            switch(Index)
            {
                case DEVEUI:
                case APPEUI:
                    Hex2Str((unsigned char *)Hex, Value, 8);
                    break;
                case APPKEY:
                case NWKSKEY:
                case APPSKEY:
                    Hex2Str((unsigned char *)Hex, Value, 16);
                    break;
                case NETID:
                case DEVADDR:
                    Hex2Str((unsigned char *)Hex, Value, 4);
                    break;
                default:
                    break;
            }
            Ret = 0;
        }
    }
    return Ret;
}
int32_t EnvWrite(const char *Name, const char *Value)
{
    int32_t Ret = -1;
    uint8_t Hex[17] = {0};
    DataIndex_t Index = ENDINDEX;

    if (!Name || !Value)
    {
        Ret = -1;
        return Ret;
    }
    Index = GetIndex(Name);
    switch(Index)
    {
        case DEVEUI:
        case APPEUI:
            Str2Hex(Value, (unsigned char *)Hex, 8);
            break;
        case APPKEY:
        case NWKSKEY:
        case APPSKEY:
            Str2Hex(Value, (unsigned char *)Hex, 16);
            break;
        case NETID:
        case DEVADDR:
            Str2Hex(Value, (unsigned char *)Hex, 4);
            break;
        default:
            break;

    }
    if (ENDINDEX != Index)
    {
        if (HAL_OK == HW_Write_CFG(Index, (void *)Hex))
        {
            Ret = 0;
        }
    }
    return Ret;
}
