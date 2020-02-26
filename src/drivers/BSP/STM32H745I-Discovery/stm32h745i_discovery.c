/**
  ******************************************************************************
  * @file    stm32h745i_discovery.c
  * @author  MCD Application Team
  * @brief   This file provides a set of firmware functions to manage LEDs,
  *          push-buttons, external SDRAM, external QSPI Flash,
  *          available on STM32H745I-Discovery board (MB1381) from
  *          STMicroelectronics.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32h745i_discovery.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32H745I_DISCOVERY
  * @{
  */

/** @defgroup STM32H745I_DISCOVERY_LOW_LEVEL STM32H745I_DISCOVERY_LOW_LEVEL
  * @{
  */

/** @defgroup STM32H745I_DISCOVERY_LOW_LEVEL_Private_Defines Private Defines
  * @{
  */
/**
 * @brief STM32H745I Discovery BSP Driver version number V1.0.0
   */
#define __STM32H745I_DISCOVERY_BSP_VERSION_MAIN   (0x01) /*!< [31:24] main version */
#define __STM32H745I_DISCOVERY_BSP_VERSION_SUB1   (0x00) /*!< [23:16] sub1 version */
#define __STM32H745I_DISCOVERY_BSP_VERSION_SUB2   (0x00) /*!< [15:8]  sub2 version */
#define __STM32H745I_DISCOVERY_BSP_VERSION_RC     (0x00) /*!< [7:0]  release candidate */
#define __STM32H745I_DISCOVERY_BSP_VERSION        ((__STM32H745I_DISCOVERY_BSP_VERSION_MAIN << 24)\
                                                  |(__STM32H745I_DISCOVERY_BSP_VERSION_SUB1 << 16)\
                                                  |(__STM32H745I_DISCOVERY_BSP_VERSION_SUB2 << 8 )\
                                                  |(__STM32H745I_DISCOVERY_BSP_VERSION_RC))
/**
  * @}
  */

/** @defgroup STM32H745I_DISCOVERY_LOW_LEVEL_Private_Variables Private Variables
  * @{
  */

GPIO_TypeDef* GPIO_PORT[LEDn] = {LED1_GPIO_PORT,
                                 LED2_GPIO_PORT};

const uint32_t GPIO_PIN[LEDn] = {LED1_PIN,
                                 LED2_PIN};

GPIO_TypeDef* BUTTON_PORT[BUTTONn] = {WAKEUP_BUTTON_GPIO_PORT };

const uint16_t BUTTON_PIN[BUTTONn] = {WAKEUP_BUTTON_PIN };

const uint16_t BUTTON_IRQn[BUTTONn] = {WAKEUP_BUTTON_EXTI_IRQn };

//NB: COM2 was not implemented in Cube. Added by EU1KY
USART_TypeDef* COM_USART[COMn] = {DISCOVERY_COM1, DISCOVERY_COM2};

GPIO_TypeDef* COM_TX_PORT[COMn] = {DISCOVERY_COM1_TX_GPIO_PORT, DISCOVERY_COM2_TX_GPIO_PORT};

GPIO_TypeDef* COM_RX_PORT[COMn] = {DISCOVERY_COM1_RX_GPIO_PORT, DISCOVERY_COM2_RX_GPIO_PORT};

const uint16_t COM_TX_PIN[COMn] = {DISCOVERY_COM1_TX_PIN, DISCOVERY_COM2_TX_PIN};

const uint16_t COM_RX_PIN[COMn] = {DISCOVERY_COM1_RX_PIN, DISCOVERY_COM2_RX_PIN};

const uint16_t COM_TX_AF[COMn] = {DISCOVERY_COM1_TX_AF, DISCOVERY_COM2_TX_AF};

const uint16_t COM_RX_AF[COMn] = {DISCOVERY_COM1_RX_AF, DISCOVERY_COM2_RX_AF};

static I2C_HandleTypeDef hdiscovery_I2c = {0};

/**
  * @}
  */

/** @defgroup STM32H745I_DISCOVERY_LOW_LEVEL_Private_FunctionPrototypes Private FunctionPrototypes
  * @{
  */
static void     I2Cx_MspInit(void);
static void     I2Cx_Init(void);
static void     I2Cx_Write(uint8_t Addr, uint8_t Reg, uint8_t Value);
static uint8_t  I2Cx_Read(uint8_t Addr, uint8_t Reg);
static HAL_StatusTypeDef I2Cx_ReadMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static HAL_StatusTypeDef I2Cx_WriteMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static void     I2Cx_Error(uint8_t Addr);

