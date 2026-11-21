
/**
 * @file modelo_bateria.h
 * @brief Modelo de batería basado en el voltaje medido.
 *
 * Estima el porcentaje de carga de una batería LiPo de 3.7 V
 * y 600 mAh mediante una curva de descarga, interpolación lineal
 * y un filtro para suavizar las variaciones del voltaje.
 *
 * El voltaje debe recibirse en milivoltios, previamente medido
 * y calibrado mediante el ADC y el divisor de tensión.
 *
 * Funcionamiento:
 * 1. Se recibe una lectura del voltaje de la batería.
 * 2. Se aplica un filtro exponencial para reducir el ruido
 *    y las variaciones momentáneas de la medición.
 * 3. Se calcula el porcentaje mediante interpolación lineal
 *    entre los puntos de la curva de descarga.
 *
 * La curva utilizada corresponde a una batería LiPo de una celda:
 *
 *   Voltaje (mV)    Porcentaje (%)
 *      4200             100
 *      4110              90
 *      4020              80
 *      3950              70
 *      3870              60
 *      3840              50
 *      3800              40
 *      3770              30
 *      3730              20
 *      3690              10
 *      3610               5
 *      3300               0
 *
 * Consideraciones:
 * - Por encima de 4200 mV se considera que la batería está al 100 %.
 * - Por debajo de 3300 mV se considera que está al 0 %.
 * - La curva es aproximada y puede variar según la batería utilizada.
 * - La temperatura y la corriente de descarga afectan la medición.
 * - Durante la carga, el voltaje no representa directamente la
 *   capacidad restante.
 * - El envejecimiento de la batería tampoco está contemplado.
 *
 * Este modelo proporciona una estimación del porcentaje de carga,
 * no una medición exacta de la capacidad restante.
 */

#ifndef MODELO_BATERIA_H
#define MODELO_BATERIA_H

#include <stdint.h>
#include <stdbool.h>

/* Capacidad nominal de la batería en mAh. */
#define BATERIA_CAPACIDAD_MAH   600u

/**
 * Divisor utilizado por el filtro exponencial.
 * Un valor mayor produce lecturas más suaves, pero una respuesta
 * más lenta ante los cambios de voltaje.
 */
#ifndef BATERIA_FILTRO_DIV
#define BATERIA_FILTRO_DIV      16
#endif

/**
 * Almacena el estado del filtro de voltaje de la batería.
 *
 * El voltaje se guarda utilizando 4 bits fraccionarios para
 * conservar mayor precisión durante los cálculos internos.
 */
typedef struct
{
    int32_t mv_q4;          /**< Voltaje filtrado en formato de punto fijo. */
    bool    inicializado;   /**< Indica si ya se recibió una lectura. */
} bateria_t;


/**
 * Calcula el porcentaje de batería a partir del voltaje medido.
 *
 * Utiliza la curva de descarga y una interpolación lineal
 * para obtener el porcentaje correspondiente.
 *
 * El resultado está limitado entre 0 y 100 %.
 */
uint8_t bateria_pct_desde_mv(uint16_t mv);


/**
 * Inicializa el estado del filtro de batería.
 *
 * Debe llamarse antes de comenzar a actualizar las mediciones.
 * La primera lectura recibida se utilizará como valor inicial.
 */
void bateria_init(bateria_t *b);


/**
 * Actualiza el filtro con una nueva lectura de voltaje.
 *
 * Suaviza la medición y calcula el porcentaje de carga
 * correspondiente al voltaje filtrado.
 *
 * Retorna el porcentaje de batería entre 0 y 100.
 */
uint8_t bateria_actualizar(bateria_t *b, uint16_t mv);


/**
 * Obtiene el voltaje actual después de aplicar el filtro.
 *
 * Retorna el voltaje en milivoltios.
 * Si el filtro no está inicializado, retorna cero.
 */
uint16_t bateria_mv_filtrado(const bateria_t *b);


/**
 * Obtiene el porcentaje de batería calculado a partir
 * del voltaje filtrado actual.
 */
uint8_t bateria_pct(const bateria_t *b);


/**
 * Calcula la capacidad restante aproximada de la batería.
 *
 * Utiliza el porcentaje de carga y la capacidad nominal
 * definida para obtener el resultado en mAh.
 */
uint32_t bateria_mah_restantes(uint8_t pct);

#endif /* MODELO_BATERIA_H */
