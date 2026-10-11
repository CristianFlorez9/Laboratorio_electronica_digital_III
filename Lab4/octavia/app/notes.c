#include "notes.h"
#include <math.h>

NotaInfo tabla_notas[3][7];

static const int8_t semitonos_desde_do[7] = {0, 2, 4, 5, 7, 9, 11};

void notes_init(float frecuencia_muestreo) {
    for (int oct = 0; oct < 3; oct++) {
        for (int n = 0; n < 7; n++) {
            int indice = (oct - 1) * 12 + semitonos_desde_do[n] - 9;
            double freq = 440.0 * pow(2.0, indice / 12.0);
            uint32_t inc = (uint32_t)((freq * 4294967296.0) / (double)frecuencia_muestreo);

            tabla_notas[oct][n].frecuencia_hz = (float)freq;
            tabla_notas[oct][n].incremento_fase = inc;
        }
    }
}

uint32_t notes_get_incremento(uint8_t octava, uint8_t nota) {
    if (octava > 2) octava = 2;
    if (nota > 6) nota = 6;

    return tabla_notas[octava][nota].incremento_fase;
}