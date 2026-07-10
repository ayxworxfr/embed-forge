#include "unity.h"

#include "ef_ringbuffer.h"

static ef_ringbuffer_t rb;
static uint8_t storage[8];

void setUp(void)
{
    ef_rb_init(&rb, storage, sizeof(storage));
}

void tearDown(void)
{
}

static void test_init_rejects_bad_params(void)
{
    TEST_ASSERT_EQUAL_INT(EF_ERR_PARAM, ef_rb_init(NULL, storage, sizeof(storage)));
    TEST_ASSERT_EQUAL_INT(EF_ERR_PARAM, ef_rb_init(&rb, NULL, sizeof(storage)));
    TEST_ASSERT_EQUAL_INT(EF_ERR_PARAM, ef_rb_init(&rb, storage, 1));
}

static void test_empty_buffer(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, ef_rb_used(&rb));
    TEST_ASSERT_EQUAL_UINT32(7, ef_rb_free(&rb)); /* capacity(8) - 1 */
}

static void test_write_then_read_roundtrip(void)
{
    const uint8_t in[] = {1, 2, 3, 4};
    uint8_t out[4] = {0};

    TEST_ASSERT_EQUAL_UINT32(4, ef_rb_write(&rb, in, 4));
    TEST_ASSERT_EQUAL_UINT32(4, ef_rb_used(&rb));

    TEST_ASSERT_EQUAL_UINT32(4, ef_rb_read(&rb, out, 4));
    TEST_ASSERT_EQUAL_MEMORY(in, out, 4);
    TEST_ASSERT_EQUAL_UINT32(0, ef_rb_used(&rb));
}

static void test_write_truncates_when_full(void)
{
    const uint8_t in[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint32_t written = ef_rb_write(&rb, in, sizeof(in));
    TEST_ASSERT_EQUAL_UINT32(7, written); /* 容量 8，可用 7 */
    TEST_ASSERT_EQUAL_UINT32(0, ef_rb_free(&rb));
}

static void test_wraparound(void)
{
    uint8_t out[8];

    /* 先写 6 个再读 6 个，把 head/tail 都推过缓冲区尾部，验证环绕 */
    uint8_t chunk[6] = {10, 11, 12, 13, 14, 15};
    ef_rb_write(&rb, chunk, 6);
    ef_rb_read(&rb, out, 6);

    uint8_t in2[6] = {20, 21, 22, 23, 24, 25};
    TEST_ASSERT_EQUAL_UINT32(6, ef_rb_write(&rb, in2, 6));
    TEST_ASSERT_EQUAL_UINT32(6, ef_rb_read(&rb, out, 6));
    TEST_ASSERT_EQUAL_MEMORY(in2, out, 6);
}

static void test_clear(void)
{
    uint8_t in[3] = {1, 2, 3};
    ef_rb_write(&rb, in, 3);
    ef_rb_clear(&rb);
    TEST_ASSERT_EQUAL_UINT32(0, ef_rb_used(&rb));
    TEST_ASSERT_EQUAL_UINT32(7, ef_rb_free(&rb));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_bad_params);
    RUN_TEST(test_empty_buffer);
    RUN_TEST(test_write_then_read_roundtrip);
    RUN_TEST(test_write_truncates_when_full);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_clear);
    return UNITY_END();
}
