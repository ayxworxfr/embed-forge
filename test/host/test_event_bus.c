#include "unity.h"

#include <string.h>

#include "ef_event_bus.h"

static int s_calls_a;
static int s_calls_b;
static char s_last_data[32];
static void *s_last_ctx;

static void sub_a(const char *topic, const void *data, uint32_t len, void *ctx)
{
    EF_UNUSED(topic);
    s_calls_a++;
    s_last_ctx = ctx;
    if (data != NULL && len < sizeof(s_last_data)) {
        memcpy(s_last_data, data, len);
        s_last_data[len] = '\0';
    }
}

static void sub_b(const char *topic, const void *data, uint32_t len, void *ctx)
{
    EF_UNUSED(topic);
    EF_UNUSED(data);
    EF_UNUSED(len);
    EF_UNUSED(ctx);
    s_calls_b++;
}

void setUp(void)
{
    ef_event_bus_reset();
    s_calls_a = 0;
    s_calls_b = 0;
    s_last_data[0] = '\0';
    s_last_ctx = NULL;
}

void tearDown(void)
{
}

static void test_publish_with_no_subscriber_is_ok(void)
{
    TEST_ASSERT_EQUAL_INT(EF_OK, ef_event_bus_publish("nobody_listens", NULL, 0));
}

static void test_single_subscriber_receives_event(void)
{
    ef_event_bus_subscribe("btn_pressed", sub_a, (void *)0x1234);
    ef_event_bus_publish("btn_pressed", "px", 2);

    TEST_ASSERT_EQUAL_INT(1, s_calls_a);
    TEST_ASSERT_EQUAL_STRING("px", s_last_data);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, s_last_ctx);
}

static void test_multiple_subscribers_all_notified(void)
{
    ef_event_bus_subscribe("topic", sub_a, NULL);
    ef_event_bus_subscribe("topic", sub_b, NULL);
    ef_event_bus_publish("topic", NULL, 0);

    TEST_ASSERT_EQUAL_INT(1, s_calls_a);
    TEST_ASSERT_EQUAL_INT(1, s_calls_b);
}

static void test_only_matching_topic_is_notified(void)
{
    ef_event_bus_subscribe("topic_a", sub_a, NULL);
    ef_event_bus_subscribe("topic_b", sub_b, NULL);
    ef_event_bus_publish("topic_a", NULL, 0);

    TEST_ASSERT_EQUAL_INT(1, s_calls_a);
    TEST_ASSERT_EQUAL_INT(0, s_calls_b);
}

static void test_unsubscribe_stops_notifications(void)
{
    ef_event_bus_subscribe("topic", sub_a, NULL);
    ef_event_bus_unsubscribe("topic", sub_a, NULL);
    ef_event_bus_publish("topic", NULL, 0);

    TEST_ASSERT_EQUAL_INT(0, s_calls_a);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_publish_with_no_subscriber_is_ok);
    RUN_TEST(test_single_subscriber_receives_event);
    RUN_TEST(test_multiple_subscribers_all_notified);
    RUN_TEST(test_only_matching_topic_is_notified);
    RUN_TEST(test_unsubscribe_stops_notifications);
    return UNITY_END();
}
