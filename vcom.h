/******************************************************************************
  * @file    vcom.h
  * @author  Zhangsf
  * @version V1.0.0
  * @date    01-October-2016
  * @brief   Header for vcom.c module
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __VCOM_H__
#define __VCOM_H__

#ifdef __cplusplus
 extern "C" {
#endif
   
/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */ 

/** 
* @brief  Init the VCOM.
* @param  None
* @return None
*/
void vcom_Init(void);

   /** 
* @brief  DeInit the VCOM.
* @param  None
* @return None
*/
void vcom_DeInit(void);

   /** 
* @brief  sends string on com port
* @param  string
* @return None
*/
void vcom_Send( const char *format, ... );

/* Exported macros -----------------------------------------------------------*/
#if 0
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


#ifdef __cplusplus
}
#endif

#endif /* __VCOM_H__*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
