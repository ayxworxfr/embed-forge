#include "unity.h"

#include <string.h>

#include "ef_device.h"

static ef_device_t s_dev_a;
static ef_device_t s_dev_b;
static int s_init_calls = 0;
static char s_write_buf[32];

static ef_err_t fake_init(ef_device_t *dev)
{
    (void)dev;
    s_init_calls++;
    return EF_OK;
}

static int32_t fake_write(ef_device_t *dev, const void *buf, uint32_t len)
{
    (void)dev;
    uint32_t n = len < sizeof(s_write_buf) ? len : sizeof(s_write_buf) - 1;
    memcpy(s_write_buf, buf, n);
    s_write_buf[n] = '\0';
    return (int32_t)len;
}

static const ef_device_ops_t s_ops_with_init = {
    .init = fake_init,
    .write = fake_write,
};

static const ef_device_ops_t s_ops_no_ops = {0};

void setUp(void)
{
    ef_device_registry_reset();
    s_init_calls = 0;
    memset(s_write_buf, 0, sizeof(s_write_buf));
}

void tearDown(void)
{
}

static void test_register_calls_init_once(void)
{
    ef_err_t rc = ef_device_register(&s_dev_a, "dev_a", &s_ops_with_init, NULL);
    TEST_ASSERT_EQUAL_INT(EF_OK, rc);
    TEST_ASSERT_EQUAL_INT(1, s_init_calls);
}

static void test_find_returns_registered_device(void)
{
    ef_device_register(&s_dev_a, "dev_a", &s_ops_with_init, NULL);
    ef_device_t *found = ef_device_find("dev_a");
    TEST_ASSERT_EQUAL_PTR(&s_dev_a, found);
    TEST_ASSERT_NULL(ef_device_find("does_not_exist"));
}

static void test_duplicate_name_rejected(void)
{
    ef_device_register(&s_dev_a, "dup", &s_ops_with_init, NULL);
    ef_err_t rc = ef_device_register(&s_dev_b, "dup", &s_ops_with_init, NULL);
    TEST_ASSERT_EQUAL_INT(EF_ERR_BUSY, rc);
}

static void test_write_dispatches_to_ops(void)
{
    ef_device_register(&s_dev_a, "dev_a", &s_ops_with_init, NULL);
    ef_device_t *dev = ef_device_find("dev_a");

    int32_t n = ef_device_write(dev, "hello", 5);
    TEST_ASSERT_EQUAL_INT32(5, n);
    TEST_ASSERT_EQUAL_STRING("hello", s_write_buf);
}

static void test_read_on_unsupported_ops_returns_error(void)
{
    ef_device_register(&s_dev_a, "no_ops", &s_ops_no_ops, NULL);
    ef_device_t *dev = ef_device_find("no_ops");

    uint8_t buf[4];
    int32_t n = ef_device_read(dev, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT32((int32_t)EF_ERR_NOTSUPPORTED, n);
}

static void test_name_too_long_rejected(void)
{
    ef_err_t rc = ef_device_register(&s_dev_a, "this_name_is_way_too_long_for_the_table",
                                     &s_ops_with_init, NULL);
    TEST_ASSERT_EQUAL_INT(EF_ERR_PARAM, rc);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_register_calls_init_once);
    RUN_TEST(test_find_returns_registered_device);
    RUN_TEST(test_duplicate_name_rejected);
    RUN_TEST(test_write_dispatches_to_ops);
    RUN_TEST(test_read_on_unsupported_ops_returns_error);
    RUN_TEST(test_name_too_long_rejected);
    return UNITY_END();
}
