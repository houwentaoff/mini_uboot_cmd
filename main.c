/******************************************************************************
  * @file    main.c
  * @author  zhangsf
  * @version V1.0.0
  * @date    01-October-2016
  * @brief   this is the main!
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include "hw.h"
#include "low_power.h"
#include "lora.h"
#include "bsp.h"
#include "timeServer.h"
#include "vcom.h"
#include "version.h"
#include "cmd.h"
#include "config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define Shell_MainLoop(cb, para)          run_main_loop(cb, para)  


#define LOW_POWER_DISABLE

/*!
 * Defines the application data transmission duty cycle. 5s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            5000
/*!
 * LoRaWAN Adaptive Data Rate
 * @note Please note that when ADR is enabled the end-device should be static
 */
//#define LORAWAN_ADR_ON                              1
/*!
 * LoRaWAN confirmed messages
 */
#define LORAWAN_CONFIRMED_MSG                    DISABLE
/*!
 * LoRaWAN application port
 * @note do not use 224. It is reserved for certification
 */
//#define LORAWAN_APP_PORT                            2

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* call back when LoRa will transmit a frame*/
static void LoraTxData( lora_AppData_t *AppData, FunctionalState* IsTxConfirmed);

/* call back when LoRa has received a frame*/
static void LoraRxData( lora_AppData_t *AppData);

/* Private variables ---------------------------------------------------------*/
/* load call backs*/
static LoRaMainCallback_t LoRaMainCallbacks ={ HW_GetBatteryLevel,
                                               HW_GetUniqueId,
                                               HW_GetRandomSeed,
                                               LoraTxData,
                                               LoraRxData};

/*!
 * Specifies the state of the application LED
 */
static uint8_t AppLedStateOn = SET;
static TestMsg_t SendBuf;

/* !
 *Initialises the Lora Parameters
 */
static  LoRaParam_t LoRaParamInit= {TX_ON_TIMER,
                                    APP_TX_DUTYCYCLE,
                                    CLASS_A,
                                    CONFIG_ADR,
                                    DR_0,
                                    LORAWAN_PUBLIC_NETWORK };
void LoRa_MainLoop(void *para)
{
    (void)para;
#if 1    
    /* run the LoRa class A state machine*/
    Lora_Fsm( );
    
    DISABLE_IRQ( );
    /* if an interrupt has occurred after DISABLE_IRQ, it is kept pending 
     * and cortex will not enter low power anyway  */
    if ( lora_getDeviceState( ) == DEVICE_STATE_SLEEP )
    {
#ifndef LOW_POWER_DISABLE
      LowPower_Handler( );
#endif
    }
    ENABLE_IRQ();
#endif
}
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main( void )
{
	  
  /* STM32 HAL library initialization*/
  HAL_Init( );
  
  /* Configure the system clock to 32 MHz*/
  SystemClock_Config( );
  
  /* Configure the debug mode*/   //JTAG conf
  DBG_Init( );
  
  /* Configure the hardware*/
  HW_Init( );  
  /* initialize environment */
  Init_Env();
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */
  
  /* Configure the Lora Stack*/
  Lora_Init( &LoRaMainCallbacks, &LoRaParamInit);
  
  printf("run main\n");
  /* main loop*/
  while( 1 )
  {
#ifdef CONFIG_MINISHELL      
    Shell_MainLoop(LoRa_MainLoop, NULL);
#else
    /* run the LoRa class A state machine*/
    Lora_Fsm( );
    
    DISABLE_IRQ( );
    /* if an interrupt has occurred after DISABLE_IRQ, it is kept pending 
     * and cortex will not enter low power anyway  */
    if ( lora_getDeviceState( ) == DEVICE_STATE_SLEEP )
    {
#ifndef LOW_POWER_DISABLE
      LowPower_Handler( );
#endif
    }
    ENABLE_IRQ();
    
    /* USER CODE BEGIN 2 */
    /* USER CODE END 2 */
#endif
  }
}


