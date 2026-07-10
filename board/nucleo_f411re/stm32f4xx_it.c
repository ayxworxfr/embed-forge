/*
 * stm32f4xx_it.c — Cortex-M4 中断向量的具体实现
 *
 * 命名必须和 startup_stm32f411xe.s 向量表里的符号完全一致（CMSIS 惯例）。
 *
 * 已知限制（EF_OSAL_BACKEND=freertos 时）：SysTick 被 FreeRTOS 独占用于
 * RTOS tick（见 FreeRTOSConfig.h 的 xPortSysTickHandler 重映射），
 * 这里不再调用 HAL_IncTick()，会导致 ST HAL 库内部基于 HAL_GetTick()
 * 的超时判断永远不会触发（本质退化为"一直等到硬件标志位就位"，功能
 * 上仍然正确，只是失去了超时保护）。生产项目如果需要保留 HAL 超时
 * 语义，扩展点是用一个空闲的 TIM 覆盖 HAL_InitTick()，这里不引入
 * 这个复杂度，在 docs/architecture.md 里已注明。
 */
#include "stm32f4xx_hal.h"

#include "board_config.h"

#if defined(EF_OSAL_BACKEND_BARE)
/* osal/bare 内部维护的 tick 计数器，仅这里需要，不放进 osal.h 公共接口 */
extern void osal_bare_tick_inc(void);

void SysTick_Handler(void)
{
    HAL_IncTick();
    osal_bare_tick_inc();
}
#endif

void HardFault_Handler(void)
{
    for (;;) {
    }
}

void MemManage_Handler(void)
{
    for (;;) {
    }
}

void BusFault_Handler(void)
{
    for (;;) {
    }
}

void UsageFault_Handler(void)
{
    for (;;) {
    }
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BOARD_BUTTON_PIN);
}

void USART2_IRQHandler(void)
{
    extern void hal_uart_stm32f4_irq_handler(void *instance);
    hal_uart_stm32f4_irq_handler(BOARD_CONSOLE_UART_PORT);
}
