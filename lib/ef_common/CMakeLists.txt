add_library(ef_common STATIC
    ef_common.c
)

target_include_directories(ef_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
