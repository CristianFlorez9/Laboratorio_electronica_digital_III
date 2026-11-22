/**
 * @file    pantalla_oled.h
 * @brief   Driver de la pantalla gráfica con framebuffer y envío asíncrono.
 * @details El dibujo se hace en un framebuffer en RAM (contexto main). El envío
 *          al panel se hace por bloques mediante el bus, sin bloquear la CPU.
 */

#ifndef PANTALLA_OLED_H
#define PANTALLA_OLED_H

#include <pico/stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "drivers/I2C_driver.h" 

#define OLED_ADDR       0x3C
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_BUF_LEN    (OLED_WIDTH * OLED_HEIGHT / 8) // 1024 bytes

// Ya no exponemos oled_cmd() aquí. Es mala práctica dejar que la aplicación
// superior mande comandos crudos de forma asíncrona si no maneja bien la memoria.

// Pasamos el bus a la inicialización
void oled_init(i2c_inst_t *bus); 

void oled_show(void);

void oled_clear(void);

void oled_pixel(int x, int y, bool on);

void oled_escribir_texto(int x, int y, const char *texto);

#endif /* PANTALLA_OLED_H */
