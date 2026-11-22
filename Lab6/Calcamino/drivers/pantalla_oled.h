/**
 * @file    pantalla_oled.h
 * @brief   Driver de la pantalla gráfica OLED SSD1306 con framebuffer y envío asíncrono.
 * @details El dibujo se realiza en un framebuffer en RAM (contexto main). El envío
 *          al panel se encadena por bloques mediante interrupciones I2C sin bloquear la CPU.
 */

#ifndef PANTALLA_OLED_H
#define PANTALLA_OLED_H

#include <stdint.h>
#include <stdbool.h>
#include "drivers/I2C_driver.h"

#define OLED_ADDR       0x3Cu
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_BUF_LEN    (OLED_WIDTH * OLED_HEIGHT / 8) /* 1024 bytes */

/**
 * @brief Inicializa la pantalla OLED de forma asíncrona sobre el bus I2C indicado.
 * @param bus Instancia del bus I2C previamente inicializada (ej. i2c0).
 */
void oled_init(i2c_inst_t *bus);

/**
 * @brief Consulta si hay una transferencia I2C de pantalla en curso.
 * @return true si el driver está enviando comandos o framebuffer; false si está libre.
 */
bool oled_is_busy(void);

/**
 * @brief Enciende (0xAF) o apaga (0xAE) el panel OLED de forma no bloqueante.
 * @param on true para encender la pantalla; false para apagarla en reposo.
 */
void oled_set_power(bool on);

/**
 * @brief Inicia el envío asíncrono del framebuffer completo hacia la pantalla OLED.
 */
void oled_show(void);

/**
 * @brief Limpia el framebuffer en RAM (pone todos los píxeles en 0).
 */
void oled_clear(void);

/**
 * @brief Dibuja o borra un píxel en el framebuffer en RAM.
 * @param x  Coordenada horizontal (0 a 127).
 * @param y  Coordenada vertical (0 a 63).
 * @param on true para encender el píxel; false para apagarlo.
 */
void oled_pixel(int x, int y, bool on);

/**
 * @brief Escribe una cadena ASCII en el framebuffer usando fuente 5x7.
 * @param x     Coordenada horizontal inicial.
 * @param y     Coordenada vertical inicial.
 * @param texto Cadena terminada en '\0'.
 */
void oled_escribir_texto(int x, int y, const char *texto);

#endif /* PANTALLA_OLED_H */