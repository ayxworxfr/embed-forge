#include "drv_button.h"

#include "board_config.h"
#include "ef_device.h"
#include "ef_init.h"
#include "hal_gpio.h"

static ef_device_t s_button_device;
static drv_button_pressed_cb_t s_user_cb = NULL;
static void *s_user_ctx = NULL;

static void on_hal_irq(void *ctx)
{
    EF_UNUSED(ctx);
    if (s_user_cb != NULL) {
        s_user_cb(s_user_ctx);
    }
}

static int32_t button_read_op(ef_device_t *dev, void *buf, uint32_t len)
{
    EF_UNUSED(dev);
    if (buf == NULL || len == 0) {
        return (int32_t)EF_ERR_PARAM;
    }
    ((uint8_t *)buf)[0] = drv_button_read();
    return 1;
}

static const ef_device_ops_t s_button_ops = {
    .init = NULL,
    .open = NULL,
    .close = NULL,
    .read = button_read_op,
    .write = NULL,
    .control = NULL,
};

ef_err_t drv_button_init(void)
{
    ef_err_t rc = ef_device_register(&s_button_device, "btn0", &s_button_ops, NULL);
    if (rc != EF_OK) {
        return rc;
    }
    return hal_gpio_register_irq(BOARD_BUTTON_PORT, BOARD_BUTTON_PIN, on_hal_irq, NULL);
}
EF_INIT_EXPORT(drv_button_init, EF_INIT_LEVEL_DRIVER);

ef_err_t drv_button_register_callback(drv_button_pressed_cb_t cb, void *ctx)
{
    s_user_cb = cb;
    s_user_ctx = ctx;
    return EF_OK;
}

uint8_t drv_button_read(void)
{
    return hal_gpio_read(BOARD_BUTTON_PORT, BOARD_BUTTON_PIN);
}
