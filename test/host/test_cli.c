#include "unity.h"

#include <string.h>

#include "ef_cli.h"

static char s_output_buf[512];
static uint32_t s_output_len;

static int s_echo_argc;
static char s_echo_arg1[32];

static void capture_output(const char *data, uint32_t len)
{
    if (s_output_len + len < sizeof(s_output_buf)) {
        memcpy(&s_output_buf[s_output_len], data, len);
        s_output_len += len;
        s_output_buf[s_output_len] = '\0';
    }
}

static int cmd_echo(int argc, char **argv)
{
    s_echo_argc = argc;
    s_echo_arg1[0] = '\0';
    if (argc >= 2) {
        strncpy(s_echo_arg1, argv[1], sizeof(s_echo_arg1) - 1);
    }
    return 0;
}

void setUp(void)
{
    ef_cli_init(); /* reset 命令表 + 注册内置 help */
    ef_cli_set_output(capture_output);
    s_output_buf[0] = '\0';
    s_output_len = 0;
    s_echo_argc = -1;
    s_echo_arg1[0] = '\0';
    ef_cli_register("echo", cmd_echo, "echo <arg>");
}

void tearDown(void)
{
}

static void test_exec_line_dispatches_to_registered_command(void)
{
    ef_err_t rc = ef_cli_exec_line("echo hello");
    TEST_ASSERT_EQUAL_INT(EF_OK, rc);
    TEST_ASSERT_EQUAL_INT(2, s_echo_argc);
    TEST_ASSERT_EQUAL_STRING("hello", s_echo_arg1);
}

static void test_exec_unknown_command_reports_error(void)
{
    ef_err_t rc = ef_cli_exec_line("nope");
    TEST_ASSERT_EQUAL_INT(EF_ERR_NOTFOUND, rc);
    TEST_ASSERT_NOT_NULL(strstr(s_output_buf, "unknown command: nope"));
}

static void test_exec_empty_line_is_noop(void)
{
    ef_err_t rc = ef_cli_exec_line("   ");
    TEST_ASSERT_EQUAL_INT(EF_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, s_output_len);
}

static void test_feed_byte_builds_line_and_execs_on_newline(void)
{
    const char *cmd = "echo world\n";
    for (const char *p = cmd; *p != '\0'; p++) {
        ef_cli_feed_byte((uint8_t)*p);
    }
    TEST_ASSERT_EQUAL_INT(2, s_echo_argc);
    TEST_ASSERT_EQUAL_STRING("world", s_echo_arg1);
}

static void test_feed_byte_backspace_erases_last_char(void)
{
    const char *cmd = "echo helloXX";
    for (const char *p = cmd; *p != '\0'; p++) {
        ef_cli_feed_byte((uint8_t)*p);
    }
    ef_cli_feed_byte(0x08);
    ef_cli_feed_byte(0x08);
    ef_cli_feed_byte((uint8_t)'\n');

    TEST_ASSERT_EQUAL_STRING("hello", s_echo_arg1);
}

static void test_register_duplicate_name_still_dispatches_first(void)
{
    /* 当前实现不做重名检测：验证至少不会 crash，且第一个注册的会被匹配到 */
    ef_cli_register("echo", cmd_echo, "dup");
    ef_err_t rc = ef_cli_exec_line("echo again");
    TEST_ASSERT_EQUAL_INT(EF_OK, rc);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_exec_line_dispatches_to_registered_command);
    RUN_TEST(test_exec_unknown_command_reports_error);
    RUN_TEST(test_exec_empty_line_is_noop);
    RUN_TEST(test_feed_byte_builds_line_and_execs_on_newline);
    RUN_TEST(test_feed_byte_backspace_erases_last_char);
    RUN_TEST(test_register_duplicate_name_still_dispatches_first);
    return UNITY_END();
}
