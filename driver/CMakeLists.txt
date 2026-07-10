add_subdirectory(device)

# 真实硬件驱动依赖 board_config.h/hal，只在交叉编译目标固件时构建
add_library(driver STATIC
    drv_led.c
    drv_button.c
    drv_uart_console.c
)

target_include_directories(driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(driver PUBLIC ef_device hal osal_impl board)