/* AUDIO IO functions */
void            AUDIO_IO_Init(void);
void            AUDIO_IO_DeInit(void);
void            AUDIO_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value);
uint16_t        AUDIO_IO_Read(uint8_t Addr, uint16_t Reg);
void            AUDIO_IO_Delay(uint32_t Delay);

/* TouchScreen (TS) IO functions */
void     TS_IO_Init(void);
void     TS_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value);
uint8_t  TS_IO_Read(uint8_t Addr, uint8_t Reg);
uint16_t TS_IO_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length);
void     TS_IO_WriteMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length);
void     TS_IO_Delay(uint32_t Delay);

/* LCD Display IO functions */
void OTM8009A_IO_Delay(uint32_t Delay);
/**
  * @}
  */

/** @defgroup STM32H745I_DISCOVERY_BSP_Exported_Functions Exported Functions
  * @{
  */

  /**
  * @brief  This method returns the STM32H745I Discovery BSP Driver revision
  * @retval version: 0xXYZR (8bits for each decimal, R for RC)
  */
uint32_t BSP_GetVersion(void)
{
  return __STM32H745I_DISCOVERY_BSP_VERSION;
}

/**
  * @brief  Configures LED GPIO.
  * @param  Led: LED to be configured.
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  * @retval None
  */
void BSP_LED_Init(Led_TypeDef Led)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  /* Enable the GPIO_LED clock */
  LEDx_GPIO_CLK_ENABLE();

  /* Configure the GPIO_LED pin */
  GPIO_InitStruct.Pin = GPIO_PIN[Led];
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  HAL_GPIO_Init(GPIO_PORT[Led], &GPIO_InitStruct);

  /* By default, turn off LED */
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
}


/**
  * @brief  DeInit LEDs.
  * @param  Led: LED to be configured.
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  * @note Led DeInit does not disable the GPIO clock
  * @retval None
  */
void BSP_LED_DeInit(Led_TypeDef Led)
{
    /* Turn off LED */
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
    /* Configure the GPIO_LED pin */
    HAL_GPIO_DeInit(GPIO_PORT[Led], GPIO_PIN[Led]);
}

/**
  * @brief  Turns selected LED On.
  * @param  Led: LED to be set on
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  * @retval None
  */
