#!/usr/bin/env bash
# new_driver.sh — 生成一个新驱动模块的骨架文件（driver/drv_<name>.h/.c）
#
# 用法：tools/new_driver.sh <name>
# 例：  tools/new_driver.sh w25qxx  ->  driver/drv_w25qxx.h / driver/drv_w25qxx.c
#
# 生成的骨架只包含"能编译通过的最小结构"，具体的 ops 实现、是否要注册成
# ef_device 由你自己决定——不是所有驱动都需要走设备框架（比如按键这种
# "轮询读 + 回调通知"模式的就没走 ef_device，参考 drv_button.c）。

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "用法: $0 <driver_name>" >&2
    echo "例:   $0 w25qxx" >&2
    exit 1
fi

NAME="$1"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HEADER="${ROOT_DIR}/driver/drv_${NAME}.h"
SOURCE="${ROOT_DIR}/driver/drv_${NAME}.c"
GUARD="DRV_$(echo "${NAME}" | tr '[:lower:]' '[:upper:]')_H"

if [[ -e "${HEADER}" || -e "${SOURCE}" ]]; then
    echo "错误: driver/drv_${NAME}.{h,c} 已存在，不覆盖" >&2
    exit 1
fi

cat > "${HEADER}" <<EOF
/*
 * drv_${NAME}.h — TODO: 一句话描述这个驱动对上层暴露的"设备语义"
 * （是什么功能，不是怎么实现），参考 driver/drv_led.h 的写法。
 */
#ifndef ${GUARD}
#define ${GUARD}

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

ef_err_t drv_${NAME}_init(void);

/* TODO: 补上层需要的操作接口，命名用 drv_${NAME}_xxx，不要暴露硬件细节 */

#ifdef __cplusplus
}
#endif

#endif /* ${GUARD} */
EOF

cat > "${SOURCE}" <<EOF
#include "drv_${NAME}.h"

#include "ef_init.h"

ef_err_t drv_${NAME}_init(void)
{
    /* TODO: 调用 hal_xxx_init() 完成硬件初始化 */
    return EF_OK;
}
EF_INIT_EXPORT(drv_${NAME}_init, EF_INIT_LEVEL_DRIVER);

/* TODO: 实现 .h 里声明的操作接口 */
EOF

echo "已生成:"
echo "  ${HEADER}"
echo "  ${SOURCE}"
echo ""
echo "接下来手动做的事："
echo "  1. 把 drv_${NAME}.c 加进 driver/CMakeLists.txt 的源文件列表"
echo "  2. 如果需要按名字查找/统一 read/write 接口，注册成 ef_device（参考 drv_led.c）"
echo "  3. 如果上层业务逻辑需要针对这个驱动写单测，在 test/mocks/ 补一份同签名的 mock"
echo "  4. 读一遍 docs/architecture.md 第 1 节，确认没有把 HAL/寄存器细节泄漏到 .h 里"
