/*
 * hal_uart_stm32f4.c — hal_uart.h 在 STM32F4 上的薄封装实现
 *
 * 逐字节中断接收模式：HAL_UART_Receive_IT 每次只收 1 字节，收到后在
 * HAL_UART_RxCpltCallback 里转发给上层注册的回调，再重新武装接收——
 * 这是 driver/drv_uart_console.c "回调"通信机制的硬件落地。
 */
#include "hal_uart.h"

#include "stm32f4xx_hal.h"

#ifndef EF_HAL_UART_MAX_INSTANCES
#define EF_HAL_UART_MAX_INSTANCES 3
#endif

typedef struct {
    void *instance;
    UART_HandleTypeDef huart;
    hal_uart_rx_isr_cb_t rx_cb;
    void *rx_ctx;
    uint8_t rx_byte;
    uint8_t used;
} uart_entry_t;

static uart_entry_t s_uart_table[EF_HAL_UART_MAX_INSTANCES];

static uart_entry_t *find_entry(void *instance)
{
    for (uint32_t i = 0; i < EF_HAL_UART_MAX_INSTANCES; i++) {
        if (s_uart_table[i].used && s_uart_table[i].instance == instance) {
            return &s_uart_table[i];
        }
    }
    return NULL;
}

static uint32_t translate_parity(uint8_t parity)
{
    switch (parity) {
        case 1:
            return UART_PARITY_ODD;
        case 2:
            return UART_PARITY_EVEN;
        default:
            return UART_PARITY_NONE;
    }
}

static uint32_t translate_stopbits(uint8_t stop_bits)
{
    return (stop_bits == 2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
}

ef_err_t hal_uart_init(void *instance, const hal_uart_config_t *config)
{
    if (instance == NULL || config == NULL) {
        return EF_ERR_PARAM;
    }

    uart_entry_t *entry = NULL;
    for (uint32_t i = 0; i < EF_HAL_UART_MAX_INSTANCES; i++) {
        if (!s_uart_table[i].used) {
            entry = &s_uart_table[i];
            break;
        }
    }
    if (entry == NULL) {
        return EF_ERR_NOMEM;
    }

    entry->huart.Instance = (USART_TypeDef *)instance;
    entry->huart.Init.BaudRate = config->baudrate;
    entry->huart.Init.WordLength =
        (config->data_bits == 9) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
    entry->huart.Init.StopBits = translate_stopbits(config->stop_bits);
    entry->huart.Init.Parity = translate_parity(config->parity);
    entry->huart.Init.Mode = UART_MODE_TX_RX;
    entry->huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    entry->huart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&entry->huart) != HAL_OK) {
        return EF_ERR_IO;
    }

    entry->instance = instance;
    entry->used = 1;
    entry->rx_cb = NULL;
    entry->rx_ctx = NULL;
    return EF_OK;
}

ef_err_t hal_uart_send(void *instance, const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    uart_entry_t *entry = find_entry(instance);
    if (entry == NULL || data == NULL) {
        return EF_ERR_PARAM;
    }
    HAL_StatusTypeDef st = HAL_UART_Transmit(&entry->huart, (uint8_t *)data, len, timeout_ms);
    return (st == HAL_OK) ? EF_OK : (st == HAL_TIMEOUT ? EF_ERR_TIMEOUT : EF_ERR_IO);
}

ef_err_t hal_uart_recv(void *instance, uint8_t *buf, uint16_t len, uint32_t timeout_ms)
{
    uart_entry_t *entry = find_entry(instance);
    if (entry == NULL || buf == NULL) {
        return EF_ERR_PARAM;
    }
    HAL_StatusTypeDef st = HAL_UART_Receive(&entry->huart, buf, len, timeout_ms);
    return (st == HAL_OK) ? EF_OK : (st == HAL_TIMEOUT ? EF_ERR_TIMEOUT : EF_ERR_IO);
}

ef_err_t hal_uart_register_rx_isr(void *instance, hal_uart_rx_isr_cb_t cb, void *ctx)
{
    uart_entry_t *entry = find_entry(instance);
    if (entry == NULL) {
        return EF_ERR_PARAM;
    }
    entry->rx_cb = cb;
    entry->rx_ctx = ctx;
    /* 对应 IRQn 的 NVIC 使能由 board_init() 负责（board 层才知道具体是哪个 IRQn），
     * 这里只负责武装第一次接收 */
    return (HAL_UART_Receive_IT(&entry->huart, &entry->rx_byte, 1) == HAL_OK) ? EF_OK : EF_ERR_IO;
}

ef_err_t hal_uart_deinit(void *instance)
{
    uart_entry_t *entry = find_entry(instance);
    if (entry == NULL) {
        return EF_ERR_PARAM;
    }
    HAL_UART_DeInit(&entry->huart);
    entry->used = 0;
    return EF_OK;
}

/* board 层 stm32f4xx_it.c 的 USARTx_IRQHandler 转发到这里 */
void hal_uart_stm32f4_irq_handler(void *instance)
{
    uart_entry_t *entry = find_entry(instance);
    if (entry != NULL) {
        HAL_UART_IRQHandler(&entry->huart);
    }
}

/* ST HAL 弱回调：一字节接收完成后转发给上层，再重新武装下一字节接收 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    for (uint32_t i = 0; i < EF_HAL_UART_MAX_INSTANCES; i++) {
        uart_entry_t *entry = &s_uart_table[i];
        if (entry->used && &entry->huart == huart) {
            if (entry->rx_cb != NULL) {
                entry->rx_cb(entry->rx_byte, entry->rx_ctx);
            }
            HAL_UART_Receive_IT(&entry->huart, &entry->rx_byte, 1);
            return;
        }
    }
}
