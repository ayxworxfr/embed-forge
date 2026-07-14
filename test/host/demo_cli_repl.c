/*
 * demo_cli_repl.c — Demo 1：PC 上交互式 CLI，不依赖真机硬件
 *
 * 从 stdin 逐字节喂给 ef_cli_feed_byte()，演示事件总线如何把 CLI 命令
 * "press" 和 app_led_blink 业务状态串起来。
 */
#include <stdio.h>
#include <stdarg.h>

#include "app_led_blink.h"
#include "drv_led.h"
#include "ef_cli.h"
#include "ef_event_bus.h"
#include "ef_log.h"

static ef_cli_output_t s_cli_output;

static void stdout_output(const char *data, uint32_t len)
{
    fwrite(data, 1, len, stdout);
    fflush(stdout);
}

static void stdout_log_sink(const char *line, uint32_t len)
{
    fwrite(line, 1, len, stdout);
    fflush(stdout);
}

static void demo_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0 && s_cli_output != NULL) {
        s_cli_output(buf, (uint32_t)n);
    }
}

static int cmd_press(int argc, char **argv)
{
    EF_UNUSED(argc);
    EF_UNUSED(argv);
    ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, NULL, 0);
    demo_printf("button event published\r\n");
    return 0;
}

static int cmd_status(int argc, char **argv)
{
    EF_UNUSED(argc);
    EF_UNUSED(argv);
    demo_printf("led=%u press_count=%lu\r\n", app_led_blink_get_led_state(),
                (unsigned long)app_led_blink_get_press_count());
    return 0;
}

static void demo_init(void)
{
    ef_event_bus_init();
    ef_log_init();
    ef_log_set_sink(stdout_log_sink);
    ef_log_set_level(EF_LOG_LEVEL_INFO);

    ef_cli_init();
    s_cli_output = stdout_output;
    ef_cli_set_output(stdout_output);
    ef_cli_register("press", cmd_press, "simulate button press");
    ef_cli_register("status", cmd_status, "show led state and press count");

    drv_led_init();
    app_led_blink_init();
}

int main(void)
{
    int ch;

    demo_init();
    demo_printf("embed-forge demo CLI — type 'help' for commands\r\n");

    while ((ch = getchar()) != EOF) {
        ef_cli_feed_byte((uint8_t)ch);
    }

    return 0;
}
