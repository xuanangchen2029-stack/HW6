cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(HW6_usb_mouse C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(HW6_usb_mouse
    HW6_main.c
    mpu6050.c
    usb_descriptors.c
)

target_include_directories(HW6_usb_mouse PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(HW6_usb_mouse
    pico_stdlib
    hardware_i2c
    hardware_gpio
    tinyusb_device
    tinyusb_board
    pico_float          # needed for fabsf() at link time
)

# Enable USB output, disable UART stdio (optional — comment out if you want UART debug)
pico_enable_stdio_usb(HW6_usb_mouse 0)
pico_enable_stdio_uart(HW6_usb_mouse 0)

pico_add_extra_outputs(HW6_usb_mouse)
