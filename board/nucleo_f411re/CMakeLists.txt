add_library(board STATIC
    board_init.c
    stm32f4xx_it.c
    startup_stm32f411xe.s
)

target_include_directories(board PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(board PUBLIC hal stm32f4xx_hal_driver ef_common)

target_link_options(board PUBLIC
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F411RETx_FLASH.ld
)

if(EF_OSAL_BACKEND STREQUAL "bare")
    target_compile_definitions(board PUBLIC EF_OSAL_BACKEND_BARE)
elseif(EF_OSAL_BACKEND STREQUAL "freertos")
    target_compile_definitions(board PUBLIC EF_OSAL_BACKEND_FREERTOS)
endif()
