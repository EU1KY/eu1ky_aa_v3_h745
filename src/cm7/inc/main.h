/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>

/* Includes ------------------------------------------------------------------*/
#include "debug.h"
#include "stm32h7xx_hal.h"
#include "stm32h745i_discovery.h"
#include "stm32h745i_discovery_lcd.h"
#include "stm32h745i_discovery_audio.h"
#include "ff_gen_drv.h"
#include "mmc_diskio.h"

/* ---------------------------------------------------------------------------*/
#define USARTx                           USART3
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART3_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

#define RCC_PERIPHCLK_USARTx             RCC_PERIPHCLK_USART3
#define RCC_USARTxCLKSOURCE_HSI          RCC_USART3CLKSOURCE_HSI

#define USARTx_FORCE_RESET()             __HAL_RCC_USART3_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART3_RELEASE_RESET()

#define USARTx_IRQn                      USART3_IRQn
#define USARTx_IRQHandler                USART3_IRQHandler

#define USARTx_TX_PIN                    GPIO_PIN_10
#define USARTx_TX_GPIO_PORT              GPIOB
#define USARTx_TX_AF                     GPIO_AF7_USART3
#define USARTx_RX_PIN                    GPIO_PIN_11
#define USARTx_RX_GPIO_PORT              GPIOB
#define USARTx_RX_AF                     GPIO_AF7_USART3

#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

#define LOCK_HSEM(__sem__)    do                                                \
                              {                                                 \
                                 while(HAL_HSEM_FastTake(__sem__) != HAL_OK) {} \
                              } while(0)

#define UNLOCK_HSEM(__sem__)  do                               \
                              {                                \
                                 HAL_HSEM_Release(__sem__, 0); \
                              } while(0)

#define HSEM_ID_0 (0U) // HW semaphore 0

/* Defines related to Clock configuration */
/* Uncomment to enable the adaquate Clock Source */
#define RTC_CLOCK_SOURCE_LSE

#ifdef RTC_CLOCK_SOURCE_LSI
#define RTC_ASYNCH_PREDIV    0x7F
#define RTC_SYNCH_PREDIV     0xF9
#endif

#ifdef RTC_CLOCK_SOURCE_LSE
#define RTC_ASYNCH_PREDIV  0x7F
#define RTC_SYNCH_PREDIV   0x00FF
#endif

extern RTC_HandleTypeDef RtcHandle;
extern volatile uint32_t ReloadFlag;

#endif /* __MAIN_H */

