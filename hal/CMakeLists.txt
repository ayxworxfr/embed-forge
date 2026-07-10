add_library(hal STATIC
    stm32f4xx/hal_gpio_stm32f4.c
    stm32f4xx/hal_uart_stm32f4.c
)

target_include_directories(hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hal PUBLIC ef_common stm32f4xx_hal_driver)
