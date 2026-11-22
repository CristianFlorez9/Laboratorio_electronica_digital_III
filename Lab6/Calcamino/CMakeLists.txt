cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(Calcamino C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(Calcamino
    Calcamino.c
    app/interfaz_grafica.c
    app/modelo_bateria.c
    app/modelo_calorias.c
    app/ring_buffer.c
    app/state_machine.c
    app/step_detector.c
    drivers/I2C_driver.c
    drivers/acelerometro_MPU_6050.c
    drivers/adc_driver.c
    drivers/pantalla_oled.c
    drivers/power_driver.c
)

target_include_directories(Calcamino PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/app
    ${CMAKE_CURRENT_LIST_DIR}/drivers
)

target_link_libraries(Calcamino
    pico_stdlib
    hardware_i2c
    hardware_adc
    hardware_clocks
    hardware_pll
    hardware_timer
    hardware_sync
    m
)

pico_enable_stdio_usb(Calcamino 1)
pico_enable_stdio_uart(Calcamino 0)
pico_add_extra_outputs(Calcamino)