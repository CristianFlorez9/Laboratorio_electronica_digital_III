#include "notes.h"
#include <math.h>

#define FS 40000.0f                   // Tu frecuencia de muestreo
#define MAX_ACCUMULATOR 4294967296.0f // 2^32 (para el acumulador de 32 bits)

// Matriz para guardar las 3 octavas x 7 notas (21 notas en total)
NotaInfo tabla_notas[3][7];

// Índices de semitonos respecto a La4 (A4). Usamos int8_t porque hay negativos.
const int8_t indice_n[3][7] = {
    {-20, -18, -16, -15, -13, -11, -10},    // Octava 3
    {-9,  -7,  -5,  -4,  -2,   0,   2},     // Octava 4, contiene La4
    { 3,   5,   7,   8,  10,  12,  14}      // Octava 5
};

void notes_init(float frecuencia_muestreo) {
    for (int oct = 0; oct < 3; oct++) {
        for (int not = 0; not < 7; not++) {
            float freq = 440.0f * powf(2.0f, indice_n[oct][not] / 12.0f);
            
            // Usamos la frecuencia_muestreo que nos pasan al inicializar
            uint32_t inc = (uint32_t)((freq * 4294967296.0f) / frecuencia_muestreo);
            
            tabla_notas[oct][not].frecuencia_hz = freq;
            tabla_notas[oct][not].incremento_fase = inc;
        }
    }
}

uint32_t notes_get_incremento(uint8_t octava, uint8_t nota) {
    // Validar límites para evitar desbordamientos de memoria
    if (octava > 2) octava = 2;
    if (nota > 6) nota = 6;
    
    // Retornamos el incremento que el Systick debe usar
    return tabla_notas[octava][nota].incremento_fase;
}