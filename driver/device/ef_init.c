/*
 * ef_init.c — 仅在 EF_ENABLE_AUTO_INIT（真机固件）构建下编译。
 * host 单测的 CMake 目标不会把这个文件加入编译，避免依赖链接器
 * 段魔法在非 ELF 平台上的不确定行为。
 */
#include "ef_init.h"

#include <stddef.h>

#if defined(EF_ENABLE_AUTO_INIT)

extern const ef_init_fn_t __ef_init_start[];
extern const ef_init_fn_t __ef_init_end[];

void ef_init_run_all(void)
{
    for (const ef_init_fn_t *p = __ef_init_start; p < __ef_init_end; p++) {
        if (*p != NULL) {
            (*p)();
        }
    }
}

#endif
