/*
 * ef_init.h — 模块自动注册（对齐 RT-Thread INIT_EXPORT）
 *
 * 新增一个驱动/服务模块时，只需要在其 .c 文件末尾写一行
 * EF_INIT_EXPORT(xxx_init, EF_INIT_LEVEL_DRIVER)，不需要去改
 * app_main.c 手动调用——这是"加一个功能，只需注册进框架"的具体实现。
 *
 * 实现机制：链接器把每个 EF_INIT_EXPORT 生成的函数指针放进对应的
 * `.ef_init.<level>` 段，链接脚本（见 board 子目录下的 .ld 文件，如
 * board/nucleo_f411re/STM32F411RETx_FLASH.ld）按段名字典序把
 * 这些段排列在一起，ef_init_run_all() 遍历 [__ef_init_start,
 * __ef_init_end) 依次调用。
 *
 * 只在交叉编译（真机固件）时启用：EF_ENABLE_AUTO_INIT 由根
 * CMakeLists.txt 只在 CMAKE_CROSSCOMPILING 时定义。PC 单元测试下这个
 * 宏什么都不做——测试代码显式调用各模块的 init 函数，不依赖链接器
 * 段的魔法（host 平台的目标文件格式不保证支持这个技巧，详见
 * docs/architecture.md）。
 */
#ifndef EF_INIT_H
#define EF_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#define EF_INIT_LEVEL_BOARD "1_board"
#define EF_INIT_LEVEL_DRIVER "2_driver"
#define EF_INIT_LEVEL_SERVICE "3_service"
#define EF_INIT_LEVEL_APP "4_app"

#include "ef_common.h"

typedef ef_err_t (*ef_init_fn_t)(void);

#if defined(EF_ENABLE_AUTO_INIT)

#define EF_INIT_EXPORT(fn, level)                                                                  \
    __attribute__((used,                                                                           \
                   section(".ef_init." level))) static const ef_init_fn_t _ef_init_ptr_##fn = fn

/** 依次执行所有通过 EF_INIT_EXPORT 注册的初始化函数，只在真机固件里调用一次 */
void ef_init_run_all(void);

#else

/* host 单测/未启用自动注册的构建下，宏退化为空：函数仍然是普通函数，
 * 由测试代码或 app 层显式调用 */
#define EF_INIT_EXPORT(fn, level) typedef int ef_init_export_unused_##fn

#endif /* EF_ENABLE_AUTO_INIT */

#ifdef __cplusplus
}
#endif

#endif /* EF_INIT_H */
