 /******************************************************************************
  * @file    env_eeprom.h
  * @author  joy
  * @version V1.0.0
  * @date    16-Nov-2016
  * @brief   Header for driver env_eeprom.c module
  ******************************************************************************
*/
#ifndef __ENV_EEPROM_H__
#define __ENV_EEPROM_H__

#ifdef __cplusplus
 extern "C" {
#endif

/**
* @brief              read val from eeprom
*
* @param name
* @param value
*
* @return 
*/
int32_t EnvRead(const char *Name, char *Value);
/**
* @brief              write conf to eeprom
*
* @param name
* @param value
*
* @return 
*/
int32_t EnvWrite(const char *Name, const char *Value);
/**
 * @brief 
 *
 * @param Str
 * @param Hex
 * @param Num
 */
void Str2Hex(const char *Str, unsigned char *Hex, int Num);
/**
 * @brief 
 *
 * @param Key
 *
 * @return 
 */
DataIndex_t GetIndex(const char *Key);

#ifdef 	__cplusplus
 }
#endif
#endif
