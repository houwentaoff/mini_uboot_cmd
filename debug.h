 /******************************************************************************
  * @file    debug.h
  * @author  Zhangsf
  * @version V1.0.0
  * @date    01-October-2016
  * @brief   Header for driver debug.c module
  ******************************************************************************
	*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/



#include <string.h>
#include <stdio.h>
#include "hw_conf.h"
#include "vcom.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */ 

void DBG_Init( void );

void Error_Handler( void );

#ifdef DEBUG

#define DBG_GPIO_WRITE( gpio, n, x )  HAL_GPIO_WritePin( gpio, n, (GPIO_PinState)(x) )

#define DBG_GPIO_SET( gpio, n )       gpio->BSRR = n

#define DBG_GPIO_RST( gpio, n )       gpio->BSRR = n<<16 

#define DBG_RTC_OUTPUT RTC_OUTPUT_DISABLE; /* RTC_OUTPUT_ALARMA on PC13 */

#define DBG( x )  do{ x } while(0)

#ifdef TRACE

#define DBG_PRINTF(...)    vcom_Send(__VA_ARGS__)

#else /*TRACE*/

#define DBG_PRINTF(...) 

#endif /*TRACE*/


#else /* DEBUG */

#define DBG_GPIO_WRITE( gpio, n, x )  

#define DBG_GPIO_SET( gpio, n )       

#define DBG_GPIO_RST( gpio, n )      

#define DBG( x ) do{  } while(0)

#define DBG_PRINTF(...) 
                      
#define DBG_RTC_OUTPUT RTC_OUTPUT_DISABLE;

#endif /* DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_H__*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
