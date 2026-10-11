#ifndef NOTES_H_
#define NOTES_H_

#include <stdint.h>

// Estructura para guardar la información de cada nota
typedef struct {
    float frecuencia_hz;
    uint32_t incremento_fase;
} NotaInfo;

// Llama a esta función una sola vez en el main() para precalcular la tabla
void notes_init(float frecuencia_muestreo);

// Le das la octava (0-2) y la nota (0-6) y te devuelve el incremento para el DDS
uint32_t notes_get_incremento(uint8_t octava, uint8_t nota);

#endif /* NOTES_H_ */