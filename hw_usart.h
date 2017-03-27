/******************************************************************************
 * @file    hw_usart.h
 * @author  Joy
 * @version V1.0.0
 * @date    09-Nov-2016
 * @brief   hw usart api
 ******************************************************************************
 */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HW_USART_H__
#define __HW_USART_H__
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef Huart1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */
#define EOF              (-1)

/**
 * @brief read a char
 *
 * @return EOF is err, other is the char
 */
int32_t Porting_Uart_ReadChar(void);
/**
 * @brief  send a buf
 *
 * @param src   
 * @param len   len of src
 *
 * @return 0 is OK, -1 is err.
 */
int8_t Porting_Uart_Send(const char *src, uint8_t len);
/**
 * @brief  init uart
 *
 * @param BaudRate
 */
void Porting_Uart_Init(uint32_t BaudRate);
/**
 * @brief 
 */
void Porting_Uart_DeInit(void);

/**
 * @brief 
 */
void Porting_Error_Handler(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /* __HW_USART_H__ */
