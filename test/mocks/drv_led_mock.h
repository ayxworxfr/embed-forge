/*
 * drv_led_mock.h — drv_led.h 的 host 测试替身
 *
 * 只在 PC 单元测试里使用：提供和真实 drv_led 完全一样的对外函数签名
 * （app_led_blink.c 不用知道自己链接的是真驱动还是这个 mock），额外
 * 暴露几个"读内部状态"的测试专用接口，方便断言。
 */
#ifndef DRV_LED_MOCK_H
#define DRV_LED_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

uint8_t drv_led_mock_get_level(void);
uint32_t drv_led_mock_get_toggle_count(void);
void drv_led_mock_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_LED_MOCK_H */
