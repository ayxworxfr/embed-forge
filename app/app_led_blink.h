/*
 * app_led_blink.h — 示例业务：按键 -> 事件总线 -> LED + 日志 + CLI
 *
 * 这是本脚手架"App 层只关心业务语义，不关心硬件/OS 细节"的示范：
 * app_led_blink_step() 只调用 Service/Driver 层的语义化接口
 * （drv_led_toggle / EF_LOG_I / ef_event_bus_publish），完全不出现
 * GPIO/寄存器/OSAL 原语。
 *
 * app_led_blink_step() 被设计成"单次执行、快速返回"的轮询函数（不是
 * 内部 while(1)），这样同一份代码可以：
 *   - 直接被 bare 后端的 app_main 主循环反复调用；
 *   - 也可以被 freertos 后端里一个 `while(1){ step(); delay(); }`
 *     的任务包一层调用；
 *   - 也可以被 PC 单元测试直接调用，断言按键事件触发后的状态变化。
 */
#ifndef APP_LED_BLINK_H
#define APP_LED_BLINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

/** 事件总线上的 topic 名，按键中断回调发布，本模块和 log 都会订阅 */
#define APP_EVENT_BUTTON_PRESSED "btn_pressed"

ef_err_t app_led_blink_init(void);

/** 主循环每次调用一次：周期性心跳闪烁 + 处理累计的按键事件计数 */
void app_led_blink_step(void);

/** 供中断回调/事件总线调用：记录一次按键按下 */
void app_led_blink_on_button_event(const char *topic, const void *data, uint32_t len, void *ctx);

/** 仅用于单元测试：读取当前 LED 逻辑状态（0/1）和累计按键次数 */
uint8_t app_led_blink_get_led_state(void);
uint32_t app_led_blink_get_press_count(void);

/** 仅用于单元测试：重置内部状态 */
void app_led_blink_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_LED_BLINK_H */
