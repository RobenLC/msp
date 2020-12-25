/**
  ******************************************************************************
  * @file    log.c
  * @author  MCD Application Team
  * @brief   Ressource table
  *
  *   This file provides services for logging
  *
  ******************************************************************************
  *
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the 
  * License. You may obtain a copy of the License at:
  *                       opensource.org/licenses/BSD-3-Clause
  *
  *
  ******************************************************************************
  */
/** @addtogroup LOG
  * @{
  */

/** @addtogroup STM32MP1xx_log
  * @{
  */

/** @addtogroup STM32MP1xx_Log_Private_Includes
  * @{
  */
#include "log.h"
#include "stm32mp1xx_hal_uart.h"
#include "stm32mp1xx_hal_uart_ex.h"

void LEDBlinking( void );
/**
  * @}
  */

/** @addtogroup STM32MP1xx_Log_Private_TypesDefinitions
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32MP1xx_Log_Private_Defines
  * @{
  */

/**
  * @}
  */

//*****************************************************************************
//
//*****************************************************************************
UART_HandleTypeDef huart7;

//*****************************************************************************
//  external function
//*****************************************************************************
extern void Error_Handler(void);

//*****************************************************************************
//
//*****************************************************************************


#if defined (__LOG_TRACE_IO_)
char system_log_buf[SYSTEM_TRACE_BUF_SZ];

__weak void log_buff(int ch)
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
 static int offset = 0;

	if (offset + 1 >= SYSTEM_TRACE_BUF_SZ)
		offset = 0;

	system_log_buf[offset] = ch;
	system_log_buf[offset++ + 1] = '\0';
}

#endif

#ifdef __GNUC__
/* With GCC/RAISONANCE, small log_info (option LD Linker->Libraries->Small log_info
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __attribute__(( weak )) __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int __attribute__(( weak )) fputc(int ch, FILE *f)
#endif /* __GNUC__ */

#if defined (__LOG_UART_IO_) || defined (__LOG_TRACE_IO_)
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
#if defined (__LOG_UART_IO_)
  HAL_UART_Transmit(&huart7, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
#endif
#if defined (__LOG_TRACE_IO_)
	log_buff(ch);
#endif
	return ch;
}
#else
/* No printf output */
#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/



/* UART7 init function */

void InitLogUART7(void)
{

  huart7.Instance = UART7;
  huart7.Init.BaudRate = 921600;
  huart7.Init.WordLength = UART_WORDLENGTH_8B;
  huart7.Init.StopBits = UART_STOPBITS_1;
  huart7.Init.Parity = UART_PARITY_NONE;
  huart7.Init.Mode = UART_MODE_TX_RX;
  huart7.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart7.Init.OverSampling = UART_OVERSAMPLING_16;
  huart7.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart7.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart7.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart7) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_UARTEx_SetTxFifoThreshold(&huart7, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_UARTEx_SetRxFifoThreshold(&huart7, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_UARTEx_DisableFifoMode(&huart7) != HAL_OK)
  {
    Error_Handler();
  }

}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  Error_Handler();
}



void logDump( void * buf, int size )
{
    unsigned char *ptr1 = buf;
    int s1 = size;

    for( int i1=0; i1< s1; i1+= 16)
    {
        log_info_nh("\t");
        for( int i2=0; (i2<16) && (size>0) ; i2++)
        {
            log_info_nh("%02X ", *ptr1++ );
            size--;
        }
        log_info_nh("\r\n");
    }
}



