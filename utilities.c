/******************************************************************************
  * @file    utilities.c
  * @author  Zhangsf
  * @version V1.0.0
  * @date    01-October-2016
  * @brief   is the Utilities
  ******************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include "utilities.h"
#include "hw_eeprom.h"

static const char HexTable[]="0123456789ABCDEF";
static const char Sep = ':';
/*!
 * Redefinition of rand() and srand() standard C functions.
 * These functions are redefined in order to get the same behavior across
 * different compiler toolchains implementations.
 */
// Standard random functions redefinition start
#define RAND_LOCAL_MAX 2147483647L

static uint32_t next = 1;

int32_t rand1( void )
{
    return ( ( next = next * 1103515245L + 12345L ) % RAND_LOCAL_MAX );
}

void srand1( uint32_t seed )
{
    next = seed;
}
// Standard random functions redefinition end

int32_t randr( int32_t min, int32_t max )
{
    return ( int32_t )rand1( ) % ( max - min + 1 ) + min;
}

void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    while( size-- )
    {
        *dst++ = *src++;
    }
}

void memcpyr( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    dst = dst + ( size - 1 );
    while( size-- )
    {
        *dst-- = *src++;
    }
}

void memset1( uint8_t *dst, uint8_t value, uint16_t size )
{
  while( size-- )
  {
    *dst++ = value;
  }
}

int8_t Nibble2HexChar( uint8_t a )
{
  if( a < 10 )
  {
    return '0' + a;
  }
  else if( a < 16 )
  {
    return 'A' + ( a - 10 );
  }
  else
  {
    return '?';
  }
}

unsigned long SimpleStrtoul(const char *cp, char **endp,
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

void Hex2Str(const unsigned char *Hex, char *Str, int Num)
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
