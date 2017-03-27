/******************************************************************************
 * @file    hw_usart.c
 * @author  Joy
 * @version V1.0.0
 * @date    09-Nov-2016
 * @brief   inter usart implements
 ******************************************************************************
 */

#include "hw_usart.h"
#include "stm32l1xx_hal_def.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define BUFFSIZE                               2048

#define USARTx                                 USART1
#define USARTx_BAUDRATE                        115200
#define USART_RCC_CLK_ENABLE()                 __HAL_RCC_USART1_CLK_ENABLE()
#define USART_RCC_CLK_DISABLE()                __HAL_RCC_USART1_CLK_DISABLE()
#define USARTx_GPIO_ClK_ENABLE()               __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_Tx_GPIO_PIN                     GPIO_PIN_9
#define USARTx_Tx_GPIO                         GPIOA
#define USARTx_Rx_GPIO_PIN                     GPIO_PIN_10
#define USARTx_Rx_GPIO                         GPIOA

#define USARTx_IRQHANDLER                      USART1_IRQHandler
#define USARTx_IRQn                            USART1_IRQn

#define READ_TIME_OUT                          1

typedef struct Uart_s
{
    uint8_t *pRear;            
    uint8_t *pFront;
}Uart_t;

UART_HandleTypeDef Huart1;

static uint8_t RxBuffer[BUFFSIZE];
static uint8_t TxBuffer[BUFFSIZE];
static Uart_t UartRecver;
static Uart_t UartSender;

unsigned int txBuffer()
{
    return (unsigned int)&TxBuffer[0];
}
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{  
    GPIO_InitTypeDef  GPIO_InitStruct;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO TX/RX clock */
    __GPIOA_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();
    __USART1_CLK_ENABLE();


    /*##-2- Configure peripheral GPIO ##########################################*/  
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* UART RX GPIO pin configuration  */
    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Pull      = GPIO_PULLDOWN;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*##-3- Configure the NVIC for UART ########################################*/
    /* NVIC for USART1 */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 1);//HAL_NVIC_SetPriority(USART1_IRQn, 1, 0); ???
    HAL_NVIC_EnableIRQ(USARTx_IRQn);
}

/**
 * @brief UART MSP De-Initialization 
 *        This function frees the hardware resources used in this example:
 *          - Disable the Peripheral's clock
 *          - Revert GPIO and NVIC configuration to their default state
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    /*##-1- Reset peripherals ##################################################*/
    USART_RCC_CLK_DISABLE();

    /*##-2- Disable peripherals and GPIO Clocks #################################*/
    /* Configure UART Tx as alternate function  */
    HAL_GPIO_DeInit(USARTx_Tx_GPIO, USARTx_Tx_GPIO_PIN);
      HAL_GPIO_DeInit(USARTx_Rx_GPIO, USARTx_Rx_GPIO_PIN);
    /* Configure UART Rx as alternate function  */

    /*##-3- Disable the NVIC for UART ##########################################*/
    HAL_NVIC_DisableIRQ(USARTx_IRQn);
}

void Porting_Error_Handler(void)
{
    while(1)
    {

    }
}
void Porting_Uart_DeInit()
{
    
}

void Porting_Uart_Init(uint32_t BaudRate)
{
    Huart1.Instance        = USART1;
    Huart1.Init.BaudRate   = BaudRate;
    Huart1.Init.WordLength = UART_WORDLENGTH_8B;
    Huart1.Init.StopBits   = UART_STOPBITS_1;
    Huart1.Init.Parity     = UART_PARITY_NONE;
    Huart1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    Huart1.Init.Mode       = UART_MODE_TX_RX;

    HAL_UART_Init(&Huart1);
    __HAL_UART_ENABLE(&Huart1);

    NVIC_SetPriority(USART1_IRQn,0);
    NVIC_EnableIRQ(USART1_IRQn);
    UartRecver.pFront = RxBuffer;
    UartRecver.pRear = RxBuffer;
    UartSender.pFront = TxBuffer;
    UartSender.pRear = TxBuffer;
    HAL_UART_Receive_IT(&Huart1,(uint8_t*)RxBuffer,1);
}

