/*
 * hal_gpio_stm32f4.c — hal_gpio.h 在 STM32F4 上的薄封装实现
 *
 * 只做"翻译"：把跨芯片的 ef_gpio_mode_t/ef_gpio_pull_t 翻译成 ST 官方
 * HAL 库的 GPIO_InitTypeDef，不重新实现寄存器操作——直接复用
 * STM32F4xx_HAL_Driver，这是文档里"HAL 层 -> STM32 HAL_UART_Receive()"
 * 这种薄封装思路的具体落地。
 */
#include "hal_gpio.h"

#include "stm32f4xx_hal.h"

#ifndef EF_HAL_GPIO_MAX_IRQ
#define EF_HAL_GPIO_MAX_IRQ 8
#endif

typedef struct {
    uint16_t pin;
    hal_gpio_irq_cb_t cb;
    void *ctx;
    uint8_t used;
} gpio_irq_entry_t;

static gpio_irq_entry_t s_irq_table[EF_HAL_GPIO_MAX_IRQ];

static uint32_t translate_mode(ef_gpio_mode_t mode)
{
    switch (mode) {
        case EF_GPIO_MODE_INPUT:
            return GPIO_MODE_INPUT;
        case EF_GPIO_MODE_OUTPUT_PP:
            return GPIO_MODE_OUTPUT_PP;
        case EF_GPIO_MODE_IT_RISING:
            return GPIO_MODE_IT_RISING;
        case EF_GPIO_MODE_IT_FALLING:
            return GPIO_MODE_IT_FALLING;
        default:
            return GPIO_MODE_INPUT;
    }
}

static uint32_t translate_pull(ef_gpio_pull_t pull)
{
    switch (pull) {
        case EF_GPIO_PULL_UP:
            return GPIO_PULLUP;
        case EF_GPIO_PULL_DOWN:
            return GPIO_PULLDOWN;
        case EF_GPIO_PULL_NONE:
        default:
            return GPIO_NOPULL;
    }
}

ef_err_t hal_gpio_init(const hal_gpio_config_t *config)
{
    if (config == NULL || config->port == NULL) {
        return EF_ERR_PARAM;
    }

    GPIO_InitTypeDef init = {0};
    init.Pin = config->pin;
    init.Mode = translate_mode(config->mode);
    init.Pull = translate_pull(config->pull);
    init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init((GPIO_TypeDef *)config->port, &init);
    return EF_OK;
}

ef_err_t hal_gpio_write(void *port, uint32_t pin, uint8_t level)
{
    if (port == NULL) {
        return EF_ERR_PARAM;
    }
    HAL_GPIO_WritePin((GPIO_TypeDef *)port, (uint16_t)pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return EF_OK;
}

uint8_t hal_gpio_read(void *port, uint32_t pin)
{
    if (port == NULL) {
        return 0;
    }
    return (HAL_GPIO_ReadPin((GPIO_TypeDef *)port, (uint16_t)pin) == GPIO_PIN_SET) ? 1 : 0;
}

ef_err_t hal_gpio_toggle(void *port, uint32_t pin)
{
    if (port == NULL) {
        return EF_ERR_PARAM;
    }
    HAL_GPIO_TogglePin((GPIO_TypeDef *)port, (uint16_t)pin);
    return EF_OK;
}

ef_err_t hal_gpio_register_irq(void *port, uint32_t pin, hal_gpio_irq_cb_t cb, void *ctx)
{
    EF_UNUSED(port); /* STM32 EXTI 按 pin 编号共享中断线，不区分具体 port */

    for (uint32_t i = 0; i < EF_HAL_GPIO_MAX_IRQ; i++) {
        if (!s_irq_table[i].used) {
            s_irq_table[i].used = 1;
            s_irq_table[i].pin = (uint16_t)pin;
            s_irq_table[i].cb = cb;
            s_irq_table[i].ctx = ctx;
            return EF_OK;
        }
    }
    return EF_ERR_NOMEM;
}

/* ST HAL 弱回调：EXTI 中断触发后由 HAL_GPIO_EXTI_IRQHandler 调用 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    for (uint32_t i = 0; i < EF_HAL_GPIO_MAX_IRQ; i++) {
        if (s_irq_table[i].used && s_irq_table[i].pin == GPIO_Pin && s_irq_table[i].cb != NULL) {
            s_irq_table[i].cb(s_irq_table[i].ctx);
        }
    }
}
