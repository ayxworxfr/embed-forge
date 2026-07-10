add_library(ef_ringbuffer STATIC
    ef_ringbuffer.c
)

target_include_directories(ef_ringbuffer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ef_ringbuffer PUBLIC ef_common)
