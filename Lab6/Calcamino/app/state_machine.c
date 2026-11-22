/**
 * @file    state_machine.c
 * @brief   Implementación de la máquina de estados del dispositivo.
 *
 * Este módulo controla los cambios entre los diferentes estados del
 * dispositivo. No tiene acceso directo al hardware: recibe eventos,
 * actualiza el estado y genera acciones que ejecutará el programa principal.
 *
 * La definición de los estados y sus transiciones se encuentra en
 * state_machine.h.
 */

#include "state_machine.h"
#include <stddef.h>


/**
 * Reinicia los datos de la sesión y deja el dispositivo en estado INACTIVO.
 *
 * También actualiza la referencia de tiempo que se utiliza para medir
 * cuánto lleva el dispositivo sin detectar actividad.
 */
static void iniciar_sesion(sm_t *sm, uint32_t t_ms)
{
    sm->estado               = SM_ESTADO_INACTIVO;
    sm->t_ref_inactividad_ms = t_ms;
    sm->acciones            |= SM_ACCION_REINICIAR_SESION;
}


void sm_init(sm_t *sm, uint32_t ahora_ms)
{
    if (sm == NULL)
    {
        return;
    }

    sm->estado                = SM_ESTADO_CONFIG;
    sm->t_ref_inactividad_ms  = ahora_ms;
    sm->t_inicio_despertar_ms = ahora_ms;
    sm->acciones              = SM_ACCION_NINGUNA;
}


void sm_evento(sm_t *sm, sm_evento_t ev, uint32_t t_ms)
{
    if (sm == NULL)
    {
        return;
    }

    switch (sm->estado)
    {
        case SM_ESTADO_CONFIG:
            if (ev == SM_EV_BOTON_ACEPTAR)
            {
                iniciar_sesion(sm, t_ms);
            }
            break;

        case SM_ESTADO_SESION_ACTIVA:
            if (ev == SM_EV_MARCHA_FINALIZADA)
            {
                /* Se toma como referencia el instante del último paso. */
                sm->estado               = SM_ESTADO_INACTIVO;
                sm->t_ref_inactividad_ms = t_ms;
            }
            else if (ev == SM_EV_BOTON_CONFIG)
            {
                /* Pulsación larga de ENTER: volver a editar los parámetros. */
                sm->estado = SM_ESTADO_CONFIG;
            }
            else if (ev == SM_EV_BOTON_REINICIAR)
            {
                sm->acciones |= SM_ACCION_REINICIAR_SESION;
            }
            break;

        case SM_ESTADO_INACTIVO:
            if (ev == SM_EV_MARCHA_CONFIRMADA)
            {
                sm->estado = SM_ESTADO_SESION_ACTIVA;
            }
            else if (ev == SM_EV_BOTON_CONFIG)
            {
                sm->estado = SM_ESTADO_CONFIG;
            }
            else if (ev == SM_EV_BOTON_REINICIAR)
            {
                /* Reinicia los acumulados, pero conserva el tiempo de inactividad. */
                sm->acciones |= SM_ACCION_REINICIAR_SESION;
            }
            break;

        case SM_ESTADO_BAJO_CONSUMO:
            if (ev == SM_EV_MOVIMIENTO)
            {
                sm->estado                = SM_ESTADO_DESPERTAR_BREVE;
                sm->t_inicio_despertar_ms = t_ms;
            }
            break;

        case SM_ESTADO_DESPERTAR_BREVE:
            if (ev == SM_EV_MARCHA_CONFIRMADA)
            {
                sm->estado    = SM_ESTADO_SESION_ACTIVA;
                sm->acciones |= SM_ACCION_PANTALLA_ON;
            }

            /* Los movimientos adicionales no reinician el tiempo de análisis. */
            break;

        default:
            break;
    }
}


void sm_actualizar(sm_t *sm, uint32_t ahora_ms)
{
    if (sm == NULL)
    {
        return;
    }

    switch (sm->estado)
    {
        case SM_ESTADO_INACTIVO:
            /*
             * La resta entre valores sin signo permite medir el tiempo
             * correctamente incluso cuando el contador de milisegundos
             * vuelve a cero por desbordamiento.
             */
            if ((ahora_ms - sm->t_ref_inactividad_ms) > SM_TIMEOUT_INACTIVIDAD_MS)
            {
                sm->estado    = SM_ESTADO_BAJO_CONSUMO;
                sm->acciones |= SM_ACCION_PANTALLA_OFF;
            }
            break;

        case SM_ESTADO_DESPERTAR_BREVE:
            /*
             * Si no se confirma una marcha dentro de esta ventana,
             * el dispositivo vuelve al estado de bajo consumo.
             */
            if ((ahora_ms - sm->t_inicio_despertar_ms) > SM_VENTANA_DESPERTAR_MS)
            {
                sm->estado = SM_ESTADO_BAJO_CONSUMO;
            }
            break;

        default:
            break;
    }
}


sm_estado_t sm_obtener_estado(const sm_t *sm)
{
    if (sm == NULL)
    {
        return SM_ESTADO_CONFIG;
    }

    return sm->estado;
}


uint32_t sm_tomar_acciones(sm_t *sm)
{
    uint32_t acciones;

    if (sm == NULL)
    {
        return SM_ACCION_NINGUNA;
    }

    /* Entrega las acciones pendientes y limpia sus indicadores. */
    acciones     = sm->acciones;
    sm->acciones = SM_ACCION_NINGUNA;

    return acciones;
}


bool sm_ms_hasta_vencimiento(const sm_t *sm, uint32_t ahora_ms, uint32_t *ms)
{
    uint32_t transcurrido;
    uint32_t limite;

    if ((sm == NULL) || (ms == NULL))
    {
        return false;
    }

    if (sm->estado == SM_ESTADO_INACTIVO)
    {
        transcurrido = ahora_ms - sm->t_ref_inactividad_ms;
        limite       = SM_TIMEOUT_INACTIVIDAD_MS;
    }
    else if (sm->estado == SM_ESTADO_DESPERTAR_BREVE)
    {
        transcurrido = ahora_ms - sm->t_inicio_despertar_ms;
        limite       = SM_VENTANA_DESPERTAR_MS;
    }
    else
    {
        return false;
    }

    /*
     * El cambio de estado ocurre cuando el tiempo transcurrido es mayor
     * que el límite, no cuando es igual. Por eso se suma 1 ms.
     */
    *ms = (transcurrido > limite) ? 0u : (limite - transcurrido + 1u);

    return true;
}


const char *sm_nombre_estado(sm_estado_t estado)
{
    switch (estado)
    {
        case SM_ESTADO_CONFIG:          return "CONFIG";
        case SM_ESTADO_SESION_ACTIVA:   return "SESION_ACTIVA";
        case SM_ESTADO_INACTIVO:        return "INACTIVO";
        case SM_ESTADO_BAJO_CONSUMO:    return "BAJO_CONSUMO";
        case SM_ESTADO_DESPERTAR_BREVE: return "DESPERTAR_BREVE";
        default:                        return "DESCONOCIDO";
    }
}