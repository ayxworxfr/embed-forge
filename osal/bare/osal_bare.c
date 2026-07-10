/*
 * osal_bare.c — 裸机后端实现
 *
 * 重要限制（刻意设计，不是缺陷）：
 * 裸机没有真正的抢占式调度器和独立堆栈，osal_thread_create 只是把入口
 * 函数登记进一张静态表；osal_scheduler_run_once() 依次调用每个已注册
 * 入口函数一次。这意味着：
 *   - 如果只创建 1 个"线程"（本脚手架 App 层示例的用法），且它内部是
 *     传统 RTOS 风格的 `while(1) { work(); osal_delay_ms(...); }`，
 *     效果等价于该函数自己接管了主循环——完全正确，这正是裸机"超级循环"
 *     的标准写法。
 *   - 如果创建 2 个以上这种"内部 while(1)"的线程，第一个会永远占住
 *     CPU，后面的入口永远不会被调用到（因为没有真正的上下文切换）。
 *     裸机本来就不适合"多个各自死循环的任务"这种并发模型；真正需要
 *     多任务并发时，请切换 EF_OSAL_BACKEND=freertos，业务代码不用改。
 * - mutex 用关中断实现的测试并设置（单核 MCU 上足够安全）。
 * - queue 用静态 arena 一次性分配存储（禁止运行期 malloc/free）。
 * - tick 由 SysTick 中断驱动，靠 board 层调用 osal_bare_tick_inc()。
 */
#include "osal.h"

#include <string.h>

#include "stm32f4xx.h" /* CMSIS 设备头：提供 __FPU_PRESENT/IRQn_Type，并间接引入 core_cm4.h */

#ifndef EF_OSAL_BARE_MAX_MUTEX
#define EF_OSAL_BARE_MAX_MUTEX 8
#endif
#ifndef EF_OSAL_BARE_MAX_QUEUE
#define EF_OSAL_BARE_MAX_QUEUE 8
#endif
#ifndef EF_OSAL_BARE_MAX_THREAD
#define EF_OSAL_BARE_MAX_THREAD 8
#endif
#ifndef EF_OSAL_BARE_ARENA_SIZE
#define EF_OSAL_BARE_ARENA_SIZE 2048u
#endif

typedef struct {
    uint8_t used;
    uint8_t locked;
} osal_bare_mutex_t;

typedef struct {
    uint8_t used;
    uint8_t *storage;
    uint32_t item_size;
    uint32_t item_count;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} osal_bare_queue_t;

typedef struct {
    uint8_t used;
    osal_thread_entry_t entry;
    void *arg;
} osal_bare_thread_t;

static osal_bare_mutex_t s_mutex_pool[EF_OSAL_BARE_MAX_MUTEX];
static osal_bare_queue_t s_queue_pool[EF_OSAL_BARE_MAX_QUEUE];
static osal_bare_thread_t s_thread_pool[EF_OSAL_BARE_MAX_THREAD];

static uint8_t s_arena[EF_OSAL_BARE_ARENA_SIZE];
static uint32_t s_arena_offset = 0;

static volatile uint32_t s_tick_ms = 0;

/* board 层的 SysTick_Handler 每 1ms 调用一次 */
void osal_bare_tick_inc(void)
{
    s_tick_ms++;
}

void osal_enter_critical(void)
{
    __disable_irq();
}

void osal_exit_critical(void)
{
    __enable_irq();
}

uint32_t osal_tick_ms(void)
{
    return s_tick_ms;
}

void osal_delay_ms(uint32_t ms)
{
    uint32_t start = s_tick_ms;
    while ((uint32_t)(s_tick_ms - start) < ms) {
        /* 忙等，裸机下没有更好的选择 */
    }
}

/* ---------------- mutex ---------------- */

