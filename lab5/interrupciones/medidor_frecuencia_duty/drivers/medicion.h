#ifndef MEDICION_H
#define MEDICION_H

#include <stdint.h>
#include <stdbool.h>


/** @brief Pin GPIO donde se conecta la señal a medir. */
#define SIGNAL_PIN 2

/** @brief Configura SIGNAL_PIN como entrada (pull-down) y habilita la IRQ en ambos flancos. */
void medicion_init(void);

/**
 * @brief Entrega una copia consistente (sin interrupciones) de la última medición.
 * @param periodo_us       Periodo entre flancos de subida [µs] (0 si aún no hay medida).
 * @param tiempo_alto_us   Tiempo en alto [µs].
 * @param ultimo_flanco_us Marca de tiempo del último flanco detectado [µs].
 */
void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us);

#endif
<<<<<<< HEAD
=======

>>>>>>> 3831f9eb6b90d3a63cf43fb8027a137d45b92edf
