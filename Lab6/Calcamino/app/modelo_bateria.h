/**
 * @file    modelo_bateria.h
 * @brief   Modelo de batería basado en el voltaje medido para dos pilas AA en serie.
 *
 * @details Estima el porcentaje de carga de un conjunto de dos baterías AA 
 *          (alcalinas o NiMH en serie, rango aprox. 3.0 V - 1.8 V) mediante una 
 *          curva de descarga, interpolación lineal y un filtro exponencial 
 *          para suavizar las variaciones del voltaje.
 *
 *          El voltaje total debe recibirse en milivoltios (la suma de ambas pilas), 
 *          previamente medido y calibrado mediante el ADC y un divisor de tensión adecuado.
 *
 *          Funcionamiento:
 *          1. Se recibe una lectura del voltaje total de las dos pilas en serie.
 *          2. Se aplica un filtro exponencial para reducir el ruido y las fluctuaciones.
 *          3. Se calcula el porcentaje mediante interpolación lineal entre los 
 *             puntos de la curva de descarga de 2x AA.
 *
 *          Curva de descarga típica (2x AA Alcalinas/NiMH en serie):
 *          
 *            Voltaje Total (mV)    Porcentaje (%)
 *                 3000                  100
 *                 2900                   90
 *                 2800                   80
 *                 2700                   70
 *                 2600                   60
 *                 2500                   50
 *                 2400                   40
 *                 2300                   30
 *                 2200                   20
 *                 2050                   10
 *                 1950                    5
 *                 1800                    0
 *
 *          Consideraciones:
 *          - Por encima de 3000 mV se considera que están al 100 %.
 *          - Por debajo de 1800 mV se considera que están al 0% (agotadas).
 *          - La curva es una aproximación empírica y puede variar según la 
 *            química exacta (Alcalina vs NiMH) y la corriente de consumo.
 */

#ifndef MODELO_BATERIA_H
#define MODELO_BATERIA_H

#include <stdint.h>
#include <stdbool.h>

/* Capacidad nominal típica de una pila AA en mAh (ej. 2000 mAh). */
#define BATERIA_CAPACIDAD_MAH   2000u

/**
 * @brief Divisor utilizado por el filtro exponencial.
 * @details Un valor mayor produce lecturas más suaves, pero una respuesta
 *          más lenta ante los cambios bruscos de voltaje.
 */
#ifndef BATERIA_FILTRO_DIV
#define BATERIA_FILTRO_DIV      16
#endif

/**
 * @brief Almacena el estado del filtro de voltaje de la batería.
 * @details El voltaje se guarda utilizando 4 bits fraccionarios para
 *          conservar mayor precisión durante los cálculos internos (formato Q4).
 */
typedef struct
{
    int32_t mv_q4;          /**< Voltaje filtrado en formato de punto fijo. */
    bool    inicializado;   /**< Indica si ya se recibió una lectura inicial. */
} bateria_t;

/**
 * @brief Calcula el porcentaje de batería a partir del voltaje medido.
 * @param mv Voltaje total medido en milivoltios (2x AA).
 * @return Porcentaje de carga estimado (0 a 100 %).
 */
uint8_t bateria_pct_desde_mv(uint16_t mv);

/**
 * @brief Inicializa el estado del filtro de batería.
 * @param b Puntero a la estructura de la batería.
 */
void bateria_init(bateria_t *b);

/**
 * @brief Actualiza el filtro con una nueva lectura de voltaje.
 * @param b Puntero a la estructura de la batería.
 * @param mv Nueva lectura de voltaje en milivoltios.
 * @return Porcentaje de batería actualizado y suavizado (0 a 100 %).
 */
uint8_t bateria_actualizar(bateria_t *b, uint16_t mv);

/**
 * @brief Obtiene el voltaje actual después de aplicar el filtro.
 * @param b Puntero a la estructura de la batería.
 * @return Voltaje filtrado en milivoltios (retorna 0 si no está inicializado).
 */
uint16_t bateria_mv_filtrado(const bateria_t *b);

/**
 * @brief Obtiene el porcentaje de batería calculado a partir del voltaje filtrado.
 * @param b Puntero a la estructura de la batería.
 * @return Porcentaje de carga actual (0 a 100 %).
 */
uint8_t bateria_pct(const bateria_t *b);

/**
 * @brief Calcula la capacidad restante aproximada de la batería en mAh.
 * @param pct Porcentaje de carga actual (0 a 100 %).
 * @return Capacidad estimada restante en mAh.
 */
uint32_t bateria_mah_restantes(uint8_t pct);

#endif /* MODELO_BATERIA_H */