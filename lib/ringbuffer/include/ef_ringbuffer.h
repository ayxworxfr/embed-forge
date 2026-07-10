/*
 * ef_ringbuffer.h — 环形缓冲区（参考 lwrb 思路的极简实现）
 *
 * 定位：lib 层，不依赖硬件/OS，可在裸机中断、RTOS 任务、PC 单测里直接复用。
 * 典型场景：中断里 ef_rb_write()（生产者），任务里 ef_rb_read()（消费者）。
 *
 * 线程安全说明：本实现只保证"单生产者 + 单消费者"场景下的安全（典型的
 * UART RX 中断 -> 任务读取），因为 head 只由生产者写、tail 只由消费者写。
 * 多生产者/多消费者场景需要在上层加锁（OSAL mutex）。
 */
#ifndef EF_RINGBUFFER_H
#define EF_RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

typedef struct {
    uint8_t *buffer;
    uint32_t capacity;      /* buffer 长度，可用容量为 capacity - 1 */
    volatile uint32_t head; /* 生产者写入位置 */
    volatile uint32_t tail; /* 消费者读取位置 */
} ef_ringbuffer_t;

/**
 * 初始化环形缓冲区。
 * @param rb        句柄
 * @param buffer    外部提供的存储区（禁止内部 malloc，零动态分配原则）
 * @param capacity  buffer 长度（字节），可用容量为 capacity - 1
 */
ef_err_t ef_rb_init(ef_ringbuffer_t *rb, uint8_t *buffer, uint32_t capacity);

/** 写入数据，返回实际写入字节数（缓冲区满时截断） */
uint32_t ef_rb_write(ef_ringbuffer_t *rb, const uint8_t *data, uint32_t len);

/** 读取数据，返回实际读取字节数（数据不足时截断） */
uint32_t ef_rb_read(ef_ringbuffer_t *rb, uint8_t *data, uint32_t len);

/** 当前已使用字节数 */
uint32_t ef_rb_used(const ef_ringbuffer_t *rb);

/** 当前剩余可写字节数 */
uint32_t ef_rb_free(const ef_ringbuffer_t *rb);

/** 清空缓冲区 */
void ef_rb_clear(ef_ringbuffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* EF_RINGBUFFER_H */
