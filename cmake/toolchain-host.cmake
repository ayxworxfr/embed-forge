# toolchain-host.cmake — PC 单元测试构建
#
# 实际上不需要交叉编译，直接用本机编译器即可，这个文件的作用只是让
# CMakePresets.json 里的 host-test preset 有一个显式、可读的入口，
# 并统一开启严格警告，让"业务逻辑在 PC 上跑测试"这件事本身也受到
# 和固件代码同等的编译告警检查。

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS_INIT "-Wall -Wextra -Wshadow -g -O0 --coverage")
set(CMAKE_EXE_LINKER_FLAGS_INIT "--coverage")
