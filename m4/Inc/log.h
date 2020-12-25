/**
  ******************************************************************************
  * @file    log.h
  * @author  MCD Application Team
  * @brief   logging services
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

/** @addtogroup stm32mp1xx_Log
  * @{
  */

/**
  * @brief Define to prevent recursive inclusion
  */
#ifndef __LOG_STM32MP1XX_H
#define __LOG_STM32MP1XX_H

#ifdef __cplusplus
 extern "C" {
#endif

/** @addtogroup STM32MP1xx_Log_Includes
  * @{
  */
#include "stm32mp1xx_hal.h"
/**
  * @}
  */


#define __LOG_UART_IO_
extern UART_HandleTypeDef huart7;

/** @addtogroup STM32MP1xx_Log_Exported_Constants
  * @{
  */
#if defined (__LOG_TRACE_IO_)
#define SYSTEM_TRACE_BUF_SZ 2048
#endif

#define LOGQUIET 0
#define   LOGSYS   1
#define	LOGERR	  2
#define	LOGWARN  3
#define	LOGINFO  4
#define	LOGDBG   5

#ifndef LOGLEVEL
#define LOGLEVEL LOGSYS
#endif


/**
  * @}
  */

/** @addtogroup STM32MP1xx_Log_Exported_types
  * @{
  */
#if defined (__LOG_TRACE_IO_)
extern char system_log_buf[SYSTEM_TRACE_BUF_SZ]; /*!< buffer for debug traces */
#endif /* __LOG_TRACE_IO_ */
/**
  * @}
  */

/** @addtogroup STM32MP1xx_Log_Exported_Macros
  * @{
  */
#if defined (__LOG_TRACE_IO_) || defined(__LOG_UART_IO_)
#if LOGLEVEL >= LOGDBG
#define log_dbg(fmt, ...)  printf("[%05ld.%03ld][DBG  ]" fmt, HAL_GetTick()/1000, HAL_GetTick() % 1000, ##__VA_ARGS__)
#define log_dbg_nh(fmt, ...)   printf( fmt, ##__VA_ARGS__)
#else
#define log_dbg(fmt, ...)
#define log_dbg_nh(fmt, ...)
#endif
#if LOGLEVEL >= LOGINFO
#define log_info(fmt, ...) printf("[%05ld.%03ld][INFO ]" fmt, HAL_GetTick()/1000, HAL_GetTick() % 1000, ##__VA_ARGS__)
#define log_info_nh(fmt, ...)  printf( fmt, ##__VA_ARGS__)
#else
#define log_info(fmt, ...)
#define log_info_nh(fmt, ...)
#endif
#if LOGLEVEL >= LOGWARN
#define log_warn(fmt, ...) printf("[%05ld.%03ld][WARN ]" fmt, HAL_GetTick()/1000, HAL_GetTick() % 1000, ##__VA_ARGS__)
#else
#define log_warn(fmt, ...)
#endif
#if LOGLEVEL >= LOGERR
#define log_err(fmt, ...)  printf("[%05ld.%03ld][ERR  ]" fmt, HAL_GetTick()/1000, HAL_GetTick() % 1000, ##__VA_ARGS__)
#else
#define log_err(fmt, ...)
#endif
#if LOGLEVEL >= LOGSYS
#define log_sys(fmt, ...)  printf("[%05ld.%03ld][SYS  ]" fmt, HAL_GetTick()/1000, HAL_GetTick() % 1000, ##__VA_ARGS__)
#else
#define log_sys(fmt, ...)
#endif
#else
#define log_dbg(fmt, ...)
#define log_info(fmt, ...)
#define log_warn(fmt, ...)
#define log_err(fmt, ...)
#define log_sys(fmt, ...)

#define log_info_nh(fmt, ...)
#define log_dbg_nh(fmt, ...)

#endif /* __LOG_TRACE_IO_ */
/**
  * @}
  */

/** @addtogroup STM32MP1xx_Log_Exported_Functions
  * @{
  */

void InitLogUART7(void);
void logDump( void * buf, int size );

/**
  * @}
  */

extern UART_HandleTypeDef huart7;

#ifdef __cplusplus
}
#endif

#endif /*__LOG_STM32MP1XX_H */

/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
