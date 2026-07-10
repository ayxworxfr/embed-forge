#include "ef_ringbuffer.h"

ef_err_t ef_rb_init(ef_ringbuffer_t *rb, uint8_t *buffer, uint32_t capacity)
{
    if (rb == NULL || buffer == NULL || capacity < 2) {
        return EF_ERR_PARAM;
    }

    rb->buffer = buffer;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;

    return EF_OK;
}

uint32_t ef_rb_used(const ef_ringbuffer_t *rb)
{
    if (rb == NULL) {
        return 0;
    }

    uint32_t head = rb->head;
    uint32_t tail = rb->tail;

    if (head >= tail) {
        return head - tail;
    }
    return rb->capacity - tail + head;
}

uint32_t ef_rb_free(const ef_ringbuffer_t *rb)
{
    if (rb == NULL) {
        return 0;
    }
    /* capacity - 1 是可用容量：head == tail 表示空，留一个槛区分满/空 */
    return (rb->capacity - 1) - ef_rb_used(rb);
}

uint32_t ef_rb_write(ef_ringbuffer_t *rb, const uint8_t *data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }

    uint32_t free_size = ef_rb_free(rb);
    uint32_t to_write = EF_MIN(len, free_size);
    uint32_t head = rb->head;

    for (uint32_t i = 0; i < to_write; i++) {
        rb->buffer[head] = data[i];
        head = (head + 1) % rb->capacity;
    }

    rb->head = head;
    return to_write;
}

uint32_t ef_rb_read(ef_ringbuffer_t *rb, uint8_t *data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }

    uint32_t used_size = ef_rb_used(rb);
    uint32_t to_read = EF_MIN(len, used_size);
    uint32_t tail = rb->tail;

    for (uint32_t i = 0; i < to_read; i++) {
        data[i] = rb->buffer[tail];
        tail = (tail + 1) % rb->capacity;
    }

    rb->tail = tail;
    return to_read;
}

void ef_rb_clear(ef_ringbuffer_t *rb)
{
    if (rb == NULL) {
        return;
    }
    rb->head = 0;
    rb->tail = 0;
}
