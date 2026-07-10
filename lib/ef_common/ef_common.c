#include "ef_common.h"

/*
 * 默认断言失败处理：死循环挂起。
 * - 裸机/RTOS 固件：这是最安全的默认行为（不依赖尚未初始化的 log/OSAL）。
 * - PC 单元测试：test/host 里用 __attribute__((weak)) 覆盖为 abort()，
 *   让断言失败在 CI 里表现为测试失败而不是挂死。
 */
__attribute__((weak)) void ef_assert_fail(const char *file, int line, const char *expr)
{
    EF_UNUSED(file);
    EF_UNUSED(line);
    EF_UNUSED(expr);
    for (;;) {
        /* 故意留空：断言失败后禁止继续执行任何业务逻辑 */
    }
}