int32_t Porting_Uart_ReadChar(void)
{
    int32_t ch = -1;
    uint16_t timeOut = READ_TIME_OUT;

    while (timeOut--)
    {
        if(UartRecver.pFront != UartRecver.pRear)
        {
            ch = *UartRecver.pFront;
            UartRecver.pFront++;
            if (UartRecver.pFront >= (RxBuffer+BUFFSIZE))
            {
                UartRecver.pFront = RxBuffer;
            }
            return ch;
        }
    }
    return (EOF);
}
#ifdef DEBUG
int8_t Porting_Uart_Read(uint8_t *fmt, uint16_t time_out)
{
    if (!fmt)
    {
        return (int8_t)-1;
    }
    while(time_out)
    {
        if(UartRecver.pFront != UartRecver.pRear)
        {
            *fmt=*UartRecver.pFront;
            UartRecver.pFront++;
            if (UartRecver.pFront >= (RxBuffer+BUFFSIZE))
            {
                UartRecver.pFront = RxBuffer;
            }
            return 0;
        }
        time_out--;
    }
    return (int8_t)-1;
}
#endif
static void Porting_Uart_SendChar(uint8_t ch)
{
    uint8_t ret = HAL_OK;
    
    NVIC_DisableIRQ(USART1_IRQn);//__disable_irq();
    if (UartSender.pRear == UartSender.pFront)
    {
        *UartSender.pRear = ch;
        UartSender.pRear++;
        if (UartSender.pRear >= TxBuffer + BUFFSIZE)
        {
            UartSender.pRear =  TxBuffer;          
        }
        do{
            ret = HAL_UART_Transmit_IT(&Huart1, UartSender.pFront, 1);
            NVIC_EnableIRQ(USART1_IRQn);//__enable_irq();
        }while(ret != HAL_OK);             
    }
    else
    {
        *UartSender.pRear = ch;
        UartSender.pRear++;
        if (UartSender.pRear >= TxBuffer + BUFFSIZE)
        {
            UartSender.pRear = TxBuffer;
        }
        if (UartSender.pRear > UartSender.pFront)
        {
            if ((UartSender.pRear - UartSender.pFront) >= (BUFFSIZE-50))
            {
                NVIC_EnableIRQ(USART1_IRQn);//__enable_irq();
                HAL_Delay(10);
                //do{
                //    ret = HAL_UART_Transmit_IT(&Huart1, UartSender.pFront, (BUFFSIZE-50));
                //    __enable_irq();
                //}while(ret != HAL_OK);    
            }
        }
        else if (UartSender.pRear < UartSender.pFront)
        {
            if ((UartSender.pFront - UartSender.pRear) >= (BUFFSIZE-50))
            {
                NVIC_EnableIRQ(USART1_IRQn);//__enable_irq();
                HAL_Delay(10);
                //do{
                //    ret = HAL_UART_Transmit_IT(&Huart1, UartSender.pFront, (BUFFSIZE-50));
                //    __enable_irq();
                //}while(ret != HAL_OK);    
            }
        }
    }
    NVIC_EnableIRQ(USART1_IRQn);//__enable_irq();
}
int8_t Porting_Uart_Send(const char *src, uint8_t len)
{
    if (!src || (len == 0))
    {
        return (-1);
    }
    while (len)
    {
        Porting_Uart_SendChar(*src);
        src++;
        len--;
    }

    return 0;
}

/*-----------------------------------------------------------------------------
 *  weak api (override sys api)
 *-----------------------------------------------------------------------------*/
int puts(const char *s)
{
    while (*s)
    {
        if ('\n' == *s)
        {
            Porting_Uart_SendChar('\r');
        }
        Porting_Uart_SendChar(*s);
        s++;
    }
    return 0;
}
int fputc(int ch, FILE *f)
{
    if ('\n' == ch&0xff)
    {
        Porting_Uart_SendChar('\r');
    }
    Porting_Uart_SendChar(ch & 0xff);
    return(ch);
}
int putc(int ch, FILE *f)
{
    Porting_Uart_SendChar(ch & 0xff);
    return(ch);
}
int getc(FILE *stream)
{
    int ch = EOF;
    ch = Porting_Uart_ReadChar();
    return ch;
}


#ifdef MY_PRINT
int printf(const char *fmt, ...)
{
    va_list args;
    uint32_t i;
    char printbuffer[256];

    va_start(args, fmt);

    /* For this to work, printbuffer must be larger than
     * anything we ever want to print.
     */
    i = vscnprintf(printbuffer, sizeof(printbuffer), fmt, args);
    va_end(args);

    /* Print the string */
    puts(printbuffer);
    return i;
}
#endif
#ifdef UART_DEBUG
int fputc(int ch, FILE *f)
{
    USART1->TDR = ch;
    while(!(USART1->ISR & USART_ISR_TXE));
    return(ch);
}
#endif
/*   weak api (override sys api) end */

/**
 * @brief  Tx Transfer completed callback
 * @param  huart: UART handle. 
 * @note   This example shows a simple way to report end of IT Tx transfer, and 
 *         you can add your own implementation. 
 * @retval None
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef * Huart)
{
    uint8_t ret = HAL_OK;
    UartSender.pFront ++;//= Huart->TxXferCount;    
    if(UartSender.pFront >= (TxBuffer + BUFFSIZE))
    {
        UartSender.pFront = TxBuffer;
    }
    if(UartSender.pFront != UartSender.pRear)
    {
        do
        {
            ret = HAL_UART_Transmit_IT(Huart, UartSender.pFront, 1);
        }while(ret != HAL_OK);
    }
}
/**
 * @brief  Rx Transfer completed callback
 * @param  huart: UART handle
 * @note   This example shows a simple way to report end of IT Rx transfer, and 
 *         you can add your own implementation.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *Huart)
{
    uint8_t ret = HAL_OK;
    /* Set transmission flag: trasfer complete*/
    UartRecver.pRear++;    //rear
    if(UartRecver.pRear >= (RxBuffer + BUFFSIZE))
    {
        UartRecver.pRear = RxBuffer;
    }
    do
    {
        ret = HAL_UART_Receive_IT(Huart, UartRecver.pRear, 1);
    }while(ret != HAL_OK);
}
/******************************************************************************/
/*                 STM32L0xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32l0xx.s).                                               */
/******************************************************************************/
/**
 * @brief  This function handles UART interrupt request.  
 * @param  None
 * @retval None
 * @Note   This function is redefined in "main.h" and related to DMA stream 
 *         used for USART data transmission     
 */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&Huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_Delay(5000);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_Delay(5000);
    Porting_Uart_Init(115200);	
}
