/*
 * app_main.c — 固件入口
 *
 * 职责严格限定在"接线"：调用 board_init() 完成硬件bring-up，调用
 * ef_init_run_all() 让所有 EF_INIT_EXPORT 注册的模块完成自注册初始化，
 * 再把几个"必须显式接线、自动注册机制管不到"的跨层胶水连起来
 * （log 输出到哪个设备、按键回调发布到哪个 topic、CLI 命令表）。
 *
 * 除了这几行胶水代码，本文件不包含任何业务逻辑——业务逻辑全部在
 * app_led_blink.c 里，且那份代码在 PC 单测里被直接复用、零改动。
 */
#include <string.h>

#include "board_config.h"
#include "drv_button.h"
#include "drv_led.h"
#include "drv_uart_console.h"
#include "ef_cli.h"
#include "ef_common.h"
#include "ef_event_bus.h"
#include "ef_init.h"
#include "ef_log.h"
#include "osal.h"

#include "app_led_blink.h"

#define APP_STEP_PERIOD_MS 500u

static void uart_console_output(const char *data, uint32_t len)
{
    drv_uart_console_write((const uint8_t *)data, (uint16_t)len, 1000);
}

static void on_button_isr(void *ctx)
{
    EF_UNUSED(ctx);
    ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, NULL, 0);
}

static int cli_cmd_led(int argc, char **argv)
{
    if (argc < 2) {
        return -1;
    }
    if (strcmp(argv[1], "on") == 0) {
        drv_led_on();
    } else if (strcmp(argv[1], "off") == 0) {
        drv_led_off();
    } else if (strcmp(argv[1], "toggle") == 0) {
        drv_led_toggle();
    }
    return 0;
}

static void app_wiring_init(void)
{
    ef_log_set_sink(uart_console_output);
    ef_cli_set_output(uart_console_output);
    ef_cli_register("led", cli_cmd_led, "led <on|off|toggle>");
    drv_button_register_callback(on_button_isr, NULL);
}

static void app_task_entry(void *arg)
{
    EF_UNUSED(arg);
    for (;;) {
        app_led_blink_step();

        uint8_t byte;
        while (drv_uart_console_read_byte(&byte, OSAL_NO_WAIT) == EF_OK) {
            ef_cli_feed_byte(byte);
        }

        osal_delay_ms(APP_STEP_PERIOD_MS);
    }
}

int main(void)
{
    board_init();
    ef_init_run_all();
    app_wiring_init();

    EF_LOG_I("app", "embed-forge boot ok");

    osal_thread_t app_thread;
    osal_thread_create(&app_thread, "app", app_task_entry, NULL, 512, 5);

    /* bare 后端：等价于直接进入 app_task_entry 的 while(1)；
     * freertos 后端：启动内核抢占式调度，正常情况下不会返回。
     * 详见 osal/bare/osal_bare.c、osal/freertos/osal_freertos.c 顶部注释。 */
    for (;;) {
        osal_scheduler_run_once();
    }
}
