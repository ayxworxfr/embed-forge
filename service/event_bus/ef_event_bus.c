#include "ef_event_bus.h"

#include <string.h>

#include "ef_init.h"

#ifndef EF_EVENT_BUS_MAX_SUBSCRIBERS
#define EF_EVENT_BUS_MAX_SUBSCRIBERS 16
#endif
#ifndef EF_EVENT_BUS_TOPIC_MAX
#define EF_EVENT_BUS_TOPIC_MAX 24
#endif

typedef struct {
    char topic[EF_EVENT_BUS_TOPIC_MAX];
    ef_event_cb_t cb;
    void *ctx;
    uint8_t used;
} subscriber_t;

static subscriber_t s_subscribers[EF_EVENT_BUS_MAX_SUBSCRIBERS];

ef_err_t ef_event_bus_init(void)
{
    ef_event_bus_reset();
    return EF_OK;
}
EF_INIT_EXPORT(ef_event_bus_init, EF_INIT_LEVEL_SERVICE);

ef_err_t ef_event_bus_subscribe(const char *topic, ef_event_cb_t cb, void *ctx)
{
    if (topic == NULL || cb == NULL || strlen(topic) >= EF_EVENT_BUS_TOPIC_MAX) {
        return EF_ERR_PARAM;
    }

    for (uint32_t i = 0; i < EF_EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        if (!s_subscribers[i].used) {
            s_subscribers[i].used = 1;
            strncpy(s_subscribers[i].topic, topic, EF_EVENT_BUS_TOPIC_MAX - 1);
            s_subscribers[i].topic[EF_EVENT_BUS_TOPIC_MAX - 1] = '\0';
            s_subscribers[i].cb = cb;
            s_subscribers[i].ctx = ctx;
            return EF_OK;
        }
    }
    return EF_ERR_NOMEM;
}

ef_err_t ef_event_bus_unsubscribe(const char *topic, ef_event_cb_t cb, void *ctx)
{
    if (topic == NULL || cb == NULL) {
        return EF_ERR_PARAM;
    }
    for (uint32_t i = 0; i < EF_EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        if (s_subscribers[i].used && s_subscribers[i].cb == cb && s_subscribers[i].ctx == ctx &&
            strcmp(s_subscribers[i].topic, topic) == 0) {
            s_subscribers[i].used = 0;
            return EF_OK;
        }
    }
    return EF_ERR_NOTFOUND;
}

ef_err_t ef_event_bus_publish(const char *topic, const void *data, uint32_t len)
{
    if (topic == NULL) {
        return EF_ERR_PARAM;
    }
    for (uint32_t i = 0; i < EF_EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        if (s_subscribers[i].used && strcmp(s_subscribers[i].topic, topic) == 0) {
            s_subscribers[i].cb(topic, data, len, s_subscribers[i].ctx);
        }
    }
    return EF_OK;
}

void ef_event_bus_reset(void)
{
    memset(s_subscribers, 0, sizeof(s_subscribers));
}
