/**
 * @file    modelo_bateria.c
 * @brief   Implementación del modelo de batería para dos pilas AA en serie.
 *
 * @details Convierte el voltaje total de un conjunto de dos pilas AA 
 *          (alcalinas o NiMH en serie, rango aprox. 3.1 V - 1.8 V) en un porcentaje 
 *          de carga mediante una curva de descarga y una interpolación lineal.
 *
 *          También incluye un filtro exponencial para suavizar las variaciones de
 *          voltaje y una función para calcular la capacidad restante en mAh.
 */

#include "modelo_bateria.h"
#include <stddef.h>

/**
 * @brief Representa un punto de la curva de descarga de la batería.
 */
typedef struct
{
    uint16_t mv;    /**< Voltaje total en milivoltios (2x AA). */
    uint8_t  pct;   /**< Porcentaje de carga correspondiente. */
} punto_curva_t;

/**
 * @brief Tabla de descarga para dos pilas AA en serie (Alcalinas / NiMH).
 * @details Los puntos están ordenados estrictamente de mayor a menor voltaje.
 */
static const punto_curva_t CURVA[] =
{
    { 3100u, 100u },  /* Pilas completamente nuevas / recién colocadas */
    { 3000u,  90u },
    { 2900u,  80u },
    { 2800u,  70u },
    { 2700u,  60u },
    { 2600u,  50u },  /* Zona media de descarga estable */
    { 2500u,  40u },
    { 2400u,  30u },
    { 2300u,  20u },
    { 2150u,  10u },  /* Comienza la caída pronunciada */
    { 2000u,   5u },  /* Zona crítica */
    { 1800u,   0u }   /* Pilas agotadas (0.9V por celda) */
};

/* Cantidad de puntos de la curva de descarga calculada automáticamente. */
#define CURVA_N   (sizeof(CURVA) / sizeof(CURVA[0]))

/**
 * @brief Convierte un voltaje en milivoltios a un porcentaje de batería.
 * @details Utiliza interpolación lineal entre los puntos de la curva de descarga.
 *          Los valores fuera de rango se saturan al 0% o al 100%.
 */
uint8_t bateria_pct_desde_mv(uint16_t mv) {
    
    uint32_t i;   // Índice para recorrer la tabla de la curva de descarga
    int32_t  dv;  // Diferencia de voltaje entre los dos puntos de la tabla (Delta V)
    int32_t  dp;  // Diferencia de porcentaje entre los dos puntos de la tabla (Delta P)
    int32_t  num; // Numerador para el cálculo proporcional de la interpolación lineal
    int32_t  den; // Denominador para el cálculo de la interpolación (ancho del intervalo)

    /* Si el voltaje supera el máximo, se limita al 100 %. */
    if (mv >= CURVA[0].mv)
    {
        return CURVA[0].pct;
    }

    /* Si el voltaje está por debajo del mínimo, se limita al 0 %. */
    if (mv <= CURVA[CURVA_N - 1u].mv)
    {
        return CURVA[CURVA_N - 1u].pct;
    }

    /* Buscar el intervalo exacto al que pertenece el voltaje medido. */
    for (i = 1u; i < CURVA_N; i++)
    {
        if (mv >= CURVA[i].mv)
        {
            break;
        }
    }

    /* Calcular el porcentaje exacto mediante interpolación lineal. */
    dv  = (int32_t)CURVA[i - 1u].mv  - (int32_t)CURVA[i].mv;
    dp  = (int32_t)CURVA[i - 1u].pct - (int32_t)CURVA[i].pct;
    num = ((int32_t)mv - (int32_t)CURVA[i].mv) * dp;
    den = dv;

    return (uint8_t)((int32_t)CURVA[i].pct + ((num + (den / 2)) / den));
}

/**
 * @brief Inicializa la estructura de control de la batería.
 */
void bateria_init(bateria_t *b)
{
    if (b == NULL)
    {
        return;
    }

    b->mv_q4        = 0;
    b->inicializado = false;
}

/**
 * @brief Actualiza el modelo con una nueva lectura de voltaje.
 * @details Aplica el filtro exponencial en formato Q4 para eliminar ruido del ADC.
 */
uint8_t bateria_actualizar(bateria_t *b, uint16_t mv)
{
    int32_t muestra_q4;

    if (b == NULL)
    {
        return 0u;
    }

    /* Convertir la lectura a formato de punto fijo Q4 (multiplicar por 16) */
    muestra_q4 = (int32_t)mv * 16;

    if (!b->inicializado)
    {
        /* Utilizar la primera lectura como valor inicial inmediato del filtro. */
        b->mv_q4        = muestra_q4;
        b->inicializado = true;
    }
    else
    {
        /* Aplicar el filtro exponencial para suavizar las fluctuaciones. */
        b->mv_q4 += (muestra_q4 - b->mv_q4) / BATERIA_FILTRO_DIV;
    }

    return bateria_pct_desde_mv(bateria_mv_filtrado(b));
}

/**
 * @brief Obtiene el voltaje actual después de aplicar el filtro (en mV).
 */
uint16_t bateria_mv_filtrado(const bateria_t *b)
{
    if ((b == NULL) || !b->inicializado)
    {
        return 0u;
    }

    /* Convertir de formato Q4 de regreso a milivoltios enteros (redondeando). */
    return (uint16_t)((b->mv_q4 + 8) / 16);
}

/**
 * @brief Obtiene el porcentaje de carga actual de la batería.
 */
uint8_t bateria_pct(const bateria_t *b)
{
    if ((b == NULL) || !b->inicializado)
    {
        return 0u;
    }

    return bateria_pct_desde_mv(bateria_mv_filtrado(b));
}

/**
 * @brief Calcula la capacidad restante de la batería en mAh.
 * @details Utiliza la macro BATERIA_CAPACIDAD_MAH definida en el archivo .h (ej. 2000 mAh).
 */
uint32_t bateria_mah_restantes(uint8_t pct)
{
    if (pct > 100u)
    {
        pct = 100u;
    }

    return ((uint32_t)pct * BATERIA_CAPACIDAD_MAH) / 100u;
}