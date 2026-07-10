#include "drv_led_mock.h"

#include "drv_led.h"

static uint8_t s_level = 0;
static uint32_t s_toggle_count = 0;

ef_err_t drv_led_init(void)
{
    drv_led_mock_reset();
    return EF_OK;
}

ef_err_t drv_led_on(void)
{
    s_level = 1;
    return EF_OK;
}

ef_err_t drv_led_off(void)
{
    s_level = 0;
    return EF_OK;
}

ef_err_t drv_led_toggle(void)
{
    s_level = (uint8_t)(!s_level);
    s_toggle_count++;
    return EF_OK;
}

uint8_t drv_led_mock_get_level(void)
{
    return s_level;
}

uint32_t drv_led_mock_get_toggle_count(void)
{
    return s_toggle_count;
}

void drv_led_mock_reset(void)
{
    s_level = 0;
    s_toggle_count = 0;
}
