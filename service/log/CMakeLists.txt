add_library(ef_log STATIC
    ef_log.c
)

target_include_directories(ef_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ef_log PUBLIC ef_common ef_device)
