/*
 * drv_button.h — 按键驱动：中断触发后通过回调通知上层，驱动本身不知道
 * "上层拿到通知要做什么"（可能是发事件、可能是记日志），这正是
 * "回调"这种层间通信机制的意义：下层只持有函数指针，不 include 上层。
 */
#ifndef DRV_BUTTON_H
#define DRV_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

typedef void (*drv_button_pressed_cb_t)(void *ctx);

ef_err_t drv_button_init(void);

/** 按键按下（下降沿）时触发；ctx 原样透传给回调，不解释其含义 */
ef_err_t drv_button_register_callback(drv_button_pressed_cb_t cb, void *ctx);

/** 轮询读当前电平：1=松开 0=按下（内部上拉，按下拉低） */
uint8_t drv_button_read(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_BUTTON_H */
