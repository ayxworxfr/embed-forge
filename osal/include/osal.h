/*
 * osal.h — OS 抽象层统一接口
 *
 * 业务代码（Driver/Service/App）只允许 include 本文件，禁止直接调用
 * FreeRTOS API（xSemaphoreGive 等）或裸机的关中断操作。三套后端二选一
 * 在编译期通过 CMake 变量 EF_OSAL_BACKEND=bare|freertos|host 决定，
 * 业务代码源码零改动。
 */
#ifndef EF_OSAL_H
#define EF_OSAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

#define OSAL_WAIT_FOREVER 0xFFFFFFFFu
#define OSAL_NO_WAIT 0u

typedef void *osal_mutex_t;
typedef void *osal_queue_t;
typedef void *osal_thread_t;

typedef void (*osal_thread_entry_t)(void *arg);

/* ---------------- 互斥锁 ---------------- */
ef_err_t osal_mutex_create(osal_mutex_t *out_mutex);
ef_err_t osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms);
ef_err_t osal_mutex_unlock(osal_mutex_t mutex);
ef_err_t osal_mutex_delete(osal_mutex_t mutex);

/* ---------------- 消息队列 ---------------- */
ef_err_t osal_queue_create(osal_queue_t *out_queue, uint32_t item_count, uint32_t item_size);
ef_err_t osal_queue_send(osal_queue_t queue, const void *item, uint32_t timeout_ms);
ef_err_t osal_queue_send_from_isr(osal_queue_t queue, const void *item);
ef_err_t osal_queue_receive(osal_queue_t queue, void *out_item, uint32_t timeout_ms);
ef_err_t osal_queue_delete(osal_queue_t queue);

/* ---------------- 线程/任务 ---------------- */
ef_err_t osal_thread_create(osal_thread_t *out_thread, const char *name, osal_thread_entry_t entry,
                            void *arg, uint32_t stack_size_words, uint8_t priority);
ef_err_t osal_thread_delete(osal_thread_t thread);

/* ---------------- 时间 ---------------- */
void osal_delay_ms(uint32_t ms);
uint32_t osal_tick_ms(void);

/* ---------------- 临界区（关中断量级） ---------------- */
void osal_enter_critical(void);
void osal_exit_critical(void);

/*
 * bare 后端专用：没有真正的抢占式调度器，用协作式轮转模拟"线程"。
 * 必须在 app_main 的主循环里反复调用，才能驱动所有 osal_thread_create
 * 注册的入口函数轮转执行。freertos/host 后端下这是空操作。
 */
void osal_scheduler_run_once(void);

#ifdef __cplusplus
}
#endif

#endif /* EF_OSAL_H */
