# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(polling_interrupciones C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ---------------------------------------------------------------------------
# Ejecutable: versión Polling + IRQs
# ---------------------------------------------------------------------------
add_executable(polling_interrupciones
    polling_interrupciones.c
    drivers/uart.c
    app/capture.c
    app/measure.c
)

pico_set_program_name(polling_interrupciones "polling_interrupciones")
pico_set_program_version(polling_interrupciones "0.1")

# Carpetas donde buscar los .h
target_include_directories(polling_interrupciones PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/app
    ${CMAKE_CURRENT_LIST_DIR}/drivers
)

# Advertencias del compilador (ayudan a detectar errores de tipos y signos)
target_compile_options(polling_interrupciones PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(polling_interrupciones
    pico_stdlib
    pico_sync
    hardware_uart
    hardware_gpio
    hardware_irq
    hardware_timer
    hardware_sync
    hardware_pwm
)

# Sin stdio: UART0 es exclusivamente del driver propio (reportes hacia el
# convertidor USB-UART). El USB nativo de la Pico solo se usa para programar.
pico_enable_stdio_uart(polling_interrupciones 0)
pico_enable_stdio_usb(polling_interrupciones 0)

pico_add_extra_outputs(polling_interrupciones)

# Dejar registrada la configuración de compilación en la salida de CMake
# (para citarla en el informe: SDK, placa, tipo de build)
message(STATUS "SDK: ${sdkVersion} | Placa: ${PICO_BOARD} | Build: ${CMAKE_BUILD_TYPE}")