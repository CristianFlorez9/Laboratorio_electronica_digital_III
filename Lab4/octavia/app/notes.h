#ifndef NOTES_H_
#define NOTES_H_

#include <stdint.h>

/** Estructura para guardar la información de cada nota*/
typedef struct {
    float frecuencia_hz;
    uint32_t incremento_fase;
} NotaInfo;

/**
* @brief Inicializa el incremento que toma el systick para todas las notas que soporta el sistema octavia
* @param frecuencia_muestreo necesario para el calculo del incremento: inc = (f_(out)*2^N)/f_s
* @note Se llama una sola vez para evitar gastar recursos cada vez que se presione una nota
*/
void notes_init(float frecuencia_muestreo);

/**
* @brief Retorno el incremento que usa el systick para una nota
* @param octava Octava actual del sistema
* @param nota Nota presionada por el usuario
* @return El valor del incremento para cada nota segun la octava y nota vigente
*/
uint32_t notes_get_incremento(uint8_t octava, uint8_t nota);

#endif /* NOTES_H_ */
