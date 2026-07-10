/*
 * drv_led.h — LED 驱动：对上暴露"点灯/灭灯"语义，而不是"拉高 GPIO"
 */
#ifndef DRV_LED_H
#define DRV_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

/* 注册为 ef_device，名字 "led0"；也提供直接函数调用形式，两种风格都支持 */
ef_err_t drv_led_init(void);
ef_err_t drv_led_on(void);
ef_err_t drv_led_off(void);
ef_err_t drv_led_toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_LED_H */
