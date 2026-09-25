# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# Requiere la variable de entorno PICO_SDK_PATH apuntando al pico-sdk.
include(pico_sdk_import.cmake)

project(PulsoMemoria C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(PulsoMemoria
    PulsoMemoria.c
    Juego.c
    Dispositivos.c
    decoder_display.c
)

target_include_directories(PulsoMemoria PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(PulsoMemoria PRIVATE -Wall -Wextra)

target_link_libraries(PulsoMemoria
    pico_stdlib
    hardware_gpio
    pico_rand   # get_rand_32(), usado en Juego.c para la secuencia aleatoria
)

# newlib-nano no imprime floats con printf("%f") por defecto; esta bandera lo habilita
# (se usa en Dispositivos.c para mostrar segundos con 3 decimales).
target_link_options(PulsoMemoria PRIVATE "-Wl,-u,_printf_float")

pico_enable_stdio_usb(PulsoMemoria 1)
pico_enable_stdio_uart(PulsoMemoria 0)

pico_add_extra_outputs(PulsoMemoria)
