# third_party.cmake — 手动、可读地把 third_party 下的官方仓库接成 CMake target。
#
# 不使用 cmsis_core 仓库自带的 CMakeLists.txt（那是面向 ARM CMSIS-Toolbox
# pack 体系的生成脚本，参数体系复杂，不适合直接嵌入一个教学向脚手架）。
# 这里手工声明最小可用的 INTERFACE / STATIC target，路径显式可读。

set(EF_THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# ---------------- CMSIS Core（ARM 官方 Cortex-M 寄存器/内核头文件） ----------------
add_library(cmsis_core INTERFACE)
target_include_directories(cmsis_core SYSTEM INTERFACE
    ${EF_THIRD_PARTY_DIR}/cmsis_core/CMSIS/Core/Include
)

# ---------------- CMSIS Device F4（ST 官方 STM32F4 系列设备头 + 启动辅助） ----------------
add_library(cmsis_device_f4 STATIC
    ${EF_THIRD_PARTY_DIR}/cmsis_device_f4/Source/Templates/system_stm32f4xx.c
)
target_include_directories(cmsis_device_f4 SYSTEM PUBLIC
    ${EF_THIRD_PARTY_DIR}/cmsis_device_f4/Include
)
target_link_libraries(cmsis_device_f4 PUBLIC cmsis_core)
# 目标芯片型号：换芯片时只需要改这一处 + board 层，cmsis/HAL 库源码不用动
target_compile_definitions(cmsis_device_f4 PUBLIC STM32F411xE)

# ---------------- STM32F4xx HAL Driver（ST 官方外设驱动库） ----------------
# 只编译本脚手架参考实现实际用到的模块，践行"可裁剪"原则：
# 需要更多外设（SPI/I2C/ADC...）时在这里追加对应 .c 即可。
add_library(stm32f4xx_hal_driver STATIC
    ${EF_THIRD_PARTY_DIR}/stm32f4xx_hal_driver/Src/stm32f4xx_hal.c
    ${EF_THIRD_PARTY_DIR}/stm32f4xx_hal_driver/Src/stm32f4xx_hal_cortex.c
    ${EF_THIRD_PARTY_DIR}/stm32f4xx_hal_driver/Src/stm32f4xx_hal_rcc.c
    ${EF_THIRD_PARTY_DIR}/stm32f4xx_hal_driver/Src/stm32f4xx_hal_flash.c
    ${EF_THIRD_PARTY_DIR}/stm32f4xx_hal_driver/Src/stm32f4xx_hal_dma.c
    ${EF_THIRD_PARTY_DIR}/stm32f4xx_hal_driver/Src/stm32f4xx_hal_gpio.c
    ${EF_THIRD_PARTY_DIR}/stm32f4xx_hal_driver/Src/stm32f4xx_hal_uart.c
)
target_include_directories(stm32f4xx_hal_driver SYSTEM PUBLIC
    ${EF_THIRD_PARTY_DIR}/stm32f4xx_hal_driver/Inc
)
target_link_libraries(stm32f4xx_hal_driver PUBLIC cmsis_device_f4 ef_common)
# stm32f4xx_hal_conf.h 是"项目级配置"，属于 board 层职责，不属于第三方库本身
target_include_directories(stm32f4xx_hal_driver PUBLIC
    ${CMAKE_SOURCE_DIR}/board/nucleo_f411re
)

# ---------------- FreeRTOS-Kernel（仅 EF_OSAL_BACKEND=freertos 时使用） ----------------
if(EF_OSAL_BACKEND STREQUAL "freertos")
    set(FREERTOS_PORT "GCC_ARM_CM4F" CACHE STRING "" FORCE)
    set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)

    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE
        ${CMAKE_SOURCE_DIR}/osal/freertos/config
    )
    target_link_libraries(freertos_config INTERFACE cmsis_device_f4)

    add_subdirectory(${EF_THIRD_PARTY_DIR}/freertos_kernel ${CMAKE_BINARY_DIR}/freertos_kernel)
endif()
