 /******************************************************************************
  * @file    hw_eeprom.c
  * @author  Ren wenlong
  * @version V1.0.1
  * @date    20-November-2016
  * @brief   driver hw_eeprom.c module
  ******************************************************************************
	*/
	
/* Includes ------------------------------------------------------------------*/

#include "hw.h"
#include "hw_eeprom.h"
#include <stdlib.h>

#define LOG_INIT_FLAG   0xA5   //Log area init flag


static uint8_t Log_Num;           //Total log number
static uint8_t Log_ReadIndex;     //Read log index that indicate the position log stored
static uint8_t Log_StoreIndex;    //log store index that indicate the position log shoud be stored
static uint32_t Log_SeqNum;       //log index 
/*
*配置参数的信息表，{Index,Datatype,Length}每一个配置信息的长度Length不能超过16字节，最好能4字节对齐
*
*/
static struct CFGStruct LoraCFGMap[ENDINDEX] = {
        {DEVEUI,BYTE,8},
        {APPEUI,BYTE,8},
        {APPKEY,BYTE,16},
        {NETID,BYTE,4},
        {DEVADDR,BYTE,4},
        {NWKSKEY,BYTE,16},
        {APPSKEY,BYTE,16}
};




/*!
 * \brief Indicates if the RTC is already Initalized or not
 */
static bool HW_EEPROM_Initalized = false;

/* Private function prototypes -----------------------------------------------*/
/*!
 * @brief Initializes the RTC timer
 * @note The timer is based on the RTC
 * @param none
 * @retval none
 */
static void HW_EEPROM_LoadMsg(void)
{

	// Load EEPROM CFG msg 


};
/*!
 * @brief Initializes the eeprom
 * @note The init is based on the EEPROM
 * @param none
 * @retval none
 */
void HW_EEprom_Init( void )
{
	
	if (HW_EEPROM_Initalized == false)
	{
	    HW_EEPROM_Enable();
	    HW_EEPROM_LoadMsg();
	    HW_Init_Log();
	    FLASH_WaitForLastOperation(0);
	    HW_EEPROM_Initalized = true;
	}
}


HAL_StatusTypeDef  HW_EEPROM_Enable(void )
{
	if (HAL_OK != HAL_FLASHEx_DATAEEPROM_Unlock())
	{
		return HAL_ERROR;
	
	}
	else
	{
	
		return HAL_OK;
	}

}


HAL_StatusTypeDef HW_EEPROM_Disable(void)
{

	if (HAL_OK != HAL_FLASHEx_DATAEEPROM_Lock())
	{
		return HAL_ERROR;
	
	}
	else
	{
	
		return HAL_OK;
	};
}


HAL_StatusTypeDef HW_EEPROM_ReadBytes(DataEEpromContext_t *EEpromData)
{

	uint16_t offSet; 
	DataType_t   dataType;
	uint8_t *wAddr;           // READ byte every times 
	uint32_t Length;
	uint8_t *Data;
	offSet = EEpromData->Offset;
	dataType = EEpromData->Datatype;
	Data = (uint8_t *)EEpromData->Data;
	Length = EEpromData->Length;

	if((Length + offSet > EEPROM_BYTE_SIZE) || (offSet > EEPROM_BYTE_SIZE))
    {
        return HAL_ERROR;
    }
			
	if ((dataType != BYTE)||(EEpromData == NULL))
	{
			
		return HAL_ERROR;
			
	}
    DISABLE_IRQ();
	
    wAddr=(uint8_t *)(EEPROM_DATA_ADDR + offSet); 
		
    while(Length--)
    {
			
        *Data++=*wAddr++;
			
    }   
    ENABLE_IRQ();
    
    return HAL_OK;

}



HAL_StatusTypeDef HW_EEPROM_ReadWords(DataEEpromContext_t *EEpromData) {

	uint16_t offSet; 
	uint32_t *wAddr;           // READ word every times 
	uint32_t Length;
	offSet = EEpromData->Offset;
	Length = EEpromData->Length;
		
    if((Length + offSet > EEPROM_BYTE_SIZE) || (offSet > EEPROM_BYTE_SIZE))
    {
        return HAL_ERROR;
    }
    
    DISABLE_IRQ();
    wAddr=(uint32_t *)(EEPROM_DATA_ADDR + offSet);  
    while(Length--)
    {
			
        *EEpromData->Data++=*wAddr++;
			
    }   
    ENABLE_IRQ();
    
    return HAL_OK;


}