ef_err_t osal_mutex_create(osal_mutex_t *out_mutex)
{
    if (out_mutex == NULL) {
        return EF_ERR_PARAM;
    }

    osal_enter_critical();
    for (uint32_t i = 0; i < EF_OSAL_BARE_MAX_MUTEX; i++) {
        if (!s_mutex_pool[i].used) {
            s_mutex_pool[i].used = 1;
            s_mutex_pool[i].locked = 0;
            osal_exit_critical();
            *out_mutex = &s_mutex_pool[i];
            return EF_OK;
        }
    }
    osal_exit_critical();
    return EF_ERR_NOMEM;
}

ef_err_t osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
    osal_bare_mutex_t *m = (osal_bare_mutex_t *)mutex;
    if (m == NULL) {
        return EF_ERR_PARAM;
    }

    uint32_t start = s_tick_ms;
    for (;;) {
        osal_enter_critical();
        if (!m->locked) {
            m->locked = 1;
            osal_exit_critical();
            return EF_OK;
        }
        osal_exit_critical();

        if (timeout_ms == OSAL_NO_WAIT) {
            return EF_ERR_BUSY;
        }
        if (timeout_ms != OSAL_WAIT_FOREVER && (uint32_t)(s_tick_ms - start) >= timeout_ms) {
            return EF_ERR_TIMEOUT;
        }
    }
}

ef_err_t osal_mutex_unlock(osal_mutex_t mutex)
{
    osal_bare_mutex_t *m = (osal_bare_mutex_t *)mutex;
    if (m == NULL) {
        return EF_ERR_PARAM;
    }
    osal_enter_critical();
    m->locked = 0;
    osal_exit_critical();
    return EF_OK;
}

ef_err_t osal_mutex_delete(osal_mutex_t mutex)
{
    osal_bare_mutex_t *m = (osal_bare_mutex_t *)mutex;
    if (m == NULL) {
        return EF_ERR_PARAM;
    }
    osal_enter_critical();
    m->used = 0;
    m->locked = 0;
    osal_exit_critical();
    return EF_OK;
}

/* ---------------- queue ---------------- */

static uint8_t *arena_alloc(uint32_t size)
{
    osal_enter_critical();
    if (s_arena_offset + size > EF_OSAL_BARE_ARENA_SIZE) {
        osal_exit_critical();
        return NULL;
    }
    uint8_t *ptr = &s_arena[s_arena_offset];
    s_arena_offset += size;
    osal_exit_critical();
    return ptr;
}

ef_err_t osal_queue_create(osal_queue_t *out_queue, uint32_t item_count, uint32_t item_size)
{
    if (out_queue == NULL || item_count == 0 || item_size == 0) {
        return EF_ERR_PARAM;
    }

    uint8_t *storage = arena_alloc(item_count * item_size);
    if (storage == NULL) {
        return EF_ERR_NOMEM;
    }

    for (uint32_t i = 0; i < EF_OSAL_BARE_MAX_QUEUE; i++) {
        if (!s_queue_pool[i].used) {
            s_queue_pool[i].used = 1;
            s_queue_pool[i].storage = storage;
            s_queue_pool[i].item_size = item_size;
            s_queue_pool[i].item_count = item_count;
            s_queue_pool[i].head = 0;
            s_queue_pool[i].tail = 0;
            s_queue_pool[i].count = 0;
            *out_queue = &s_queue_pool[i];
            return EF_OK;
        }
    }
    return EF_ERR_NOMEM;
}

static ef_err_t queue_push(osal_bare_queue_t *q, const void *item)
{
    if (q->count >= q->item_count) {
        return EF_ERR_BUSY;
    }
    memcpy(&q->storage[q->head * q->item_size], item, q->item_size);
    q->head = (q->head + 1) % q->item_count;
    q->count++;
    return EF_OK;
}