void BSP_LED_On(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: LED to be set off
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  * @retval None
  */
void BSP_LED_Off(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: LED to be toggled
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  * @retval None
  */
void BSP_LED_Toggle(Led_TypeDef Led)
{
  HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
}

/**
  * @brief  Configures button GPIO and EXTI Line.
  * @param  Button: Button to be configured
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  *            @arg  BUTTON_USER: User Push Button
  * @param  Button_Mode: Button mode
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_MODE_GPIO: Button will be used as simple IO
  *            @arg  BUTTON_MODE_EXTI: Button will be connected to EXTI line
  *                                    with interrupt generation capability
  * @retval None
  */
void BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Enable the BUTTON clock */
  BUTTON_GPIO_CLK_ENABLE();

  if(Button_Mode == BUTTON_MODE_GPIO)
  {
    /* Configure Button pin as input */
    GPIO_InitStruct.Pin = BUTTON_PIN[Button];
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BUTTON_PORT[Button], &GPIO_InitStruct);
  }

  if(Button_Mode == BUTTON_MODE_EXTI)
  {
    /* Configure Button pin as input with External interrupt */
    GPIO_InitStruct.Pin = BUTTON_PIN[Button];
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;

    HAL_GPIO_Init(BUTTON_PORT[Button], &GPIO_InitStruct);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority((IRQn_Type)(BUTTON_IRQn[Button]), 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
  }
}

/**
  * @brief  Push Button DeInit.
  * @param  Button: Button to be configured
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  *            @arg  BUTTON_USER: User Push Button
  * @note PB DeInit does not disable the GPIO clock
  * @retval None
  */
void BSP_PB_DeInit(Button_TypeDef Button)
{
    GPIO_InitTypeDef gpio_init_structure;

    gpio_init_structure.Pin = BUTTON_PIN[Button];
    HAL_NVIC_DisableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
    HAL_GPIO_DeInit(BUTTON_PORT[Button], gpio_init_structure.Pin);
}

/**
  * @brief  Returns the selected button state.
  * @param  Button: Button to be checked
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  *            @arg  BUTTON_USER: User Push Button
  * @retval The Button GPIO pin value
  */
uint32_t BSP_PB_GetState(Button_TypeDef Button)
{
  return HAL_GPIO_ReadPin(BUTTON_PORT[Button], BUTTON_PIN[Button]);
}
/**
  * @brief  Configures COM port.
  * @param  COM: COM port to be configured.
  *          This parameter can be one of the following values:
  *            @arg  COM1
  *            @arg  COM2
  * @param  huart: Pointer to a UART_HandleTypeDef structure that contains the
  *                configuration information for the specified USART peripheral.
  * @retval None
  */
void BSP_COM_Init(COM_TypeDef COM, UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef gpio_init_structure;

  /* Enable GPIO clock */
  DISCOVERY_COMx_TX_GPIO_CLK_ENABLE(COM);
  DISCOVERY_COMx_RX_GPIO_CLK_ENABLE(COM);

  /* Enable USART clock */
  DISCOVERY_COMx_CLK_ENABLE(COM);

  /* Configure USART Tx as alternate function */
  gpio_init_structure.Pin = COM_TX_PIN[COM];
  gpio_init_structure.Mode = GPIO_MODE_AF_PP;
  gpio_init_structure.Speed = GPIO_SPEED_FAST;
  gpio_init_structure.Pull = GPIO_PULLUP;
  gpio_init_structure.Alternate = COM_TX_AF[COM];
  HAL_GPIO_Init(COM_TX_PORT[COM], &gpio_init_structure);

  /* Configure USART Rx as alternate function */
  gpio_init_structure.Pin = COM_RX_PIN[COM];
  gpio_init_structure.Mode = GPIO_MODE_AF_PP;
  gpio_init_structure.Alternate = COM_RX_AF[COM];
  HAL_GPIO_Init(COM_RX_PORT[COM], &gpio_init_structure);

  /* USART configuration */
  huart->Instance = COM_USART[COM];
  HAL_UART_Init(huart);
}

/**
  * @brief  DeInit COM port.
  * @param  COM: COM port to be configured.
  *          This parameter can be one of the following values:
  *            @arg  COM1
  *            @arg  COM2
  * @param  huart: Pointer to a UART_HandleTypeDef structure that contains the
  *                configuration information for the specified USART peripheral.
  * @retval None
  */
void BSP_COM_DeInit(COM_TypeDef COM, UART_HandleTypeDef *huart)
{
  /* USART configuration */
  huart->Instance = COM_USART[COM];
  HAL_UART_DeInit(huart);

  /* Enable USART clock */
  DISCOVERY_COMx_CLK_DISABLE(COM);

  /* DeInit GPIO pins can be done in the application
     (by surcharging this __weak function) */

  /* GPIO pins clock, DMA clock can be shut down in the application
     by surcharging this __weak function */
}


/**
  * @}
  */

/** @defgroup STM32H745I_DISCOVERY_LOW_LEVEL_Private_Functions Private Functions
  * @{
  */
/*******************************************************************************
                            BUS OPERATIONS
*******************************************************************************/

/******************************* I2C Routines *********************************/
/**
  * @brief  Initializes I2C MSP.
  * @retval None
  */
static void I2Cx_MspInit(void)
{
  GPIO_InitTypeDef  gpio_init_structure;
  RCC_PeriphCLKInitTypeDef  RCC_PeriphClkInit;

  /* Configure the I2C clock source */
  RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C123;
  RCC_PeriphClkInit.I2c123ClockSelection = RCC_I2C123CLKSOURCE_D2PCLK1;
  HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

  /* set STOPWUCK in RCC_CFGR */
  __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_HSI);

  /*** Configure the GPIOs ***/
  /* Enable GPIO clock */
  DISCOVERY_I2Cx_SCL_SDA_GPIO_CLK_ENABLE();

  /* Configure I2C Tx as alternate function */
  gpio_init_structure.Pin = DISCOVERY_I2Cx_SCL_PIN;
  gpio_init_structure.Mode = GPIO_MODE_AF_OD;
  gpio_init_structure.Pull = GPIO_NOPULL;
  gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init_structure.Alternate = DISCOVERY_I2Cx_SCL_SDA_AF;
  HAL_GPIO_Init(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, &gpio_init_structure);

  /* Configure I2C Rx as alternate function */
  gpio_init_structure.Pin = DISCOVERY_I2Cx_SDA_PIN;
  HAL_GPIO_Init(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, &gpio_init_structure);

  /*** Configure the I2C peripheral ***/
  /* Enable I2C clock */
  DISCOVERY_I2Cx_CLK_ENABLE();

  /* Force the I2C peripheral clock reset */
  DISCOVERY_I2Cx_FORCE_RESET();

  /* Release the I2C peripheral clock reset */
  DISCOVERY_I2Cx_RELEASE_RESET();

  /* Enable and set I2Cx Interrupt to a lower priority */
  HAL_NVIC_SetPriority(DISCOVERY_I2Cx_EV_IRQn, 0x0F, 0);
  HAL_NVIC_EnableIRQ(DISCOVERY_I2Cx_EV_IRQn);

  /* Enable and set I2Cx Interrupt to a lower priority */
  HAL_NVIC_SetPriority(DISCOVERY_I2Cx_ER_IRQn, 0x0F, 0);
  HAL_NVIC_EnableIRQ(DISCOVERY_I2Cx_ER_IRQn);
}

