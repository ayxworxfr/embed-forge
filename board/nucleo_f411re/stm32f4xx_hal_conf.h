/*
 * stm32f4xx_hal_conf.h — ST HAL 库的项目级配置
 *
 * 按需开启模块（可裁剪原则）。除业务直接使用的 RCC/GPIO/UART/CORTEX 外，
 * DMA/FLASH 也需开启：UART 头文件结构体含 DMA_HandleTypeDef，HAL_Init()
 * 依赖 __HAL_FLASH_* 宏——这是 ST HAL 头文件级联依赖，不代表固件启用 DMA 传输。
 *
 * 末尾必须 include 已启用模块的头文件（见 ST stm32f4xx_hal_conf_template.h）。
 */
#ifndef STM32F4XX_HAL_CONF_H
#define STM32F4XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED /* UART_HandleTypeDef 需要 DMA_HandleTypeDef */
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED /* HAL_Init() 需要 __HAL_FLASH_* 宏 */
#define HAL_UART_MODULE_ENABLED

#if !defined(HSE_VALUE)
#define HSE_VALUE 8000000U /* Nucleo-F411RE 上通过 ST-LINK MCO 提供，参考实现未使用 HSE */
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT 100U
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE 16000000U
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE 32000U
#endif

#if !defined(LSE_VALUE)
#define LSE_VALUE 32768U
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT 5000U
#endif

#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE 12288000U
#endif

#define VDD_VALUE 3300U
#define TICK_INT_PRIORITY 0x0FU
#define USE_RTOS 0U
#define PREFETCH_ENABLE 1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE 1U

#include "stm32f4xx.h"

/* HAL 库要求提供 assert_param 宏，接到本脚手架统一的 EF_ASSERT */
#include "ef_common.h"

/* 已启用模块头文件：顺序与 ST 官方模板一致，DMA 必须在 UART 之前 */
#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f4xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f4xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32f4xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f4xx_hal_cortex.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f4xx_hal_flash.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f4xx_hal_uart.h"
#endif

#define assert_param(expr) EF_ASSERT(expr)

#ifdef __cplusplus
}
#endif

#endif /* STM32F4XX_HAL_CONF_H */
