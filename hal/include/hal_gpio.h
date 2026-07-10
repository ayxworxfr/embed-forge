/*
 * hal_gpio.h — GPIO 硬件抽象层接口（跨芯片契约）
 *
 * 设计原则："面向接口编程"：port/pin 用不透明指针/整数表示，本文件
 * 不 include 任何芯片头文件。stm32f4xx 实现内部把 port 转型回
 * GPIO_TypeDef*；换一颗芯片，只需要重新实现 hal/<chip>/hal_gpio_*.c，
 * 本头文件、以及所有调用方代码都不需要改一行。
 */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

typedef enum {
    EF_GPIO_MODE_INPUT,
    EF_GPIO_MODE_OUTPUT_PP,
    EF_GPIO_MODE_IT_RISING,
    EF_GPIO_MODE_IT_FALLING,
} ef_gpio_mode_t;

typedef enum {
    EF_GPIO_PULL_NONE,
    EF_GPIO_PULL_UP,
    EF_GPIO_PULL_DOWN,
} ef_gpio_pull_t;

typedef struct {
    void *port; /* 芯片相关：stm32f4xx 下是 GPIO_TypeDef* 转型而来 */
    uint32_t pin;
    ef_gpio_mode_t mode;
    ef_gpio_pull_t pull;
} hal_gpio_config_t;

typedef void (*hal_gpio_irq_cb_t)(void *ctx);

ef_err_t hal_gpio_init(const hal_gpio_config_t *config);
ef_err_t hal_gpio_write(void *port, uint32_t pin, uint8_t level);
uint8_t hal_gpio_read(void *port, uint32_t pin);
ef_err_t hal_gpio_toggle(void *port, uint32_t pin);

/** 注册中断回调，config.mode 必须是 EF_GPIO_MODE_IT_RISING/FALLING */
ef_err_t hal_gpio_register_irq(void *port, uint32_t pin, hal_gpio_irq_cb_t cb, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* HAL_GPIO_H */