/**
  * @brief  Initializes I2C HAL.
  * @retval None
  */
static void I2Cx_Init(void)
{
  if(HAL_I2C_GetState(&hdiscovery_I2c) == HAL_I2C_STATE_RESET)
  {
    hdiscovery_I2c.Instance              = DISCOVERY_I2Cx;
    hdiscovery_I2c.Init.Timing           = DISCOVERY_I2Cx_TIMING;
    hdiscovery_I2c.Init.OwnAddress1      = 0x72;
    hdiscovery_I2c.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hdiscovery_I2c.Init.DualAddressMode  = I2C_DUALADDRESS_ENABLE;
    hdiscovery_I2c.Init.OwnAddress2      = 0;
    hdiscovery_I2c.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hdiscovery_I2c.Init.GeneralCallMode  = I2C_GENERALCALL_ENABLE;
    hdiscovery_I2c.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    /* Init the I2C */
    I2Cx_MspInit();
    HAL_I2C_Init(&hdiscovery_I2c);
  }
}

/**
  * @brief  Writes a single data.
  * @param  Addr: I2C address
  * @param  Reg: Register address
  * @param  Value: Data to be written
  * @retval None
  */
static void I2Cx_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  HAL_StatusTypeDef status = HAL_OK;
RETRY:
  status = HAL_I2C_Mem_Write(&hdiscovery_I2c, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, &Value, 1, 100);

  // Check the communication status. Do not call error handler if NACK is detected
  if((status != HAL_OK) && (0 == (HAL_I2C_ERROR_NO_ERR_HANDLING & hdiscovery_I2c.ErrorCode)))
  {
    uint32_t err = hdiscovery_I2c.ErrorCode;
    // Re-Initiaize the I2C Bus
    I2Cx_Error(Addr);
    if (HAL_I2C_ERROR_TIMEOUT & err)
    {
      goto RETRY;
    }
  }
}

/**
  * @brief  Reads a single data.
  * @param  Addr: I2C address
  * @param  Reg: Register address
  * @retval Read data
  */
static uint8_t I2Cx_Read(uint8_t Addr, uint8_t Reg)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t Value = 0;

RETRY:
  status = HAL_I2C_Mem_Read(&hdiscovery_I2c, Addr, Reg, I2C_MEMADD_SIZE_8BIT, &Value, 1, 1000);

  // Check the communication status. Do not call error handler if NACK is detected
  if((status != HAL_OK) && (0 == (HAL_I2C_ERROR_NO_ERR_HANDLING & hdiscovery_I2c.ErrorCode)))
  {
    uint32_t err = hdiscovery_I2c.ErrorCode;
    // Re-Initiaize the I2C Bus
    I2Cx_Error(Addr);
    if (HAL_I2C_ERROR_TIMEOUT & err)
    {
      goto RETRY;
    }
  }
  return Value;
}

/**
  * @brief  Reads multiple data.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @param  MemAddress: memory address
  * @param  Buffer: Pointer to data buffer
  * @param  Length: Length of the data
  * @retval HAL status
  */
static HAL_StatusTypeDef I2Cx_ReadMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddress, uint8_t *Buffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

RETRY:
  status = HAL_I2C_Mem_Read(&hdiscovery_I2c, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

  // Check the communication status. Do not call error handler if NACK is detected
  if((status != HAL_OK) && (0 == (HAL_I2C_ERROR_NO_ERR_HANDLING & hdiscovery_I2c.ErrorCode)))
  {
    uint32_t err = hdiscovery_I2c.ErrorCode;
    // Re-Initiaize the I2C Bus
    I2Cx_Error(Addr);
    if (HAL_I2C_ERROR_TIMEOUT & err)
    {
      goto RETRY;
    }
  }
  return status;
}

