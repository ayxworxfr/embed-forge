#include "app_led_blink.h"

#include "drv_led.h"
#include "ef_event_bus.h"
#include "ef_init.h"
#include "ef_log.h"

static uint8_t s_led_state = 0;
static uint32_t s_press_count = 0;
static uint32_t s_last_logged_press_count = 0;

void app_led_blink_on_button_event(const char *topic, const void *data, uint32_t len, void *ctx)
{
    EF_UNUSED(topic);
    EF_UNUSED(data);
    EF_UNUSED(len);
    EF_UNUSED(ctx);
    s_press_count++;
}

ef_err_t app_led_blink_init(void)
{
    app_led_blink_reset();
    return ef_event_bus_subscribe(APP_EVENT_BUTTON_PRESSED, app_led_blink_on_button_event, NULL);
}
EF_INIT_EXPORT(app_led_blink_init, EF_INIT_LEVEL_APP);

void app_led_blink_step(void)
{
    drv_led_toggle();
    s_led_state = (uint8_t)(!s_led_state);
    EF_LOG_D("app", "heartbeat led=%d press_count=%lu", s_led_state, (unsigned long)s_press_count);

    if (s_press_count != s_last_logged_press_count) {
        EF_LOG_I("app", "button pressed, total=%lu", (unsigned long)s_press_count);
        s_last_logged_press_count = s_press_count;
    }
}

uint8_t app_led_blink_get_led_state(void)
{
    return s_led_state;
}

uint32_t app_led_blink_get_press_count(void)
{
    return s_press_count;
}

void app_led_blink_reset(void)
{
    s_led_state = 0;
    s_press_count = 0;
    s_last_logged_press_count = 0;
}
