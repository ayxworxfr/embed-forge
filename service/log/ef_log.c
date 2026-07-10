#include "ef_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ef_init.h"

#ifndef EF_LOG_LINE_MAX
#define EF_LOG_LINE_MAX 160
#endif

static ef_log_sink_t s_sink = NULL;
static ef_log_level_t s_level = EF_LOG_LEVEL_INFO;

static const char *level_tag(ef_log_level_t level)
{
    switch (level) {
        case EF_LOG_LEVEL_DEBUG:
            return "D";
        case EF_LOG_LEVEL_INFO:
            return "I";
        case EF_LOG_LEVEL_WARN:
            return "W";
        case EF_LOG_LEVEL_ERROR:
            return "E";
        default:
            return "?";
    }
}

ef_err_t ef_log_init(void)
{
    s_sink = NULL;
    s_level = EF_LOG_LEVEL_INFO;
    return EF_OK;
}
EF_INIT_EXPORT(ef_log_init, EF_INIT_LEVEL_SERVICE);

void ef_log_set_sink(ef_log_sink_t sink)
{
    s_sink = sink;
}

void ef_log_set_level(ef_log_level_t level)
{
    s_level = level;
}

void ef_log_write(ef_log_level_t level, const char *tag, const char *fmt, ...)
{
    if (level < s_level || s_sink == NULL) {
        return;
    }
    if (tag == NULL) {
        tag = "-";
    }

    /* 已知限制：静态缓冲区不可重入。多任务并发打日志需要在调用方加
     * osal_mutex，本组件本身不内置锁，避免在裸机单任务场景强加锁开销。 */
    static char line[EF_LOG_LINE_MAX];
    int prefix_len = snprintf(line, sizeof(line), "[%s][%s] ", level_tag(level), tag);
    if (prefix_len < 0) {
        prefix_len = 0;
    }
    if ((uint32_t)prefix_len >= sizeof(line)) {
        prefix_len = (int)sizeof(line) - 1;
    }

    va_list args;
    va_start(args, fmt);
    int body_len = vsnprintf(&line[prefix_len], sizeof(line) - (uint32_t)prefix_len, fmt, args);
    va_end(args);

    uint32_t total_len = (uint32_t)prefix_len;
    if (body_len > 0) {
        total_len += (uint32_t)body_len;
        if (total_len > sizeof(line) - 3) {
            total_len = sizeof(line) - 3; /* 给 "\r\n\0" 留位置 */
        }
    }

    line[total_len++] = '\r';
    line[total_len++] = '\n';
    line[total_len] = '\0';

    s_sink(line, total_len);
}
