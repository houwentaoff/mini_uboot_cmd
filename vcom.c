 /******************************************************************************
  * @file    vcom.c
  * @author  Zhangsf
  * @version V1.0.0
  * @date    01-October-2016
  * @brief   manages virtual com port
  ******************************************************************************
  */
  
//#include "hw.h"
#include "stm32l1xx_hal.h"
#include "stm32l1xx_hal_def.h"
#include "vcom.h"
#include "hw_usart.h"
#include <stdarg.h>


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define BUFSIZE 512

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static UART_HandleTypeDef UartHandle;

/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
void vcom_Init(void)
{
  Porting_Uart_Init(115200);
}


void vcom_DeInit(void)
{
  Porting_Uart_DeInit();
}

void vcom_Send( const char *format, ... )
{
  va_list args;
  char buff[BUFSIZE]={0};
  uint16_t count = 0;

  va_start(args, format);
  count = vsprintf(&buff[0], format, args);
  Porting_Uart_Send(buff, count);
  va_end(args);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
