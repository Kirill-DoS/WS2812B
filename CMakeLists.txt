cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ws2812_sound_wave_project)

pico_sdk_init()

add_executable(main
    ws2812.c
)

pico_enable_stdio_uart(main 1)
pico_enable_stdio_usb(main 1)

target_sources(main PRIVATE
    ws2812.c
    #effects.c
)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

target_link_libraries(main
    pico_stdlib
    hardware_pio
    hardware_clocks
)

pico_add_extra_outputs(main)
