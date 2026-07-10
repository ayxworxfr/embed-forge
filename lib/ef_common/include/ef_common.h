/*
 * ef_common.h — 全局公共定义：统一错误码、断言宏、编译期工具宏
 *
 * 本文件不依赖任何硬件/OS，是整个分层架构的最底层公共契约。
 * 所有层（HAL/Driver/OSAL/Service/App）都可以 include 本文件，
 * 但本文件绝不 include 上层任何头文件。
 */
#ifndef EF_COMMON_H
#define EF_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* 统一错误码：所有对外接口必须返回 ef_err_t，禁止裸 -1/0 混用语义 */
typedef enum {
    EF_OK = 0,                /* 成功 */
    EF_ERR = -1,              /* 未分类错误 */
    EF_ERR_PARAM = -2,        /* 参数非法 */
    EF_ERR_TIMEOUT = -3,      /* 超时 */
    EF_ERR_BUSY = -4,         /* 资源忙 */
    EF_ERR_NOMEM = -5,        /* 内存/资源不足 */
    EF_ERR_NOTFOUND = -6,     /* 未找到（设备/命令/键值） */
    EF_ERR_NOTSUPPORTED = -7, /* 不支持的操作 */
    EF_ERR_IO = -8,           /* 底层 I/O 失败 */
} ef_err_t;

/* 断言失败处理钩子，默认实现在 ef_common.c（弱符号），可被具体平台覆盖 */
void ef_assert_fail(const char *file, int line, const char *expr);

#define EF_ASSERT(expr)                                                                            \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            ef_assert_fail(__FILE__, __LINE__, #expr);                                             \
        }                                                                                          \
    } while (0)

#define EF_UNUSED(x) ((void)(x))

#define EF_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define EF_MIN(a, b) ((a) < (b) ? (a) : (b))
#define EF_MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef __cplusplus
}
#endif

#endif /* EF_COMMON_H */
