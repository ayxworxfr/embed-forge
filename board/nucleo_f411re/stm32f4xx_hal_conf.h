/*
 * stm32f4xx_hal_conf.h — ST HAL 库的项目级配置
 *
 * 按需开启模块（可裁剪原则）：本参考实现只用到 RCC/GPIO/UART/CORTEX，
 * 其他模块全部关闭，减小编译体积、减少无关依赖。新增外设时在这里
 * 打开对应 HAL_xxx_MODULE_ENABLED 即可，不需要改 HAL 库源码。
 */
#ifndef STM32F4XX_HAL_CONF_H
#define STM32F4XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED

#if !defined(HSE_VALUE)
#define HSE_VALUE 8000000U /* Nucleo-F411RE 上通过 ST-LINK MCO 提供，参考实现未使用 HSE */
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE 16000000U
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE 32000U
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
#define assert_param(expr) EF_ASSERT(expr)

#ifdef __cplusplus
}
#endif

#endif /* STM32F4XX_HAL_CONF_H */
