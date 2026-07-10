cmake_minimum_required(VERSION 3.20)

if(CMAKE_TOOLCHAIN_FILE MATCHES "arm-none-eabi")
    project(embed_forge C ASM)
else()
    project(embed_forge C)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# OSAL 后端只在交叉编译真机固件时有意义；host 单测固定用 osal/host。
set(EF_OSAL_BACKEND "bare" CACHE STRING "Firmware OSAL backend: bare|freertos")
set_property(CACHE EF_OSAL_BACKEND PROPERTY STRINGS bare freertos)

if(CMAKE_CROSSCOMPILING)
    message(STATUS "embed-forge: building MCU firmware (target=STM32F411RE, OSAL=${EF_OSAL_BACKEND})")

    add_subdirectory(lib/ef_common)
    include(${CMAKE_SOURCE_DIR}/cmake/third_party.cmake)
    # 模块自动注册（EF_INIT_EXPORT 的链接器段机制）只在真机固件里启用，
    # 原因见 driver/device/ef_init.h 顶部注释。
    add_compile_definitions(EF_ENABLE_AUTO_INIT)

    add_subdirectory(lib/ringbuffer)
    add_subdirectory(hal)

    if(EF_OSAL_BACKEND STREQUAL "bare")
        add_library(osal_impl STATIC osal/bare/osal_bare.c)
        target_link_libraries(osal_impl PUBLIC ef_common cmsis_device_f4)
    elseif(EF_OSAL_BACKEND STREQUAL "freertos")
        add_library(osal_impl STATIC osal/freertos/osal_freertos.c)
        target_link_libraries(osal_impl PUBLIC ef_common freertos_kernel)
    else()
        message(FATAL_ERROR "Unknown EF_OSAL_BACKEND: ${EF_OSAL_BACKEND} (expect bare|freertos)")
    endif()
    target_include_directories(osal_impl PUBLIC ${CMAKE_SOURCE_DIR}/osal/include)

    add_subdirectory(board/nucleo_f411re)
    add_subdirectory(driver)
    add_subdirectory(service)
    add_subdirectory(app)
else()
    message(STATUS "embed-forge: native host build -> unit tests only (use -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake to build firmware)")

    enable_testing()
    find_package(Threads REQUIRED)

    add_subdirectory(third_party/unity)
    add_subdirectory(lib/ef_common)
    add_subdirectory(lib/ringbuffer)
    add_subdirectory(driver/device)

    add_library(osal_impl STATIC osal/host/osal_host.c)
    target_include_directories(osal_impl PUBLIC ${CMAKE_SOURCE_DIR}/osal/include)
    target_link_libraries(osal_impl PUBLIC ef_common Threads::Threads)

    add_subdirectory(service)
    add_subdirectory(test)
endif()
