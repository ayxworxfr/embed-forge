/*
 * hal_uart.h — UART 硬件抽象层接口（跨芯片契约）
 *
 * instance 是不透明句柄（stm32f4xx 下直接是 USARTx 外设基地址转型而来，
 * 例如 board_config.h 里的 BOARD_CONSOLE_UART_PORT 就是 USART2），
 * HAL 层本身不知道、也不关心"哪个 UART 接了什么功能"，那是 board 层
 * 的职责。
 */
#ifndef HAL_UART_H
#define HAL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

typedef struct {
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity; /* 0=None 1=Odd 2=Even */
} hal_uart_config_t;

typedef void (*hal_uart_rx_isr_cb_t)(uint8_t byte, void *ctx);

ef_err_t hal_uart_init(void *instance, const hal_uart_config_t *config);
ef_err_t hal_uart_send(void *instance, const uint8_t *data, uint16_t len, uint32_t timeout_ms);
ef_err_t hal_uart_recv(void *instance, uint8_t *buf, uint16_t len, uint32_t timeout_ms);

/** 注册"收到 1 字节"回调，内部用 HAL_UART_Receive_IT 实现逐字节中断接收 */
ef_err_t hal_uart_register_rx_isr(void *instance, hal_uart_rx_isr_cb_t cb, void *ctx);

ef_err_t hal_uart_deinit(void *instance);

#ifdef __cplusplus
}
#endif

#endif /* HAL_UART_H */