HAL_StatusTypeDef HW_EEPROM_WriteBytes(DataEEpromContext_t *EEpromData)
{
	
	uint32_t TypeProgram = FLASH_TYPEPROGRAMDATA_BYTE;
	uint16_t offSet; 
	DataType_t   dataType;
	uint32_t Address;           // READ word every times 
	uint32_t Length;
	uint8_t *Data;
	offSet = EEpromData->Offset;
	dataType = EEpromData->Datatype;
	Data = (uint8_t *)EEpromData->Data;
	Length = EEpromData->Length;

	
	if(dataType != TypeProgram  )
	{
		return HAL_ERROR;
	}

    if((Length + offSet > EEPROM_BYTE_SIZE) || (offSet > EEPROM_BYTE_SIZE))
    {
        return HAL_ERROR;
    }
	Address  = (uint32_t )(EEPROM_DATA_ADDR + offSet); 
     DISABLE_IRQ();
	while(Length--)
	{
		
	    if(HAL_OK != HAL_FLASHEx_DATAEEPROM_Program(TypeProgram, Address, *Data))
	    {
             ENABLE_IRQ();
	        return HAL_ERROR;
	    }
	    else
	    {
			Address++;
			Data++;
			
	    }
	}
    ENABLE_IRQ();
    
    return HAL_OK;

}

HAL_StatusTypeDef HW_EEPROM_WriteWords(DataEEpromContext_t *EEpromData) {

	
	uint32_t TypeProgram = FLASH_TYPEPROGRAMDATA_WORD;
	uint16_t offSet; 
	DataType_t   dataType;
	uint32_t Address,Length;           // READ word every times 
	uint32_t *Data;
	offSet = EEpromData->Offset;
	dataType = EEpromData->Datatype;
	Length = EEpromData->Length;
	Data = EEpromData->Data;
	
	if(dataType != TypeProgram  )
	{
	
		return HAL_ERROR;
	}

    if((Length + offSet > EEPROM_BYTE_SIZE) || (offSet > EEPROM_BYTE_SIZE))
    {
        return HAL_ERROR;
    }
	Address  = (uint32_t )(EEPROM_DATA_ADDR + offSet); 
    DISABLE_IRQ();
	FLASH_WaitForLastOperation(0);
	
	while(Length--)
	{
		
		if(HAL_OK != HAL_FLASHEx_DATAEEPROM_Program(TypeProgram, Address, (uint32_t)*Data))
		{
            ENABLE_IRQ();
			return HAL_ERROR;
		}
		else
		{
			Address += sizeof(uint32_t);
			Data++;
			
		}
	}
    ENABLE_IRQ();
    
    return HAL_OK;
}

HAL_StatusTypeDef HW_EEPROM_Erase(DataEEpromContext_t *EEpromData){
		
	uint16_t offSet; 
	DataType_t   dataType;
	uint32_t Address;           // READ word every times 
	uint32_t Length;
	offSet = EEpromData->Offset;
	dataType = EEpromData->Datatype;
	Length = EEpromData->Length;
	
	if((Length + offSet > EEPROM_BYTE_SIZE) || (offSet > EEPROM_BYTE_SIZE))
    {
        return HAL_ERROR;
    }
	Address  = (uint32_t )(EEPROM_DATA_ADDR + offSet); 
    DISABLE_IRQ();
    FLASH_WaitForLastOperation(0);

    //printf("Erase Address:%X\n",Address);
	while(Length--)
	{
	    if(HAL_OK== HAL_FLASHEx_DATAEEPROM_Erase(dataType, Address))
	    {
		
		    Address += (1 <<dataType);
	    }
	    else
	    {
            ENABLE_IRQ();
		    return HAL_ERROR;
	    }
	}
    ENABLE_IRQ();
	return HAL_OK;

}


