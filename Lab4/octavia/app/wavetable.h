#ifndef WAVETABLE_H_
#define WAVETABLE_H_

#include <stdint.h>

/** @brief Número de muestras de la tabla (un ciclo completo). */
#define TABLE_SIZE 256

/**
* @brief Tabla con los valores de un ciclo de la señal sen(x)
* @note Valores de 8 bits sin signo (0 a 255), centrados en 128, que es el nivel de reposo del DAC.
* @note Declarado como extern para indicar que esta declarado en otro archivo .c,
* el calificador const indica que esta almacenado en memoria flash.
*/
extern const uint8_t tabla_onda[TABLE_SIZE];

#endif /* WAVETABLE_H_ */