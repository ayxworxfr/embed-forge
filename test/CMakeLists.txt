# PC 单元测试聚合入口，只在 host（非交叉编译）构建下被根 CMakeLists.txt 引入。

add_library(drv_led_mock STATIC mocks/drv_led_mock.c)
target_include_directories(drv_led_mock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mocks)
target_link_libraries(drv_led_mock PUBLIC ef_common)
# app_led_blink.c #include "drv_led.h"，声明在 driver/ 下，跟真实实现无关，
# host 侧只借用这份纯声明头文件，不链接 driver 库本身。
target_include_directories(drv_led_mock PUBLIC ${CMAKE_SOURCE_DIR}/driver)

# ef_add_host_test(<test_name> SOURCES <src...> LIBS <lib...>)
# 每个测试用例编译成独立可执行文件并注册进 CTest，互不干扰全局静态状态。
function(ef_add_host_test test_name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${test_name} ${ARG_SOURCES})
    target_link_libraries(${test_name} PRIVATE unity ${ARG_LIBS})
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

ef_add_host_test(test_ringbuffer
    SOURCES host/test_ringbuffer.c
    LIBS ef_ringbuffer
)

ef_add_host_test(test_ef_device
    SOURCES host/test_ef_device.c
    LIBS ef_device
)

ef_add_host_test(test_event_bus
    SOURCES host/test_event_bus.c
    LIBS ef_event_bus
)

ef_add_host_test(test_log
    SOURCES host/test_log.c
    LIBS ef_log
)

ef_add_host_test(test_config
    SOURCES host/test_config.c
    LIBS ef_config
)

ef_add_host_test(test_cli
    SOURCES host/test_cli.c
    LIBS ef_cli
)

ef_add_host_test(test_app_led_blink
    SOURCES host/test_app_led_blink.c ${CMAKE_SOURCE_DIR}/app/app_led_blink.c
    LIBS ef_event_bus ef_log drv_led_mock
)
target_include_directories(test_app_led_blink PRIVATE ${CMAKE_SOURCE_DIR}/app)
