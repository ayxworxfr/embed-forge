#include "drv_uart_console.h"

#include "board_config.h"
#include "ef_device.h"
#include "ef_init.h"
#include "hal_uart.h"
#include "osal.h"

#define DRV_UART_CONSOLE_RX_QUEUE_LEN 64

static ef_device_t s_uart_device;
static osal_queue_t s_rx_queue = NULL;

static void on_hal_rx_byte(uint8_t byte, void *ctx)
{
    EF_UNUSED(ctx);
    /* ISR 上下文：只做"存数据"，不做任何解析/日志/业务逻辑 */
    osal_queue_send_from_isr(s_rx_queue, &byte);
}

static int32_t uart_write_op(ef_device_t *dev, const void *buf, uint32_t len)
{
    EF_UNUSED(dev);
    ef_err_t rc = drv_uart_console_write((const uint8_t *)buf, (uint16_t)len, 1000);
    return (rc == EF_OK) ? (int32_t)len : (int32_t)rc;
}

static int32_t uart_read_op(ef_device_t *dev, void *buf, uint32_t len)
{
    EF_UNUSED(dev);
    if (buf == NULL) {
        return (int32_t)EF_ERR_PARAM;
    }
    uint32_t n = 0;
    uint8_t *out = (uint8_t *)buf;
    while (n < len) {
        if (drv_uart_console_read_byte(&out[n], OSAL_NO_WAIT) != EF_OK) {
            break;
        }
        n++;
    }
    return (int32_t)n;
}

static const ef_device_ops_t s_uart_ops = {
    .init = NULL,
    .open = NULL,
    .close = NULL,
    .read = uart_read_op,
    .write = uart_write_op,
    .control = NULL,
};

ef_err_t drv_uart_console_init(void)
{
    ef_err_t rc = osal_queue_create(&s_rx_queue, DRV_UART_CONSOLE_RX_QUEUE_LEN, sizeof(uint8_t));
    if (rc != EF_OK) {
        return rc;
    }

    rc = ef_device_register(&s_uart_device, "uart0", &s_uart_ops, NULL);
    if (rc != EF_OK) {
        return rc;
    }

    return hal_uart_register_rx_isr(BOARD_CONSOLE_UART_PORT, on_hal_rx_byte, NULL);
}
EF_INIT_EXPORT(drv_uart_console_init, EF_INIT_LEVEL_DRIVER);

ef_err_t drv_uart_console_write(const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    return hal_uart_send(BOARD_CONSOLE_UART_PORT, data, len, timeout_ms);
}

ef_err_t drv_uart_console_read_byte(uint8_t *out_byte, uint32_t timeout_ms)
{
    if (out_byte == NULL || s_rx_queue == NULL) {
        return EF_ERR_PARAM;
    }
    return osal_queue_receive(s_rx_queue, out_byte, timeout_ms);
}
