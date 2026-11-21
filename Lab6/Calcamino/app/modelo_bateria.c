
/**
 * @file modelo_bateria.c
 * @brief Implementación del modelo de batería.
 *
 * Convierte el voltaje de la batería en un porcentaje de carga
 * mediante una curva de descarga y una interpolación lineal.
 *
 * También incluye un filtro para suavizar las variaciones de
 * voltaje y una función para calcular la capacidad restante.
 */

#include "modelo_bateria.h"
#include <stddef.h>

/**
 * Representa un punto de la curva de descarga de la batería.
 */
typedef struct
{
    uint16_t mv;    /**< Voltaje en milivoltios. */
    uint8_t  pct;   /**< Porcentaje de carga. */
} punto_curva_t;

/**
 * Tabla de descarga de una batería LiPo.
 * Los puntos están ordenados de mayor a menor voltaje.
 */
static const punto_curva_t CURVA[] =
{
    { 4200u, 100u },
    { 4110u,  90u },
    { 4020u,  80u },
    { 3950u,  70u },
    { 3870u,  60u },
    { 3840u,  50u },
    { 3800u,  40u },
    { 3770u,  30u },
    { 3730u,  20u },
    { 3690u,  10u },
    { 3610u,   5u },
    { 3300u,   0u }
};

/* Cantidad de puntos de la curva de descarga. */
#define CURVA_N   (sizeof(CURVA) / sizeof(CURVA[0]))


/**
 * Convierte un voltaje en milivoltios a un porcentaje de batería.
 *
 * Utiliza interpolación lineal entre los puntos de la curva
 * de descarga. Los valores fuera de la curva se limitan al
 * porcentaje mínimo o máximo correspondiente.
 */
uint8_t bateria_pct_desde_mv(uint16_t mv)
{
    uint32_t i;
    int32_t  dv;
    int32_t  dp;
    int32_t  num;
    int32_t  den;

    /* Limitar el resultado al rango de la curva. */
    if (mv >= CURVA[0].mv)
    {
        return CURVA[0].pct;
    }

    if (mv <= CURVA[CURVA_N - 1u].mv)
    {
        return CURVA[CURVA_N - 1u].pct;
    }

    /* Buscar el intervalo al que pertenece el voltaje medido. */
    for (i = 1u; i < CURVA_N; i++)
    {
        if (mv >= CURVA[i].mv)
        {
            break;
        }
    }

    /* Calcular el porcentaje mediante interpolación lineal. */
    dv  = (int32_t)CURVA[i - 1u].mv  - (int32_t)CURVA[i].mv;
    dp  = (int32_t)CURVA[i - 1u].pct - (int32_t)CURVA[i].pct;
    num = ((int32_t)mv - (int32_t)CURVA[i].mv) * dp;
    den = dv;

    return (uint8_t)((int32_t)CURVA[i].pct + ((num + (den / 2)) / den));
}


/**
 * Inicializa la estructura de control de la batería.
 *
 * Establece el voltaje filtrado en cero y marca el sistema
 * como no inicializado.
 */
void bateria_init(bateria_t *b)
{
    if (b == NULL)
    {
        return;
    }

    b->mv_q4       = 0;
    b->inicializado = false;
}


/**
 * Actualiza el modelo con una nueva lectura de voltaje.
 *
 * La primera lectura se utiliza como valor inicial del filtro.
 * Las siguientes lecturas se suavizan para reducir las variaciones.
 *
 * Retorna el porcentaje de batería calculado.
 */
uint8_t bateria_actualizar(bateria_t *b, uint16_t mv)
{
    int32_t muestra_q4;

    if (b == NULL)
    {
        return 0u;
    }

    muestra_q4 = (int32_t)mv * 16;

    if (!b->inicializado)
    {
        /* Utilizar la primera lectura como valor inicial. */
        b->mv_q4        = muestra_q4;
        b->inicializado = true;
    }
    else
    {
        /* Aplicar el filtro para suavizar las lecturas. */
        b->mv_q4 += (muestra_q4 - b->mv_q4) / BATERIA_FILTRO_DIV;
    }

    return bateria_pct_desde_mv(bateria_mv_filtrado(b));
}


/**
 * Obtiene el voltaje actual después de aplicar el filtro.
 *
 * Retorna el voltaje en milivoltios.
 * Si la estructura no está inicializada, retorna cero.
 */
uint16_t bateria_mv_filtrado(const bateria_t *b)
{
    if ((b == NULL) || !b->inicializado)
    {
        return 0u;
    }

    /* Convertir el valor interno al voltaje en milivoltios. */
    return (uint16_t)((b->mv_q4 + 8) / 16);
}


/**
 * Obtiene el porcentaje de carga actual de la batería.
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
 * Calcula la capacidad restante de la batería en mAh.
 *
 * El cálculo se realiza a partir del porcentaje de carga
 * y de la capacidad total definida en el archivo de cabecera.
 */
uint32_t bateria_mah_restantes(uint8_t pct)
{
    if (pct > 100u)
    {
        pct = 100u;
    }

    return ((uint32_t)pct * BATERIA_CAPACIDAD_MAH) / 100u;
}
