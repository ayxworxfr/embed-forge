#!/usr/bin/env bash
# new_service.sh — 生成一个新 Service 组件的骨架（service/<name>/...）
#
# 用法：tools/new_service.sh <name>
# 例：  tools/new_service.sh mqtt  ->  service/mqtt/{include/ef_mqtt.h, ef_mqtt.c, CMakeLists.txt}
#
# Service 组件的硬性要求（docs/architecture.md 第 1/5 节）：不能出现 hal_*/
# board_config.h/芯片专属类型，必须能在 host 上独立编译测试。

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "用法: $0 <service_name>" >&2
    echo "例:   $0 mqtt" >&2
    exit 1
fi

NAME="$1"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SVC_DIR="${ROOT_DIR}/service/${NAME}"
GUARD="EF_$(echo "${NAME}" | tr '[:lower:]' '[:upper:]')_H"

if [[ -e "${SVC_DIR}" ]]; then
    echo "错误: service/${NAME} 已存在，不覆盖" >&2
    exit 1
fi

mkdir -p "${SVC_DIR}/include"

cat > "${SVC_DIR}/include/ef_${NAME}.h" <<EOF
/*
 * ef_${NAME}.h — TODO: 一句话描述这个组件解决什么问题。
 * 硬性要求：不允许出现 hal_*/board_config.h/芯片专属类型，必须能在 host
 * 上独立编译测试（参考 service/log/、service/config/ 的写法）。
 */
#ifndef ${GUARD}
#define ${GUARD}

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

ef_err_t ef_${NAME}_init(void);

/* TODO: 补对外接口 */

/** 仅用于单元测试：清空内部状态，避免测试用例之间互相污染 */
void ef_${NAME}_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* ${GUARD} */
EOF

cat > "${SVC_DIR}/ef_${NAME}.c" <<EOF
#include "ef_${NAME}.h"

#include "ef_init.h"

ef_err_t ef_${NAME}_init(void)
{
    ef_${NAME}_reset();
    return EF_OK;
}
EF_INIT_EXPORT(ef_${NAME}_init, EF_INIT_LEVEL_SERVICE);

/* TODO: 实现 .h 里声明的对外接口 */

void ef_${NAME}_reset(void)
{
    /* TODO: 清空内部静态状态 */
}
EOF

cat > "${SVC_DIR}/CMakeLists.txt" <<EOF
add_library(ef_${NAME} STATIC
    ef_${NAME}.c
)

target_include_directories(ef_${NAME} PUBLIC \${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ef_${NAME} PUBLIC ef_common ef_device)
EOF

echo "已生成:"
echo "  ${SVC_DIR}/include/ef_${NAME}.h"
echo "  ${SVC_DIR}/ef_${NAME}.c"
echo "  ${SVC_DIR}/CMakeLists.txt"
echo ""
echo "接下来手动做的事："
echo "  1. 在 service/CMakeLists.txt 加一行: add_subdirectory(${NAME})"
echo "  2. 在 test/CMakeLists.txt 用 ef_add_host_test() 注册对应单测"
echo "  3. 实现 .h/.c 里的 TODO"