ef_err_t osal_queue_send(osal_queue_t queue, const void *item, uint32_t timeout_ms)
{
    osal_bare_queue_t *q = (osal_bare_queue_t *)queue;
    if (q == NULL || item == NULL) {
        return EF_ERR_PARAM;
    }

    uint32_t start = s_tick_ms;
    for (;;) {
        osal_enter_critical();
        ef_err_t rc = queue_push(q, item);
        osal_exit_critical();
        if (rc == EF_OK) {
            return EF_OK;
        }
        if (timeout_ms == OSAL_NO_WAIT) {
            return EF_ERR_BUSY;
        }
        if (timeout_ms != OSAL_WAIT_FOREVER && (uint32_t)(s_tick_ms - start) >= timeout_ms) {
            return EF_ERR_TIMEOUT;
        }
    }
}

ef_err_t osal_queue_send_from_isr(osal_queue_t queue, const void *item)
{
    osal_bare_queue_t *q = (osal_bare_queue_t *)queue;
    if (q == NULL || item == NULL) {
        return EF_ERR_PARAM;
    }
    /* 中断上下文本身已经是不可重入的，且裸机没有嵌套 IRQ 优先级抢占本函数，
     * 直接操作即可，无需再次关中断（避免在 ISR 里嵌套禁用中断） */
    return queue_push(q, item);
}

ef_err_t osal_queue_receive(osal_queue_t queue, void *out_item, uint32_t timeout_ms)
{
    osal_bare_queue_t *q = (osal_bare_queue_t *)queue;
    if (q == NULL || out_item == NULL) {
        return EF_ERR_PARAM;
    }

    uint32_t start = s_tick_ms;
    for (;;) {
        osal_enter_critical();
        if (q->count > 0) {
            memcpy(out_item, &q->storage[q->tail * q->item_size], q->item_size);
            q->tail = (q->tail + 1) % q->item_count;
            q->count--;
            osal_exit_critical();
            return EF_OK;
        }
        osal_exit_critical();

        if (timeout_ms == OSAL_NO_WAIT) {
            return EF_ERR_TIMEOUT;
        }
        if (timeout_ms != OSAL_WAIT_FOREVER && (uint32_t)(s_tick_ms - start) >= timeout_ms) {
            return EF_ERR_TIMEOUT;
        }
        /* 裸机没有调度器可以让出 CPU，顺便驱动一下协作式线程轮转，
         * 避免"等队列"和"产生队列数据的线程"互相饿死 */
        osal_scheduler_run_once();
    }
}

ef_err_t osal_queue_delete(osal_queue_t queue)
{
    osal_bare_queue_t *q = (osal_bare_queue_t *)queue;
    if (q == NULL) {
        return EF_ERR_PARAM;
    }
    q->used = 0;
    return EF_OK;
}

/* ---------------- 协作式"线程" ---------------- */

ef_err_t osal_thread_create(osal_thread_t *out_thread, const char *name, osal_thread_entry_t entry,
                            void *arg, uint32_t stack_size_words, uint8_t priority)
{
    EF_UNUSED(name);
    EF_UNUSED(stack_size_words);
    EF_UNUSED(priority);

    if (out_thread == NULL || entry == NULL) {
        return EF_ERR_PARAM;
    }

    for (uint32_t i = 0; i < EF_OSAL_BARE_MAX_THREAD; i++) {
        if (!s_thread_pool[i].used) {
            s_thread_pool[i].used = 1;
            s_thread_pool[i].entry = entry;
            s_thread_pool[i].arg = arg;
            *out_thread = &s_thread_pool[i];
            return EF_OK;
        }
    }
    return EF_ERR_NOMEM;
}

ef_err_t osal_thread_delete(osal_thread_t thread)
{
    osal_bare_thread_t *t = (osal_bare_thread_t *)thread;
    if (t == NULL) {
        return EF_ERR_PARAM;
    }
    t->used = 0;
    return EF_OK;
}

void osal_scheduler_run_once(void)
{
    for (uint32_t i = 0; i < EF_OSAL_BARE_MAX_THREAD; i++) {
        if (s_thread_pool[i].used) {
            s_thread_pool[i].entry(s_thread_pool[i].arg);
        }
    }
}
