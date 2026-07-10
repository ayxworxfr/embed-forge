#include "drv_led.h"

#include "board_config.h"
#include "ef_device.h"
#include "ef_init.h"
#include "hal_gpio.h"

#define DRV_LED_CTRL_TOGGLE 1

static ef_device_t s_led_device;

static int32_t led_write(ef_device_t *dev, const void *buf, uint32_t len)
{
    EF_UNUSED(dev);
    if (buf == NULL || len == 0) {
        return (int32_t)EF_ERR_PARAM;
    }
    uint8_t level = ((const uint8_t *)buf)[0];
    hal_gpio_write(BOARD_LED_PORT, BOARD_LED_PIN, level ? 1 : 0);
    return 1;
}

static ef_err_t led_control(ef_device_t *dev, int32_t cmd, void *arg)
{
    EF_UNUSED(dev);
    EF_UNUSED(arg);
    if (cmd == DRV_LED_CTRL_TOGGLE) {
        return hal_gpio_toggle(BOARD_LED_PORT, BOARD_LED_PIN);
    }
    return EF_ERR_NOTSUPPORTED;
}

static const ef_device_ops_t s_led_ops = {
    .init = NULL,
    .open = NULL,
    .close = NULL,
    .read = NULL,
    .write = led_write,
    .control = led_control,
};

ef_err_t drv_led_init(void)
{
    return ef_device_register(&s_led_device, "led0", &s_led_ops, NULL);
}
EF_INIT_EXPORT(drv_led_init, EF_INIT_LEVEL_DRIVER);

ef_err_t drv_led_on(void)
{
    return hal_gpio_write(BOARD_LED_PORT, BOARD_LED_PIN, 1);
}

ef_err_t drv_led_off(void)
{
    return hal_gpio_write(BOARD_LED_PORT, BOARD_LED_PIN, 0);
}

ef_err_t drv_led_toggle(void)
{
    return hal_gpio_toggle(BOARD_LED_PORT, BOARD_LED_PIN);
}
