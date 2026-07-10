/*
 * board_config.h — Nucleo-F411RE 引脚/外设映射
 *
 * 这是整个仓库里唯一"知道具体引脚接在哪"的地方。换一块板子，只需要
 * 新增一个 board/<board_name>/ 目录、改这一个文件，hal/ 和上层代码
 * 完全不用动。
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* LD2（板载绿色 LED） */
#define BOARD_LED_PORT GPIOA
#define BOARD_LED_PIN GPIO_PIN_5

/* B1（板载用户按键，蓝色，按下为低电平，外部下拉不需要，芯片内部上拉） */
#define BOARD_BUTTON_PORT GPIOC
#define BOARD_BUTTON_PIN GPIO_PIN_13
#define BOARD_BUTTON_EXTI_IRQn EXTI15_10_IRQn

/* USART2：通过板载 ST-LINK 虚拟串口(VCP)直连 PC，无需额外接线 */
#define BOARD_CONSOLE_UART_PORT USART2
#define BOARD_CONSOLE_UART_TX_PORT GPIOA
#define BOARD_CONSOLE_UART_TX_PIN GPIO_PIN_2
#define BOARD_CONSOLE_UART_RX_PORT GPIOA
#define BOARD_CONSOLE_UART_RX_PIN GPIO_PIN_3
#define BOARD_CONSOLE_UART_AF GPIO_AF7_USART2
#define BOARD_CONSOLE_UART_IRQn USART2_IRQn

void board_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_CONFIG_H */
