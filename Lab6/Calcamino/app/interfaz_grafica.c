/**
 * @file    interfaz_grafica.c
 * @brief   Implementación de las pantallas de configuración y sesión en OLED.
 */

#include "interfaz_grafica.h"
#include "drivers/pantalla_oled.h"
#include <stdio.h>

void interfaz_cnfg(uint32_t kcal, uint32_t peso, uint32_t altura, uint32_t bateria, uint8_t campo) {
    char buffer[32];

    oled_clear();

    snprintf(buffer, sizeof(buffer), "CONFIG USUARIO");
    oled_escribir_texto(0, 0, buffer);

    snprintf(buffer, sizeof(buffer), "Bat:%lu%%", (unsigned long)bateria);
    oled_escribir_texto(86, 0, buffer);

    snprintf(buffer, sizeof(buffer), "%cPeso: %lu kg",
             (campo == 0u) ? '>' : ' ', (unsigned long)peso);
    oled_escribir_texto(0, 16, buffer);

    snprintf(buffer, sizeof(buffer), "%cEstatura: %lu cm",
             (campo == 1u) ? '>' : ' ', (unsigned long)altura);
    oled_escribir_texto(0, 28, buffer);

    snprintf(buffer, sizeof(buffer), " Kcal: %lu kcal", (unsigned long)kcal);
    oled_escribir_texto(0, 40, buffer);

    oled_show();
}

void interfaz_sesion(uint32_t pasos, float estado, int cadence, uint32_t kcal, uint32_t bateria) {
    char buffer[32];

    oled_clear();

    snprintf(buffer, sizeof(buffer), "Est:%d", (int)estado);
    oled_escribir_texto(0, 0, buffer);

    snprintf(buffer, sizeof(buffer), "Bat:%lu%%", (unsigned long)bateria);
    oled_escribir_texto(86, 0, buffer);

    snprintf(buffer, sizeof(buffer), "Pasos: %lu", (unsigned long)pasos);
    oled_escribir_texto(0, 16, buffer);

    snprintf(buffer, sizeof(buffer), "Kcal: %lu kcal", (unsigned long)kcal);
    oled_escribir_texto(0, 28, buffer);

    snprintf(buffer, sizeof(buffer), "Cadencia: %d SPM", cadence);
    oled_escribir_texto(0, 40, buffer);

    oled_show();
}