/**
  * @brief  Writes a value in a register of the device through BUS in using DMA mode.
  * @param  Addr: Device address on BUS Bus.
  * @param  Reg: The target register address to write
  * @param  MemAddress: memory address
  * @param  Buffer: The target register value to be written
  * @param  Length: buffer size to be written
  * @retval HAL status
  */
static HAL_StatusTypeDef I2Cx_WriteMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddress, uint8_t *Buffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

RETRY:
  status = HAL_I2C_Mem_Write(&hdiscovery_I2c, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 100);

  // Check the communication status. Do not call error handler if NACK is detected
  if((status != HAL_OK) && (0 == (HAL_I2C_ERROR_NO_ERR_HANDLING & hdiscovery_I2c.ErrorCode)))
  {
    uint32_t err = hdiscovery_I2c.ErrorCode;
    // Re-Initiaize the I2C Bus
    I2Cx_Error(Addr);
    if (HAL_I2C_ERROR_TIMEOUT & err)
    {
      goto RETRY;
    }
  }
  return status;
}

static uint8_t wait_for_gpio_state_timeout(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout)
 {
    uint32_t Tickstart = HAL_GetTick();
    uint8_t ret = 1;
    // Wait until flag is set
    for( ; (state != HAL_GPIO_ReadPin(port, pin)) && (1 == ret); )
    {
        /* Check for the timeout */
        if (timeout != HAL_MAX_DELAY)
        {
            if ((timeout == 0U) || ((HAL_GetTick() - Tickstart) > timeout))
            {
                ret = 0;
            }
        }
        asm volatile ("nop");
    }
    return ret;
}

/**
  * @brief  Manages error callback by re-initializing I2C.
  * 
  * Recovery is based on a procedure from https://electronics.stackexchange.com/questions/267972/i2c-busy-flag-strange-behaviour
  * 
  * @param  Addr: I2C Address
  * @retval None
  */
static void I2Cx_Error(uint8_t Addr)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // 1. Clear PE bit.
  CLEAR_BIT(hdiscovery_I2c.Instance->CR1, I2C_CR1_PE);

  // 2. Configure the SCL and SDA I/Os as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
  HAL_I2C_DeInit(&hdiscovery_I2c);

  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = DISCOVERY_I2Cx_SCL_PIN;
  HAL_GPIO_Init(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.Pin = DISCOVERY_I2Cx_SDA_PIN;
  HAL_GPIO_Init(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, &GPIO_InitStructure);

  // 3. Check SCL and SDA High level in GPIOx_IDR.
  HAL_GPIO_WritePin(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SDA_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SCL_PIN, GPIO_PIN_SET);

  wait_for_gpio_state_timeout(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SCL_PIN, GPIO_PIN_SET, 10);
  wait_for_gpio_state_timeout(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SDA_PIN, GPIO_PIN_SET, 10);

  // 4. Configure the SDA I/O as General Purpose Output Open-Drain, Low level
  HAL_GPIO_WritePin(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SDA_PIN, GPIO_PIN_RESET);

  // 5. Check SDA Low level in GPIOx_IDR.
  wait_for_gpio_state_timeout(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SDA_PIN, GPIO_PIN_RESET, 10);

  // 6. Configure the SCL I/O as General Purpose Output Open-Drain, Low level
  HAL_GPIO_WritePin(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SCL_PIN, GPIO_PIN_RESET);

  // 7. Check SCL Low level in GPIOx_IDR.
  wait_for_gpio_state_timeout(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SCL_PIN, GPIO_PIN_RESET, 10);

  // 8. Configure the SCL I/O as General Purpose Output Open-Drain, High level
  HAL_GPIO_WritePin(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SCL_PIN, GPIO_PIN_SET);

  // 9. Check SCL High level in GPIOx_IDR.
  wait_for_gpio_state_timeout(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SCL_PIN, GPIO_PIN_SET, 10);

  // 10. Configure the SDA I/O as General Purpose Output Open-Drain , High level (Write 1 to GPIOx_ODR).
  HAL_GPIO_WritePin(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SDA_PIN, GPIO_PIN_SET);

  // 11. Check SDA High level in GPIOx_IDR.
  wait_for_gpio_state_timeout(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, DISCOVERY_I2Cx_SDA_PIN, GPIO_PIN_SET, 10);

  // 12. Configure the SCL and SDA I/Os as Alternate function Open-Drain.
  GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStructure.Alternate = DISCOVERY_I2Cx_SCL_SDA_AF;
  GPIO_InitStructure.Pin = DISCOVERY_I2Cx_SCL_PIN;
  HAL_GPIO_Init(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.Pin = DISCOVERY_I2Cx_SDA_PIN;
  HAL_GPIO_Init(DISCOVERY_I2Cx_SCL_SDA_GPIO_PORT, &GPIO_InitStructure);

  // 13. I2C reset on
  DISCOVERY_I2Cx_FORCE_RESET();
  for (uint32_t i = 0; i < 40; i++)
  {
    asm volatile ("nop");
  }

  // 14. I2C reset off
  DISCOVERY_I2Cx_RELEASE_RESET();

  // 15. Set PE bit.
  SET_BIT(hdiscovery_I2c.Instance->CR1, I2C_CR1_PE);

  // Re-Initialize the I2C communication bus
  I2Cx_Init();
}

/*******************************************************************************
                            LINK OPERATIONS
*******************************************************************************/

/********************************* LINK AUDIO *********************************/

/**
  * @brief  Initializes Audio low level.
  * @retval None
  */
void AUDIO_IO_Init(void)
{
  I2Cx_Init();
}

/**
  * @brief  Deinitializes Audio low level.
  * @retval None
  */
void AUDIO_IO_DeInit(void)
{
}

/**
  * @brief  Writes a single data.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @param  Value: Data to be written
  * @retval None
  */
void AUDIO_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value)
{
  uint16_t tmp = Value;

  Value = ((uint16_t)(tmp >> 8) & 0x00FF);

  Value |= ((uint16_t)(tmp << 8)& 0xFF00);

  I2Cx_WriteMultiple(Addr, Reg, I2C_MEMADD_SIZE_16BIT,(uint8_t*)&Value, 2);
}

