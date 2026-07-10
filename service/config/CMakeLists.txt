add_library(ef_config STATIC
    ef_config.c
)

target_include_directories(ef_config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ef_config PUBLIC ef_common ef_device)
