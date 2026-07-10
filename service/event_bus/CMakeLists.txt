add_library(ef_event_bus STATIC
    ef_event_bus.c
)

target_include_directories(ef_event_bus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ef_event_bus PUBLIC ef_common ef_device)
