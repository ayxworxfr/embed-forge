#!/usr/bin/env bash
# flash.sh — 把编译好的固件烧进 Nucleo-F411RE（板载 ST-Link）
#
# 用法：tools/flash.sh [target-bare|target-freertos]  （默认 target-bare）
#
# 优先用 st-flash（stlink-tools 包，Linux/macOS 通过包管理器装，Windows 用
# msys2 的 mingw-w64-x86_64-stlink），找不到就退回 openocd。两者都没装就报错
# 退出，本脚本不负责帮你装烧录工具。

set -euo pipefail

PRESET="${1:-target-bare}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-${PRESET}"
BIN_FILE="${BUILD_DIR}/app/embed_forge.bin"
FLASH_ADDR="0x08000000"

if [[ "${PRESET}" != "target-bare" && "${PRESET}" != "target-freertos" ]]; then
    echo "错误: preset 必须是 target-bare 或 target-freertos，收到: ${PRESET}" >&2
    exit 1
fi

if [[ ! -f "${BIN_FILE}" ]]; then
    echo "错误: 找不到 ${BIN_FILE}" >&2
    echo "先编译: cmake --preset ${PRESET} && cmake --build --preset ${PRESET}" >&2
    exit 1
fi

if command -v st-flash >/dev/null 2>&1; then
    echo "使用 st-flash 烧录 ${BIN_FILE} -> ${FLASH_ADDR}"
    st-flash write "${BIN_FILE}" "${FLASH_ADDR}"
elif command -v openocd >/dev/null 2>&1; then
    echo "使用 openocd 烧录 ${BIN_FILE} -> ${FLASH_ADDR}"
    openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program ${BIN_FILE} ${FLASH_ADDR} verify reset exit"
else
    echo "错误: 没找到 st-flash 或 openocd，装一个再试" >&2
    echo "  Ubuntu/Debian: sudo apt install stlink-tools   (提供 st-flash)" >&2
    echo "  或:            sudo apt install openocd" >&2
    exit 1
fi