static void LoraTxData( lora_AppData_t *AppData, FunctionalState* IsTxConfirmed)
{
  /* USER CODE BEGIN 3 */
  uint16_t pressure = 0;
  int16_t temperature = 0;
  uint16_t humidity = 0;
  uint8_t batteryLevel;
  sensor_t sensor_data;
  char *Value = NULL;
//  int32_t latitude, longitude = 0;
 // uint16_t altitudeGps = 0;
  BSP_sensor_Read( &sensor_data );
  temperature = ( uint16_t )( sensor_data.temperature * 100 );     /* in 癈 * 100 */
  pressure    = ( uint16_t )( sensor_data.pressure * 100 / 10 );  /* in hPa / 10 */
  humidity    = ( uint16_t )( sensor_data.humidity * 10 );        /* in %*10     */
 // latitude = sensor_data.latitude;
 // longitude= sensor_data.longitude;
  uint32_t i = 0;

  batteryLevel = HW_GetBatteryLevel( );                     /* 1 (very low) to 254 (fully charged) */
#if 0
  PRINTF("temperature=%d,%d degC\n", temperature/100, temperature-(temperature/100)*100);
  PRINTF("pressure=%d hPa\n", pressure/10);
  PRINTF("humidity=%d,%d %%\n", humidity/10, humidity-(humidity/10)*10);
  PRINTF("batteryLevel=%d %%\n", batteryLevel*100/255);
#endif
#ifdef CONFIG_MINISHELL
  Value = getenv("port");
  if (!Value)
  {
    AppData->Port = CONFIG_PORT;
  }
  else
  {
    AppData->Port = atoi(Value)&0xff;
  }
#else
    AppData->Port = CONFIG_PORT;
#endif
  *IsTxConfirmed =  LORAWAN_CONFIRMED_MSG;

#if defined( USE_BAND_868 )
  AppData->Buff[i++] = AppLedStateOn;
  AppData->Buff[i++] = ( pressure >> 8 ) & 0xFF;
  AppData->Buff[i++] = pressure & 0xFF;
  AppData->Buff[i++] = ( temperature >> 8 ) & 0xFF;
  AppData->Buff[i++] = temperature & 0xFF;
  AppData->Buff[i++] = ( humidity >> 8 ) & 0xFF;
  AppData->Buff[i++] = humidity & 0xFF;
  AppData->Buff[i++] = batteryLevel;
  AppData->Buff[i++] = ( latitude >> 16 ) & 0xFF;
  AppData->Buff[i++] = ( latitude >> 8 ) & 0xFF;
  AppData->Buff[i++] = latitude & 0xFF;
  AppData->Buff[i++] = ( longitude >> 16 ) & 0xFF;
  AppData->Buff[i++] = ( longitude >> 8 ) & 0xFF;
  AppData->Buff[i++] = longitude & 0xFF;
  AppData->Buff[i++] = ( altitudeGps >> 8 ) & 0xFF;
  AppData->Buff[i++] = altitudeGps & 0xFF;
#elif defined( USE_BAND_915 ) || defined( USE_BAND_915_HYBRID )
  AppData->Buff[i++] = AppLedStateOn;
  AppData->Buff[i++] = ( pressure >> 8 ) & 0xFF;
  AppData->Buff[i++] = pressure & 0xFF;
  AppData->Buff[i++] = ( temperature >> 8 ) & 0xFF;
  AppData->Buff[i++] = temperature & 0xFF;
  AppData->Buff[i++] = ( humidity >> 8 ) & 0xFF;
  AppData->Buff[i++] = humidity & 0xFF;
  AppData->Buff[i++] = batteryLevel;
  AppData->Buff[i++] = 0;
  AppData->Buff[i++] = 0;
  AppData->Buff[i++] = 0;
#elif defined(USE_BAND_433)|| defined( USE_BAND_470 )

#ifdef PRESSURE
	AppData->Buff[i++] = 01;
	pressure = 0x1234;
  AppData->Buff[i++] = ( pressure >> 8 ) & 0xFF;
  AppData->Buff[i++] = pressure & 0xFF;
#endif
  if (SendBuf.total > 0 && SendBuf.sended < SendBuf.total)
  {
    int count = 0;
    for (count=0; count <(SendBuf.blockSize/4) && SendBuf.sended < SendBuf.total; count++)
    {
        *(unsigned int*)&AppData->Buff[i] = SendBuf.sended+1;        
        SendBuf.sended += 4;
        if (SendBuf.sended > SendBuf.total)
        {
            SendBuf.sended = SendBuf.total;
            if (SendBuf.total % 4 != 0)
            {
                i += SendBuf.total % 4;
                break;
            }
        }
        i += 4;
    }
  }
  
  //AppData->Buff[i++] = 0x2;
  //AppData->Buff[i++] = 0x2;

#ifdef TEMPERATURE
temperature = 0x5678;
// if testmsg open 一次加64个字节
  AppData->Buff[i++] = ( temperature >> 8 ) & 0xFF;
  AppData->Buff[i++] = temperature & 0xFF;
#endif
#ifdef HUMIDITY
	humidity = 0x90a0;
  AppData->Buff[i++] = ( humidity >> 8 ) & 0xFF;
  AppData->Buff[i++] = humidity & 0xFF;
#endif
#ifdef BATTERY
	batteryLevel = 0xFF;
  AppData->Buff[i++] = batteryLevel;
#endif
#ifdef GPS
	latitude = 0x0bd9;
	
  AppData->Buff[i++] = ( latitude >> 16 ) & 0xFF;
  AppData->Buff[i++] = ( latitude >> 8 ) & 0xFF;
  AppData->Buff[i++] = latitude & 0xFF;
	
	longitude =0X28a4;
	
  AppData->Buff[i++] = ( longitude >> 16 ) & 0xFF;
  AppData->Buff[i++] = ( longitude >> 8 ) & 0xFF;
  AppData->Buff[i++] = longitude & 0xFF;
	
	altitudeGps = 0x01F4; 
  AppData->Buff[i++] = ( altitudeGps >> 8 ) & 0xFF;
  AppData->Buff[i++] = altitudeGps & 0xFF;

#endif


#endif
  AppData->BuffSize = i;
  
  /* USER CODE END 3 */
}
    
static void LoraRxData( lora_AppData_t *AppData )
{
  /* USER CODE BEGIN 4 */
  char *Value = NULL;
  uint8_t Port = 0;
#ifdef CONFIG_MINISHELL
  Value = getenv("port");
  if (!Value)
  {
    Port = CONFIG_PORT;
  }
  else 
  {
    Port = atoi(Value)&0xff;
  }
#else
  Port = CONFIG_PORT;
#endif
	PRINTF("-------------RCV Data");
  if (AppData->Port == Port)
  {
#if 0
      if( AppData->BuffSize == 1 )
      {
        AppLedStateOn = AppData->Buff[0] & 0x01;
        if ( AppLedStateOn == RESET )
        {
          PRINTF("LED OFF\n");
        }
        else
        {
          PRINTF("LED ON\n");
        }
        //GpioWrite( &Led3, ( ( AppLedStateOn & 0x01 ) != 0 ) ? 0 : 1 );
      }
#endif
			AppLedStateOn = !(AppLedStateOn);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, AppLedStateOn);
			if ( AppLedStateOn == RESET )
      {
         PRINTF("LED ON\n");
      }
      else
      {
				 PRINTF("LED OFF\n");
      }
  }
  /* USER CODE END 4 */
}
TestMsg_t *GetTestMsg()
{

    return &SendBuf;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
