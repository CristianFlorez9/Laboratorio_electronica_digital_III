#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "interfaz_grafica.h"
#include "drivers/pantalla_oled.h"



void interfaz_cnfg(uint32_t kcal, uint32_t peso, uint32_t altura, uint32_t bateria){
    char buffer[32]; // Un lienzo temporal de texto (caben 31 letras)

    oled_clear(); // Limpiamos el framebuffer en RAM

    // Formateamos y dibujamos las kcal
    // snprintf junta el texto "Kcal: " con la variable entera (%lu para uint32_t)
    snprintf(buffer, sizeof(buffer), "kcal: %lu kcal", kcal);
    oled_escribir_texto(0, 10, buffer); // Lo dibujamos arriba a la izquierda

    // Formateamos y dibujamos el peso
    snprintf(buffer, sizeof(buffer), "Peso: %lu Kg", peso);
    oled_escribir_texto(0, 20, buffer); // Lo dibujamos un poco más abajo (Y=20)

    // Formateamos y dibujamos la Altura
    snprintf(buffer, sizeof(buffer), "Altura: %lu m", altura);
    oled_escribir_texto(0, 30, buffer); // Lo dibujamos un poco más abajo (Y=30)

    // Formateamos y dibujamos la BATERÍA (Alineado a la derecha, aprox X=100)
    snprintf(buffer, sizeof(buffer), "Bat:%lu%%", bateria);
    oled_escribir_texto(100, 0, buffer); 

    // Enviamos el mapa de píxeles terminado a la pantalla real
    oled_show(); 
}

void interfaz_sesion(uint32_t pasos, float estado, int cadence, uint32_t kcal, uint32_t bateria) {
    char buffer[32]; // Un lienzo temporal de texto (caben 31 letras)

    oled_clear(); // Limpiamos el framebuffer en RAM

    // Formateamos y dibujamos los PASOS
    // snprintf junta el texto "Pasos: " con la variable entera (%lu para uint32_t)
    snprintf(buffer, sizeof(buffer), "Pasos: %lu", pasos);
    oled_escribir_texto(0, 10, buffer); // Lo dibujamos arriba a la izquierda

    // Formateamos y dibujamos las KCAL
    snprintf(buffer, sizeof(buffer), "Kcal: %lu kcal", kcal);
    oled_escribir_texto(0, 20, buffer); // Lo dibujamos un poco más abajo (Y=20)

    // Formateamos y dibujamos la cadencia
    snprintf(buffer, sizeof(buffer), "Cadence: %lu SPM", cadence);
    oled_escribir_texto(0, 30, buffer); // Lo dibujamos un poco más abajo (Y=30)

    // Formateamos y dibujamos el ESTADO (float usa %.1f para un decimal)
    snprintf(buffer, sizeof(buffer), "Est: %.1f", estado);
    oled_escribir_texto(0, 40, buffer);

    // Formateamos y dibujamos la BATERÍA (Alineado a la derecha, aprox X=100)
    snprintf(buffer, sizeof(buffer), "Bat:%lu%%", bateria);
    oled_escribir_texto(100, 0, buffer); 

    // Enviamos el mapa de píxeles terminado a la pantalla real
    oled_show(); 
}
