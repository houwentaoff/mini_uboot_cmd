 /******************************************************************************
  * @file    hw_eeprom.h
  * @author  Zhangsf
  * @version V1.0.0
  * @date    31-October-2016
  * @brief   Header for driver hw_eeprom.c module
  ******************************************************************************
	*/


#ifndef __HW_EEPROM_H__
#define __HW_EEPROM_H__


#ifdef __cplusplus
 extern "C" {
#endif

#include "utilities.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/*  EEPROM 4096 kbytes    ONE word == uint32_t*/	  
#define EEPROM_BASIC_ADDR    ((uint32_t)0x08080000) 

//#define EEPROM_INDEX_ADDR    ((uint32_t)0x08080000)	/* 1K */  
#define EEPROM_DATA_ADDR     ((uint32_t)0x08080000) /* 4k */

#define EEPROM_CFG_ADDR      ((uint32_t)0x08080000)   /* 1K */ 
#define EEPROM_LOG_ADDR      ((uint32_t)0x08080400)   /* 1K */ 
	 
	 
#define EEPROM_DATA_ENDADDR   ((uint32_t)0x08080FFF)
	 
	 
#define EEPROM_BYTE_SIZE     0x0FFF             /* 4k */
//------------store interval ---------
#define EEPROM_INTERVAL  sizeof(uint32_t)*8     /* data and  backup  data */

#define CFG_BAKEUP_OFFSET (EEPROM_INTERVAL / 2)

/*----------------------all data 4 bytes aligned-----------------------*/	
 /* 
	* FUNC: the data type for  program 
	*
	*/	 
typedef enum eDataType{
	BYTE,           /*  one byte */
	HALF_WORD,			/*  2 bytes */
	WORD						/*  4 bytes */
}	DataType_t;

/*
*               ----------------------------------
*								|   INDEX     |    DATAVALUE		|
*								|		DEVaddr		|    0x010203			|
*								...............................
*						
*				4K eeprom ,the first 1k for store index, 
*					the second 1k for store data,
*/


typedef enum eDataIndex{
	// Note : net configure msg 
	DEVEUI ,   // 8 BYTES 
	APPEUI,			  // 8 BYTES
	APPKEY,       // 16 BYTES
	NETID ,      // 4 BYTES
	DEVADDR,		//4 BYTES
	NWKSKEY,		//16 BYTES
	APPSKEY, 		//16 BYTES

	// Note : Lora configure msg 
	
	ENDINDEX

} DataIndex_t;

struct CFGStruct{            //Config struct

    DataIndex_t Index;
    DataType_t  DataType;
    uint32_t    Length;      //Length in byte
};



/*----------------------Abaut Log-----------------------*/

#define LOG_MAX_NUM      15           //the maximum log items



typedef struct Log{                  //log struct
    uint8_t Data[32];
}Log_t;
        
/*----------------------all data 4 bytes aligned-----------------------*/	
 /* 
	* FUNC: the data type for  program 
	*	
	*/	 
	 
typedef struct DataEEpromContext{
	
	DataType_t Datatype;     /* data type */
	
	uint32_t  Offset;    /* eeprom address offset */
	
	uint32_t     *Data ;	/* data */
	
	uint32_t Length;    /* the length of data */
	

}DataEEpromContext_t;




/* Private function prototypes -----------------------------------------------*/
/*!
 * @brief Initializes the EEprom
 * @note 
 * @param none
 * @retval none
 */
void HW_EEprom_Init( void );

/*!
 * @brief Unlocks the data memory and FLASH_PECR register access.
 * @param none
 * @retval HAL_StatusTypeDef HAL Status
 */
HAL_StatusTypeDef  HW_EEPROM_Enable(void );

/**
  * @brief  Read Byte at a specified address
  * @note   To correctly run this function, the HAL_FLASH_EEPROM_Unlock() function
  *         must be called before.
  *         Call the EEPROM_Enable() to the data EEPROM access
  *         and Flash program erase control register access(recommended to protect 
  *         the DATA_EEPROM against possible unwanted operation).
  * @param  specifies the struct of data to be programmed.
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HW_EEPROM_ReadBytes(DataEEpromContext_t *EEpromData)  ;

/**
  * @brief  Read word at a specified address
  * @note   To correctly run this function, the HAL_FLASH_EEPROM_Unlock() function
  *         must be called before.
  *         Call the EEPROM_Enable() to he data EEPROM access
  *         and Flash program erase control register access(recommended to protect 
  *         the DATA_EEPROM against possible unwanted operation).
  * @param  specifies the struct of data to be programmed.
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HW_EEPROM_ReadWords(DataEEpromContext_t *EEpromData) ;

/**
  * @brief  program byte at a specified address
  * @note   To correctly run this function, the HAL_FLASH_EEPROM_Unlock() function
  *         must be called before.
  *         Call the EEPROM_Enable() to he data EEPROM access
  *         and Flash program erase control register access(recommended to protect 
  *         the DATA_EEPROM against possible unwanted operation).
  * @param  specifies the struct of data to be programmed.
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HW_EEPROM_WriteBytes(DataEEpromContext_t *EEpromData)  ;

/**
  * @brief  Program word at a specified address
  * @note   To correctly run this function, the HAL_FLASH_EEPROM_Unlock() function
  *         must be called before.
  *         Call the EEPROM_Enable() to he data EEPROM access
  *         and Flash program erase control register access(recommended to protect 
  *         the DATA_EEPROM against possible unwanted operation).
  * @param  specifies the struct of data to be programmed.
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HW_EEPROM_WriteWords(DataEEpromContext_t *EEpromData) ;

/**
  * @brief  Erase at a specified address
  * @note   To correctly run this function, the HAL_FLASH_EEPROM_Unlock() function
  *         must be called before.
  *         Call the EEPROM_Enable() to he data EEPROM access
  *         and Flash program erase control register access(recommended to protect 
  *         the DATA_EEPROM against possible unwanted operation).
  * @param  specifies the struct of data to be programmed.
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HW_EEPROM_Erase(DataEEpromContext_t *EEpromData)  ;


/*!
 * @brief Locks the Data memory and FLASH_PECR register access.
 * @param none
 * @retval HAL_StatusTypeDef HAL Status
 */
HAL_StatusTypeDef EEPROM_Disable(void);

/**
  * @brief  Initializes the Log sysytem
  * @note   none
  * @param  none
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */

HAL_StatusTypeDef HW_Init_Log(void);

/**
  * @brief  store the system config value
  * @note   none
  * @param  Index:DataIndex_t
  *         Buf:buffer that used to store config value
  * @retval HAL_StatusTypeDef HAL Status
  */

HAL_StatusTypeDef HW_Write_CFG(DataIndex_t Index,void *Buf);

/**
  * @brief  read the system config value
  * @note   none 
  * @param  Index:DataIndex_t
  *         Buf:buffer that used to store config value
  * @retval HAL_StatusTypeDef HAL Status
  */

HAL_StatusTypeDef HW_Read_CFG(DataIndex_t Index,void *Buf);
/**
  * @brief  write a system log
  * @note   none
  * @param  
  *         Buf:buffer that store log string
  * @retval HAL_StatusTypeDef HAL Status
  */

HAL_StatusTypeDef HW_Write_Log(uint8_t *Buf);

/**
  * @brief  read a system log
  * @note   none
  * @param  Index:value between 1-31
  *         Buf:buffer that used to store log string
  * @retval HAL_StatusTypeDef HAL Status
  */

HAL_StatusTypeDef HW_Read_Log(uint8_t *Buf,uint8_t Index);

/**
* @brief              read val from eeprom (legacy interface)
*
* @param name         env var name
* @param value        env var value
*
* @return 
*/
int32_t EnvRead(const char *Name, char *Value);
/**
* @brief           write conf to eeprom (legacy interface)
*
* @param name      env config name   
* @param value     env config value
*
* @return 
*/
int32_t EnvWrite(const char *Name, const char *Value);
/**
 * @brief          get data index from key
 *
 * @param Key      var key
 *
 * @return         key type index
 */
DataIndex_t GetIndex(const char *Key);
/**
 * @brief              read data from eeprom 
 *
 * @param DevAddr      the eeprom addr
 * @param Offset       the offset of eeprom
 * @param Buffer       buffer from caller
 * @param Cnt          the num of bytes you want to read
 *
 * @return             0:success -1:fail
 */
int32_t E2promBusRead(unsigned int DevAddr, unsigned int Offset,
			   unsigned char *Buffer, unsigned int Cnt);
/**
 * @brief              write data to eeprom 
 *
 * @param DevAddr      the eeprom addr
 * @param Offset       the offset of eeprom
 * @param Buffer       the buffer you want to write
 * @param Cnt          the num of bytes you want to write 
 *
 * @return             0:success -1:fail
 */
int32_t E2promBusWrite(unsigned int DevAddr, unsigned int Offset,
			    unsigned char *Buffer, unsigned int Cnt);

/**
 * @brief              erase  eeprom 
 *
 * @param DevAddr      the eeprom addr
 * @param Offset       the offset of eeprom
 * @param Cnt          the num of bytes you want to erase 
 *
 * @return             0:success -1:fail
 */
int32_t E2promBusErase(unsigned int DevAddr, unsigned int Offset,
			    unsigned int Cnt); 
#ifdef 	__cplusplus
}
#endif

#endif
 
