/*
 * drv_uart_console.h — 控制台 UART 驱动
 *
 * 层间通信机制在这里的落地：hal_uart 的中断回调（"回调"）只做一件事——
 * 把收到的字节丢进内部的 osal_queue_t（"消息队列"），真正的消费者
 * （service/cli）用 drv_uart_console_read_byte() 阻塞/带超时地取数据，
 * 彼此完全解耦。
 */
#ifndef DRV_UART_CONSOLE_H
#define DRV_UART_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

ef_err_t drv_uart_console_init(void);

/** 发送一段数据，阻塞直到发完或超时 */
ef_err_t drv_uart_console_write(const uint8_t *data, uint16_t len, uint32_t timeout_ms);

/** 从内部接收队列取 1 字节，timeout_ms 支持 OSAL_NO_WAIT / OSAL_WAIT_FOREVER */
ef_err_t drv_uart_console_read_byte(uint8_t *out_byte, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* DRV_UART_CONSOLE_H */
