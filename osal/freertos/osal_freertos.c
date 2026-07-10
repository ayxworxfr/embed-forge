/*
 * osal_freertos.c — FreeRTOS 后端实现
 *
 * 直接把 osal.h 的接口映射到 FreeRTOS API。business 代码永远不直接
 * include FreeRTOS.h/task.h/queue.h/semphr.h。
 */
#include "osal.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

static inline TickType_t ms_to_ticks(uint32_t timeout_ms)
{
    if (timeout_ms == OSAL_WAIT_FOREVER) {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS(timeout_ms);
}

/* ---------------- mutex ---------------- */

ef_err_t osal_mutex_create(osal_mutex_t *out_mutex)
{
    if (out_mutex == NULL) {
        return EF_ERR_PARAM;
    }
    SemaphoreHandle_t handle = xSemaphoreCreateMutex();
    if (handle == NULL) {
        return EF_ERR_NOMEM;
    }
    *out_mutex = (osal_mutex_t)handle;
    return EF_OK;
}

ef_err_t osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
    if (mutex == NULL) {
        return EF_ERR_PARAM;
    }
    return (xSemaphoreTake((SemaphoreHandle_t)mutex, ms_to_ticks(timeout_ms)) == pdTRUE)
               ? EF_OK
               : EF_ERR_TIMEOUT;
}

ef_err_t osal_mutex_unlock(osal_mutex_t mutex)
{
    if (mutex == NULL) {
        return EF_ERR_PARAM;
    }
    return (xSemaphoreGive((SemaphoreHandle_t)mutex) == pdTRUE) ? EF_OK : EF_ERR;
}

ef_err_t osal_mutex_delete(osal_mutex_t mutex)
{
    if (mutex == NULL) {
        return EF_ERR_PARAM;
    }
    vSemaphoreDelete((SemaphoreHandle_t)mutex);
    return EF_OK;
}

/* ---------------- queue ---------------- */

ef_err_t osal_queue_create(osal_queue_t *out_queue, uint32_t item_count, uint32_t item_size)
{
    if (out_queue == NULL || item_count == 0 || item_size == 0) {
        return EF_ERR_PARAM;
    }
    QueueHandle_t handle = xQueueCreate(item_count, item_size);
    if (handle == NULL) {
        return EF_ERR_NOMEM;
    }
    *out_queue = (osal_queue_t)handle;
    return EF_OK;
}

ef_err_t osal_queue_send(osal_queue_t queue, const void *item, uint32_t timeout_ms)
{
    if (queue == NULL || item == NULL) {
        return EF_ERR_PARAM;
    }
    return (xQueueSend((QueueHandle_t)queue, item, ms_to_ticks(timeout_ms)) == pdTRUE)
               ? EF_OK
               : EF_ERR_TIMEOUT;
}

ef_err_t osal_queue_send_from_isr(osal_queue_t queue, const void *item)
{
    if (queue == NULL || item == NULL) {
        return EF_ERR_PARAM;
    }
    BaseType_t higher_priority_task_woken = pdFALSE;
    BaseType_t ok = xQueueSendFromISR((QueueHandle_t)queue, item, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return (ok == pdTRUE) ? EF_OK : EF_ERR_BUSY;
}

ef_err_t osal_queue_receive(osal_queue_t queue, void *out_item, uint32_t timeout_ms)
{
    if (queue == NULL || out_item == NULL) {
        return EF_ERR_PARAM;
    }
    return (xQueueReceive((QueueHandle_t)queue, out_item, ms_to_ticks(timeout_ms)) == pdTRUE)
               ? EF_OK
               : EF_ERR_TIMEOUT;
}

ef_err_t osal_queue_delete(osal_queue_t queue)
{
    if (queue == NULL) {
        return EF_ERR_PARAM;
    }
    vQueueDelete((QueueHandle_t)queue);
    return EF_OK;
}

/* ---------------- 线程/任务 ---------------- */

ef_err_t osal_thread_create(osal_thread_t *out_thread, const char *name, osal_thread_entry_t entry,
                            void *arg, uint32_t stack_size_words, uint8_t priority)
{
    if (out_thread == NULL || entry == NULL) {
        return EF_ERR_PARAM;
    }
    TaskHandle_t handle = NULL;
    /* FreeRTOS 任务入口签名是 void(*)(void*)，与 osal_thread_entry_t 一致，可直接传递 */
    BaseType_t ok = xTaskCreate((TaskFunction_t)entry, name, (uint16_t)stack_size_words, arg,
                                (UBaseType_t)priority, &handle);
    if (ok != pdPASS || handle == NULL) {
        return EF_ERR_NOMEM;
    }
    *out_thread = (osal_thread_t)handle;
    return EF_OK;
}

ef_err_t osal_thread_delete(osal_thread_t thread)
{
    if (thread == NULL) {
        return EF_ERR_PARAM;
    }
    vTaskDelete((TaskHandle_t)thread);
    return EF_OK;
}

/* ---------------- 时间 ---------------- */

void osal_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

uint32_t osal_tick_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * (1000u / configTICK_RATE_HZ));
}

/* ---------------- 临界区 ---------------- */

void osal_enter_critical(void)
{
    taskENTER_CRITICAL();
}

void osal_exit_critical(void)
{
    taskEXIT_CRITICAL();
}

void osal_scheduler_run_once(void)
{
    /* FreeRTOS 下调度完全由内核抢占式驱动，这里只需要启动一次调度器；
     * vTaskStartScheduler() 正常情况下永不返回。 */
    vTaskStartScheduler();
}
