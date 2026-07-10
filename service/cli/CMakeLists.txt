add_library(ef_cli STATIC
    ef_cli.c
)

target_include_directories(ef_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ef_cli PUBLIC ef_common ef_device)
