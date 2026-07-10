/*
 * ef_log.h — 分级日志组件
 *
 * 设计要点：不直接依赖 ef_device/hal，只依赖一个"输出 sink"函数指针
 * （ef_log_set_sink）。真机固件里由 app 层把 sink 接到
 * drv_uart_console_write；PC 单测里把 sink 接到一个把内容存进内存
 * buffer 的假实现——同一份 log 逻辑代码，两边零改动，这正是
 * Service 层"可独立编译、独立测试"的意义。
 */
#ifndef EF_LOG_H
#define EF_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

typedef enum {
    EF_LOG_LEVEL_DEBUG = 0,
    EF_LOG_LEVEL_INFO,
    EF_LOG_LEVEL_WARN,
    EF_LOG_LEVEL_ERROR,
    EF_LOG_LEVEL_NONE, /* 设为 level 可以彻底关闭日志 */
} ef_log_level_t;

typedef void (*ef_log_sink_t)(const char *line, uint32_t len);

ef_err_t ef_log_init(void);
void ef_log_set_sink(ef_log_sink_t sink);
void ef_log_set_level(ef_log_level_t level);

/** 内部格式化+分发函数，一般不直接调用，用下面的 EF_LOG_x 宏 */
void ef_log_write(ef_log_level_t level, const char *tag, const char *fmt, ...);

#define EF_LOG_D(tag, fmt, ...) ef_log_write(EF_LOG_LEVEL_DEBUG, (tag), (fmt), ##__VA_ARGS__)
#define EF_LOG_I(tag, fmt, ...) ef_log_write(EF_LOG_LEVEL_INFO, (tag), (fmt), ##__VA_ARGS__)
#define EF_LOG_W(tag, fmt, ...) ef_log_write(EF_LOG_LEVEL_WARN, (tag), (fmt), ##__VA_ARGS__)
#define EF_LOG_E(tag, fmt, ...) ef_log_write(EF_LOG_LEVEL_ERROR, (tag), (fmt), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* EF_LOG_H */
