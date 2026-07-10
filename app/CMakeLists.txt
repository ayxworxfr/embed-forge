add_executable(embed_forge.elf
    app_main.c
    app_led_blink.c
)

target_link_libraries(embed_forge.elf PRIVATE
    driver
    ef_log
    ef_event_bus
    ef_cli
    ef_config
    osal_impl
    board
)

if(CMAKE_OBJCOPY)
    add_custom_command(TARGET embed_forge.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:embed_forge.elf> embed_forge.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:embed_forge.elf> embed_forge.bin
        COMMENT "Generating embed_forge.hex / embed_forge.bin"
    )
endif()
