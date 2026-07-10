/*
 * osal_host.c — PC（host）后端实现
 *
 * 用途：让 Service/App 层代码可以在 x86 上用 pthread 编译运行，配合
 * test/mocks 里的 HAL mock，实现"业务逻辑完全脱离真实硬件的单元测试"。
 * 只在 native（非交叉编译）构建下被编译进 test 目标。
 */
#include "osal.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

typedef struct {
    uint8_t *storage;
    uint32_t item_size;
    uint32_t item_count;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} osal_host_queue_t;

typedef struct {
    osal_thread_entry_t entry;
    void *arg;
    pthread_t tid;
} osal_host_thread_t;

static pthread_mutex_t s_critical_lock = PTHREAD_MUTEX_INITIALIZER;

/* ---------------- mutex ---------------- */

ef_err_t osal_mutex_create(osal_mutex_t *out_mutex)
{
    if (out_mutex == NULL) {
        return EF_ERR_PARAM;
    }
    pthread_mutex_t *m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (m == NULL) {
        return EF_ERR_NOMEM;
    }
    pthread_mutex_init(m, NULL);
    *out_mutex = (osal_mutex_t)m;
    return EF_OK;
}

ef_err_t osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
    EF_UNUSED(timeout_ms); /* host 后端简化处理：直接阻塞等待，测试场景不依赖超时语义 */
    if (mutex == NULL) {
        return EF_ERR_PARAM;
    }
    return (pthread_mutex_lock((pthread_mutex_t *)mutex) == 0) ? EF_OK : EF_ERR;
}

ef_err_t osal_mutex_unlock(osal_mutex_t mutex)
{
    if (mutex == NULL) {
        return EF_ERR_PARAM;
    }
    return (pthread_mutex_unlock((pthread_mutex_t *)mutex) == 0) ? EF_OK : EF_ERR;
}

ef_err_t osal_mutex_delete(osal_mutex_t mutex)
{
    if (mutex == NULL) {
        return EF_ERR_PARAM;
    }
    pthread_mutex_destroy((pthread_mutex_t *)mutex);
    free(mutex);
    return EF_OK;
}

/* ---------------- queue ---------------- */

ef_err_t osal_queue_create(osal_queue_t *out_queue, uint32_t item_count, uint32_t item_size)
{
    if (out_queue == NULL || item_count == 0 || item_size == 0) {
        return EF_ERR_PARAM;
    }

    osal_host_queue_t *q = (osal_host_queue_t *)malloc(sizeof(osal_host_queue_t));
    if (q == NULL) {
        return EF_ERR_NOMEM;
    }
    q->storage = (uint8_t *)malloc((size_t)item_count * item_size);
    if (q->storage == NULL) {
        free(q);
        return EF_ERR_NOMEM;
    }
    q->item_size = item_size;
    q->item_count = item_count;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);

    *out_queue = (osal_queue_t)q;
    return EF_OK;
}

ef_err_t osal_queue_send(osal_queue_t queue, const void *item, uint32_t timeout_ms)
{
    EF_UNUSED(timeout_ms);
    osal_host_queue_t *q = (osal_host_queue_t *)queue;
    if (q == NULL || item == NULL) {
        return EF_ERR_PARAM;
    }

    pthread_mutex_lock(&q->lock);
    while (q->count >= q->item_count) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    memcpy(&q->storage[q->head * q->item_size], item, q->item_size);
    q->head = (q->head + 1) % q->item_count;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return EF_OK;
}

ef_err_t osal_queue_send_from_isr(osal_queue_t queue, const void *item)
{
    /* host 没有真实中断上下文，直接复用 osal_queue_send 的非阻塞语义 */
    osal_host_queue_t *q = (osal_host_queue_t *)queue;
    if (q == NULL || item == NULL) {
        return EF_ERR_PARAM;
    }
    pthread_mutex_lock(&q->lock);
    if (q->count >= q->item_count) {
        pthread_mutex_unlock(&q->lock);
        return EF_ERR_BUSY;
    }
    memcpy(&q->storage[q->head * q->item_size], item, q->item_size);
    q->head = (q->head + 1) % q->item_count;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return EF_OK;
}

ef_err_t osal_queue_receive(osal_queue_t queue, void *out_item, uint32_t timeout_ms)
{
    EF_UNUSED(timeout_ms);
    osal_host_queue_t *q = (osal_host_queue_t *)queue;
    if (q == NULL || out_item == NULL) {
        return EF_ERR_PARAM;
    }

    pthread_mutex_lock(&q->lock);
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    memcpy(out_item, &q->storage[q->tail * q->item_size], q->item_size);
    q->tail = (q->tail + 1) % q->item_count;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return EF_OK;
}

ef_err_t osal_queue_delete(osal_queue_t queue)
{
    osal_host_queue_t *q = (osal_host_queue_t *)queue;
    if (q == NULL) {
        return EF_ERR_PARAM;
    }
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q->storage);
    free(q);
    return EF_OK;
}

/* ---------------- 线程 ---------------- */

static void *thread_trampoline(void *arg)
{
    osal_host_thread_t *t = (osal_host_thread_t *)arg;
    t->entry(t->arg);
    return NULL;
}

ef_err_t osal_thread_create(osal_thread_t *out_thread, const char *name, osal_thread_entry_t entry,
                            void *arg, uint32_t stack_size_words, uint8_t priority)
{
    EF_UNUSED(name);
    EF_UNUSED(stack_size_words);
    EF_UNUSED(priority);

    if (out_thread == NULL || entry == NULL) {
        return EF_ERR_PARAM;
    }

    osal_host_thread_t *t = (osal_host_thread_t *)malloc(sizeof(osal_host_thread_t));
    if (t == NULL) {
        return EF_ERR_NOMEM;
    }
    t->entry = entry;
    t->arg = arg;

    if (pthread_create(&t->tid, NULL, thread_trampoline, t) != 0) {
        free(t);
        return EF_ERR;
    }

    *out_thread = (osal_thread_t)t;
    return EF_OK;
}

ef_err_t osal_thread_delete(osal_thread_t thread)
{
    osal_host_thread_t *t = (osal_host_thread_t *)thread;
    if (t == NULL) {
        return EF_ERR_PARAM;
    }
    pthread_cancel(t->tid);
    free(t);
    return EF_OK;
}

/* ---------------- 时间 ---------------- */

void osal_delay_ms(uint32_t ms)
{
#if defined(_WIN32)
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

uint32_t osal_tick_ms(void)
{
#if defined(_WIN32)
    return (uint32_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
#endif
}

/* ---------------- 临界区 ---------------- */

void osal_enter_critical(void)
{
    pthread_mutex_lock(&s_critical_lock);
}

void osal_exit_critical(void)
{
    pthread_mutex_unlock(&s_critical_lock);
}

void osal_scheduler_run_once(void)
{
    /* host 下线程由 pthread 内核态调度器抢占式驱动，无需额外操作 */
}
