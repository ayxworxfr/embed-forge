# toolchain-arm-none-eabi.cmake — STM32F4 (Cortex-M4F) 交叉编译工具链文件
#
# 用法：cmake -B build-target -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake
# 依赖：系统 PATH 里的 arm-none-eabi-gcc 工具链（apt install gcc-arm-none-eabi 或
#       从 ARM 官网下载 arm-gnu-toolchain）。

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE FILEPATH "")
set(CMAKE_SIZE arm-none-eabi-size CACHE FILEPATH "")

# 交叉编译时不能运行目标程序来探测编译器，必须用静态库探测方式
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -ffunction-sections -fdata-sections -fno-common -Wall -Wextra")
# startup_stm32f411xe.s 是纯 GNU 汇编（无 #include）：
# - 不用 arm-none-eabi-as：CMake 会把 -D 宏传给 as，触发 bad defsym
# - 不用 assembler-with-preprocessor：Ubuntu apt 工具链不支持
# - 用 gcc + -x assembler，且不传 <DEFINES>
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")
set(CMAKE_ASM_COMPILE_OBJECT
    "<CMAKE_ASM_COMPILER> <INCLUDES> <FLAGS> -x assembler -c <SOURCE> -o <OBJECT>")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections -Wl,--print-memory-usage"
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
