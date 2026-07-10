/*
 * ef_event_bus.h — 发布-订阅（一对多通知）
 *
 * 典型用法：drv_button 的按键回调里，app 层调用
 * ef_event_bus_publish("btn_pressed", ...)，log 组件和 app 的业务
 * 逻辑各自订阅同一个 topic，互不知道对方存在——这是"发布订阅"区别于
 * "回调"的关键：发布者不持有、也不关心具体订阅者是谁。
 */
#ifndef EF_EVENT_BUS_H
#define EF_EVENT_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

typedef void (*ef_event_cb_t)(const char *topic, const void *data, uint32_t len, void *ctx);

ef_err_t ef_event_bus_init(void);

/** 订阅一个 topic（精确字符串匹配），同一个 topic 可以有多个订阅者 */
ef_err_t ef_event_bus_subscribe(const char *topic, ef_event_cb_t cb, void *ctx);

/** 取消订阅：cb+ctx 必须和订阅时完全一致 */
ef_err_t ef_event_bus_unsubscribe(const char *topic, ef_event_cb_t cb, void *ctx);

/** 发布事件：同步遍历调用所有匹配 topic 的订阅者回调 */
ef_err_t ef_event_bus_publish(const char *topic, const void *data, uint32_t len);

/** 仅用于单元测试：清空订阅表 */
void ef_event_bus_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* EF_EVENT_BUS_H */
