/*
 * test_app_led_blink.c — 验证 App 层业务逻辑，且业务代码本身零改动
 *
 * 关键点：链接的是 test/mocks/drv_led_mock.c 而不是真实的
 * driver/drv_led.c，app_led_blink.c 完全不知道也不关心这件事——这正是
 * "面向接口编程"带来的可测试性收益。
 */
#include "unity.h"

#include "app_led_blink.h"
#include "drv_led_mock.h"
#include "ef_event_bus.h"

void setUp(void)
{
    ef_event_bus_reset();
    drv_led_mock_reset();
    app_led_blink_reset();
    app_led_blink_init();
}

void tearDown(void)
{
}

static void test_init_subscribes_to_button_topic(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, app_led_blink_get_press_count());
    ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, NULL, 0);
    TEST_ASSERT_EQUAL_UINT32(1, app_led_blink_get_press_count());
}

static void test_step_toggles_led_each_call(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, app_led_blink_get_led_state());

    app_led_blink_step();
    TEST_ASSERT_EQUAL_UINT8(1, app_led_blink_get_led_state());
    TEST_ASSERT_EQUAL_UINT8(1, drv_led_mock_get_level());

    app_led_blink_step();
    TEST_ASSERT_EQUAL_UINT8(0, app_led_blink_get_led_state());
    TEST_ASSERT_EQUAL_UINT8(0, drv_led_mock_get_level());

    TEST_ASSERT_EQUAL_UINT32(2, drv_led_mock_get_toggle_count());
}

static void test_multiple_button_events_accumulate(void)
{
    ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, NULL, 0);
    ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, NULL, 0);
    ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, NULL, 0);

    TEST_ASSERT_EQUAL_UINT32(3, app_led_blink_get_press_count());
}

static void test_reset_clears_state(void)
{
    ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, NULL, 0);
    app_led_blink_step();

    app_led_blink_reset();

    TEST_ASSERT_EQUAL_UINT32(0, app_led_blink_get_press_count());
    TEST_ASSERT_EQUAL_UINT8(0, app_led_blink_get_led_state());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_subscribes_to_button_topic);
    RUN_TEST(test_step_toggles_led_each_call);
    RUN_TEST(test_multiple_button_events_accumulate);
    RUN_TEST(test_reset_clears_state);
    return UNITY_END();
}
