#include "unity.h"

#include <string.h>

#include "ef_log.h"

static char s_last_line[256];
static uint32_t s_sink_calls;

static void capture_sink(const char *line, uint32_t len)
{
    uint32_t n = len < sizeof(s_last_line) ? len : sizeof(s_last_line) - 1;
    memcpy(s_last_line, line, n);
    s_last_line[n] = '\0';
    s_sink_calls++;
}

void setUp(void)
{
    ef_log_init();
    ef_log_set_sink(capture_sink);
    s_last_line[0] = '\0';
    s_sink_calls = 0;
}

void tearDown(void)
{
}

static void test_no_sink_does_not_crash(void)
{
    ef_log_set_sink(NULL);
    EF_LOG_I("t", "hello %d", 1); /* 仅要求不崩溃 */
    TEST_PASS();
}

static void test_info_message_reaches_sink(void)
{
    EF_LOG_I("app", "boot ok");
    TEST_ASSERT_EQUAL_UINT32(1, s_sink_calls);
    TEST_ASSERT_NOT_NULL(strstr(s_last_line, "[I][app] boot ok"));
}

static void test_default_level_filters_debug(void)
{
    /* 默认 level = INFO，DEBUG 应该被过滤掉 */
    EF_LOG_D("app", "should be filtered");
    TEST_ASSERT_EQUAL_UINT32(0, s_sink_calls);
}

static void test_set_level_filters_lower_levels(void)
{
    ef_log_set_level(EF_LOG_LEVEL_ERROR);
    EF_LOG_W("app", "warn should be filtered");
    TEST_ASSERT_EQUAL_UINT32(0, s_sink_calls);

    EF_LOG_E("app", "error passes");
    TEST_ASSERT_EQUAL_UINT32(1, s_sink_calls);
}

static void test_format_arguments_are_substituted(void)
{
    EF_LOG_I("app", "count=%d name=%s", 42, "led0");
    TEST_ASSERT_NOT_NULL(strstr(s_last_line, "count=42 name=led0"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_no_sink_does_not_crash);
    RUN_TEST(test_info_message_reaches_sink);
    RUN_TEST(test_default_level_filters_debug);
    RUN_TEST(test_set_level_filters_lower_levels);
    RUN_TEST(test_format_arguments_are_substituted);
    return UNITY_END();
}