/**
  * @brief  Reads a single data.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @retval Data to be read
  */
uint16_t AUDIO_IO_Read(uint8_t Addr, uint16_t Reg)
{
  uint16_t read_value = 0, tmp = 0;

  I2Cx_ReadMultiple(Addr, Reg, I2C_MEMADD_SIZE_16BIT, (uint8_t*)&read_value, 2);

  tmp = ((uint16_t)(read_value >> 8) & 0x00FF);

  tmp |= ((uint16_t)(read_value << 8)& 0xFF00);

  read_value = tmp;

  return read_value;
}

/**
  * @brief  AUDIO Codec delay
  * @param  Delay: Delay in ms
  * @retval None
  */
void AUDIO_IO_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

/******************************** LINK TS (TouchScreen) *****************************/

/**
  * @brief  Initialize I2C communication
  *         channel from MCU to TouchScreen (TS).
  */
void TS_IO_Init(void)
{
  I2Cx_Init();
}

/**
  * @brief  Writes single data with I2C communication
  *         channel from MCU to TouchScreen.
  * @param  Addr: I2C address
  * @param  Reg: Register address
  * @param  Value: Data to be written
  */
void TS_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  I2Cx_Write(Addr, Reg, Value);
}

/**
  * @brief  Reads single data with I2C communication
  *         channel from TouchScreen.
  * @param  Addr: I2C address
  * @param  Reg: Register address
  * @retval Read data
  */
uint8_t TS_IO_Read(uint8_t Addr, uint8_t Reg)
{
  return I2Cx_Read(Addr, Reg);
}

/**
  * @brief  Reads multiple data with I2C communication
  *         channel from TouchScreen.
  * @param  Addr: I2C address
  * @param  Reg: Register address
  * @param  Buffer: Pointer to data buffer
  * @param  Length: Length of the data
  * @retval Number of read data
  */
uint16_t TS_IO_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length)
{
 return I2Cx_ReadMultiple(Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, Buffer, Length);
}

/**
  * @brief  Writes multiple data with I2C communication
  *         channel from MCU to TouchScreen.
  * @param  Addr: I2C address
  * @param  Reg: Register address
  * @param  Buffer: Pointer to data buffer
  * @param  Length: Length of the data
  * @retval None
  */
void TS_IO_WriteMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length)
{
  I2Cx_WriteMultiple(Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, Buffer, Length);
}

/**
  * @brief  Delay function used in TouchScreen low level driver.
  * @param  Delay: Delay in ms
  * @retval None
  */
void TS_IO_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
