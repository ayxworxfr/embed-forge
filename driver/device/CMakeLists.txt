# ef_device 是纯 C、零硬件依赖的设备框架核心，host 和 target 都会编译它，
# 这也是"设备注册/查找机制本身可以脱离硬件做单元测试"的关键。
add_library(ef_device STATIC
    ef_device.c
    ef_init.c
)

target_include_directories(ef_device PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ef_device PUBLIC ef_common)
