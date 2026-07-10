#include "unity.h"

#include "ef_config.h"

void setUp(void)
{
    ef_config_reset();
}

void tearDown(void)
{
}

static void test_i32_roundtrip(void)
{
    TEST_ASSERT_EQUAL_INT(EF_OK, ef_config_set_i32("baud", 115200));
    TEST_ASSERT_EQUAL_INT32(115200, ef_config_get_i32("baud", -1));
}

static void test_i32_missing_key_returns_default(void)
{
    TEST_ASSERT_EQUAL_INT32(-1, ef_config_get_i32("no_such_key", -1));
}

static void test_str_roundtrip(void)
{
    char out[16] = {0};
    TEST_ASSERT_EQUAL_INT(EF_OK, ef_config_set_str("name", "embed-forge"));
    TEST_ASSERT_EQUAL_INT(EF_OK, ef_config_get_str("name", out, sizeof(out), "fallback"));
    TEST_ASSERT_EQUAL_STRING("embed-forge", out);
}

static void test_str_missing_key_returns_default_and_notfound(void)
{
    char out[16] = {0};
    ef_err_t rc = ef_config_get_str("missing", out, sizeof(out), "fallback");
    TEST_ASSERT_EQUAL_INT(EF_ERR_NOTFOUND, rc);
    TEST_ASSERT_EQUAL_STRING("fallback", out);
}

static void test_set_overwrites_existing_key(void)
{
    ef_config_set_i32("k", 1);
    ef_config_set_i32("k", 2);
    TEST_ASSERT_EQUAL_INT32(2, ef_config_get_i32("k", -1));
}

static void test_key_too_long_rejected(void)
{
    ef_err_t rc = ef_config_set_i32("this_key_name_is_definitely_longer_than_24_chars", 1);
    TEST_ASSERT_EQUAL_INT(EF_ERR_PARAM, rc);
}

static void test_reset_clears_all_entries(void)
{
    ef_config_set_i32("k", 42);
    ef_config_reset();
    TEST_ASSERT_EQUAL_INT32(-1, ef_config_get_i32("k", -1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_i32_roundtrip);
    RUN_TEST(test_i32_missing_key_returns_default);
    RUN_TEST(test_str_roundtrip);
    RUN_TEST(test_str_missing_key_returns_default_and_notfound);
    RUN_TEST(test_set_overwrites_existing_key);
    RUN_TEST(test_key_too_long_rejected);
    RUN_TEST(test_reset_clears_all_entries);
    return UNITY_END();
}