HAL_StatusTypeDef HW_Write_CFG(DataIndex_t Index,void *Buf)
{
    DataEEpromContext_t CFG;
    void *pTmpBuf = NULL;

    if((LoraCFGMap[Index].Length % 4) != 0)   // insure four byte alignment
    {
        pTmpBuf = (void *)malloc((LoraCFGMap[Index].Length / 4) * 4 + 4);
        if(pTmpBuf == NULL)
        {
            return HAL_ERROR;
        }
        memset(pTmpBuf,0,(LoraCFGMap[Index].Length / 4) * 4 + 4);
        memcpy(pTmpBuf,Buf,LoraCFGMap[Index].Length);
        CFG.Length = (LoraCFGMap[Index].Length / 4) + 1;
        CFG.Data = pTmpBuf;
    }
    else
    {
        CFG.Length = LoraCFGMap[Index].Length / 4;
        CFG.Data = Buf;
    }
    
    CFG.Datatype = WORD;
    CFG.Offset = Index * EEPROM_INTERVAL + EEPROM_CFG_ADDR;
    if(HW_EEPROM_WriteWords(&CFG) != HAL_OK) 
    {
        if(pTmpBuf)
        {
            free(pTmpBuf);
        }
        return HAL_ERROR;
    }
    CFG.Offset = Index * EEPROM_INTERVAL + EEPROM_CFG_ADDR + CFG_BAKEUP_OFFSET;
    if(HW_EEPROM_WriteWords(&CFG) != HAL_OK) 
    {
        if(pTmpBuf)
        {
            free(pTmpBuf);
        }
        return HAL_ERROR;
    }

    if(pTmpBuf)
    {
        free(pTmpBuf);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef HW_Read_CFG(DataIndex_t Index,void *Buf)
{
    DataEEpromContext_t CFG;

    if(LoraCFGMap[Index].Length % 4 != 0)  // insure four byte alignment
    {
        CFG.Length = LoraCFGMap[Index].Length / 4 + 1;
    }
    else
    {
        CFG.Length = LoraCFGMap[Index].Length / 4;
    }
    CFG.Data = (uint32_t *)Buf;
    CFG.Datatype = WORD;
    CFG.Offset = Index * EEPROM_INTERVAL + EEPROM_CFG_ADDR;
    if(HW_EEPROM_ReadWords(&CFG) != HAL_OK)
    {
        CFG.Offset = Index * EEPROM_INTERVAL + EEPROM_CFG_ADDR + CFG_BAKEUP_OFFSET;
        if(HW_EEPROM_ReadWords(&CFG) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef HW_Init_Log(void)
{
    DataEEpromContext_t CFG;
    Log_t InitLog;

    CFG.Length = sizeof(Log_t) / sizeof(uint32_t);
    CFG.Data = (uint32_t *)&InitLog;
    CFG.Datatype = WORD;
    CFG.Offset = EEPROM_LOG_ADDR;

    if(HW_EEPROM_ReadWords(&CFG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if((InitLog.Data[0] != LOG_INIT_FLAG) || (InitLog.Data[1] != LOG_INIT_FLAG))
    {
        //InitLog.LogIndex = LOG_INIT_FLAG;

        InitLog.Data[0] = LOG_INIT_FLAG;
        InitLog.Data[1] = LOG_INIT_FLAG;
        InitLog.Data[2] = 0;
        InitLog.Data[3] = 1;
        InitLog.Data[4] = 1;
        *((uint32_t *)(&InitLog.Data[12])) = 1;

        CFG.Length = sizeof(Log_t) / sizeof(uint32_t);
        CFG.Data = (uint32_t *)&InitLog;
        CFG.Datatype = WORD;
        CFG.Offset = EEPROM_LOG_ADDR;

        if(HW_EEPROM_WriteWords(&CFG) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }

    Log_Num = InitLog.Data[2];
    Log_StoreIndex = InitLog.Data[4];
    Log_ReadIndex = Log_StoreIndex;

    Log_SeqNum = *((uint32_t *)(&InitLog.Data[12]));

    return HAL_OK;
}

HAL_StatusTypeDef HW_Store_LogInfo(void)
{
    DataEEpromContext_t CFG;
    Log_t InitLog;
    
    InitLog.Data[0] = LOG_INIT_FLAG;
    InitLog.Data[1] = LOG_INIT_FLAG;
    InitLog.Data[2] = Log_Num;
    InitLog.Data[3] = Log_ReadIndex;
    InitLog.Data[4] = Log_StoreIndex;
    *((uint32_t *)(&InitLog.Data[12])) = Log_SeqNum;

    CFG.Length = sizeof(Log_t) / sizeof(uint32_t);
    CFG.Data = (uint32_t *)&InitLog;
    CFG.Datatype = WORD;
    CFG.Offset = EEPROM_LOG_ADDR;

    if(HW_EEPROM_WriteWords(&CFG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef HW_Write_Log(uint8_t *Buf)
{
    DataEEpromContext_t CFG;
    Log_t CurLog;
    uint8_t LogLen;

    //CurLog.Date
    LogLen = strlen((char *)Buf);
    if(LogLen < 1)
    {
        return HAL_ERROR;
    }
    if(LogLen > sizeof(CurLog.Data) - 9)
    {
        LogLen = sizeof(CurLog.Data) - 9;
    }
    //CurLog.LogIndex = 0;
    snprintf((char *)CurLog.Data,9,"%8d",Log_SeqNum);
    memcpy(&CurLog.Data[8],Buf,LogLen);
    CurLog.Data[sizeof(CurLog.Data) - 1] = '\0';

    CFG.Length = sizeof(Log_t) / sizeof(uint32_t);
    CFG.Data = (uint32_t *)&CurLog;
    CFG.Datatype = WORD;
    CFG.Offset = Log_StoreIndex * EEPROM_INTERVAL + EEPROM_LOG_ADDR;

    if(HW_EEPROM_WriteWords(&CFG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    Log_StoreIndex++;
    if(Log_StoreIndex > LOG_MAX_NUM)
    {
        Log_StoreIndex = 1;
    }

    Log_Num++;
    if(Log_Num > LOG_MAX_NUM)
    {
        Log_Num = LOG_MAX_NUM;
    }

    Log_SeqNum++;

    HW_Store_LogInfo();
    //LOG_MAX_NUM

    return HAL_OK;
}

HAL_StatusTypeDef HW_Read_Log(uint8_t *Buf,uint8_t Index)
{
    DataEEpromContext_t CFG;
    
    
    if(Buf == NULL)
    {
        return HAL_ERROR;
    }
    if(Index == 0)
    {
        *Buf = Log_Num;
        return HAL_OK;
    }
    if(Index > Log_Num)
    {
        return HAL_ERROR;
    }

    
    Log_ReadIndex = (Log_StoreIndex + LOG_MAX_NUM + Index) - 2 - Log_Num;
    Log_ReadIndex = (Log_ReadIndex % LOG_MAX_NUM) + 1;

    CFG.Length = sizeof(Log_t) / sizeof(uint32_t);
    CFG.Data = (uint32_t *)Buf;
    CFG.Datatype = WORD;
    CFG.Offset = Log_ReadIndex * EEPROM_INTERVAL + EEPROM_LOG_ADDR;

    if(HW_EEPROM_ReadWords(&CFG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
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
int32_t E2promBusErase(unsigned int DevAddr, unsigned int Offset,
			   unsigned int Cnt)
{
    DataEEpromContext_t EEpromData;
    (void)DevAddr;
    
    EEpromData.Datatype = WORD;
    EEpromData.Offset   = Offset;
    EEpromData.Length   = Cnt/4;
    if (HAL_OK != HW_EEPROM_Erase(&EEpromData))
    {
        return -1;
    }
    return 0;
}

int32_t E2promBusRead(unsigned int DevAddr, unsigned int Offset,
			   unsigned char *Buffer, unsigned int Cnt)
{
    DataEEpromContext_t EEpromData;
    (void)DevAddr;
    
    EEpromData.Datatype = WORD;
    EEpromData.Offset   = Offset;
    EEpromData.Length   = Cnt/4;
    EEpromData.Data     = (uint32_t *)Buffer;
    if (HAL_OK != HW_EEPROM_ReadWords(&EEpromData))
    {
        return -1;
    }
    return 0;
}
int32_t E2promBusWrite(unsigned int DevAddr, unsigned int Offset,
			    unsigned char *Buffer, unsigned int Cnt)
{
    DataEEpromContext_t EEpromData;
    (void)DevAddr;

    EEpromData.Datatype = WORD;
    EEpromData.Offset   = Offset;
    EEpromData.Length   = Cnt/4;
    EEpromData.Data     = (uint32_t *)Buffer;
    
    if (HAL_OK != HW_EEPROM_WriteWords(&EEpromData))
    {
        return -1;
    }
    return 0;
}

