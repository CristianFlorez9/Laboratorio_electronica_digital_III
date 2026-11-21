/**
 * @file    state_machine.h
 * @brief   Máquina de estados del dispositivo.
 *
 * Este módulo controla el funcionamiento del dispositivo mediante cinco
 * estados: CONFIG, SESION_ACTIVA, INACTIVO, BAJO_CONSUMO y DESPERTAR_BREVE.
 *
 * La inactividad se mide desde el instante del último paso de la marcha.
 * Un movimiento aislado, un botón o una actualización de pantalla no
 * reinician este tiempo.
 *
 * Si el dispositivo despierta por un movimiento, pero no se confirma una
 * marcha dentro de la ventana de análisis, vuelve directamente a bajo
 * consumo, sin esperar nuevamente los 60 segundos.
 *
 * Los acumulados y la configuración se conservan durante el reposo.
 *
 * Transiciones principales:
 *
 * CONFIG          + BOTON_ACEPTAR       -> INACTIVO
 * SESION_ACTIVA   + MARCHA_FINALIZADA   -> INACTIVO
 * INACTIVO        + MARCHA_CONFIRMADA   -> SESION_ACTIVA
 * INACTIVO        + BOTON_CONFIG        -> CONFIG
 * INACTIVO        + más de 60 s         -> BAJO_CONSUMO
 * BAJO_CONSUMO    + MOVIMIENTO          -> DESPERTAR_BREVE
 * DESPERTAR_BREVE + MARCHA_CONFIRMADA   -> SESION_ACTIVA
 * DESPERTAR_BREVE + vence la ventana    -> BAJO_CONSUMO
 *
 * La máquina de estados no accede directamente al hardware ni utiliza
 * interrupciones. Recibe eventos y actualiza su estado; las acciones
 * generadas son ejecutadas posteriormente por el ciclo principal.
 *
 * Se ejecuta únicamente desde el main. Las interrupciones deben limitarse
 * a guardar datos o activar banderas para que el programa principal genere
 * los eventos correspondientes.
 *
 * Los tiempos se manejan en milisegundos mediante contadores de 32 bits.
 * Las comparaciones se realizan mediante restas, lo que permite manejar
 * correctamente el desbordamiento del contador.
 *
 * El tiempo necesario para entrar en bajo consumo depende de cuánto tarda
 * el detector en declarar el fin de la marcha, los 60 segundos de
 * inactividad y el periodo entre llamadas a sm_actualizar.
 *
 * CONFIG solo se puede alcanzar antes de iniciar una sesión o desde
 * INACTIVO. Al aceptar la configuración se inicia una sesión nueva,
 * reiniciando sus acumulados.
 *
 * La ventana de despertar debe ser suficiente para que el detector confirme
 * una marcha. Los pasos detectados durante ese intervalo deben contarse
 * retroactivamente cuando se confirme la marcha.
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>


/** Tiempo sin caminar para pasar a bajo consumo, medido desde el último paso [ms]. */
#ifndef SM_TIMEOUT_INACTIVIDAD_MS
#define SM_TIMEOUT_INACTIVIDAD_MS   60000u
#endif

/** Tiempo máximo para confirmar una marcha después de despertar [ms]. */
#ifndef SM_VENTANA_DESPERTAR_MS
#define SM_VENTANA_DESPERTAR_MS     5000u
#endif


/* Acciones que debe ejecutar el ciclo principal. */
#define SM_ACCION_NINGUNA            0x00u
#define SM_ACCION_REINICIAR_SESION   0x01u  /**< Reiniciar los pasos y las calorías. */
#define SM_ACCION_PANTALLA_OFF       0x02u  /**< Apagar la pantalla. */
#define SM_ACCION_PANTALLA_ON        0x04u  /**< Encender la pantalla. */


/**
 * Estados posibles del dispositivo.
 */
typedef enum
{
    SM_ESTADO_CONFIG = 0,       /**< Edición de los parámetros personales. */
    SM_ESTADO_SESION_ACTIVA,    /**< Sesión en curso y marcha detectada. */
    SM_ESTADO_INACTIVO,         /**< Sin marcha, pero con la pantalla encendida. */
    SM_ESTADO_BAJO_CONSUMO,     /**< Pantalla apagada y dispositivo en ahorro de energía. */
    SM_ESTADO_DESPERTAR_BREVE   /**< Despertar temporal para analizar un movimiento. */
} sm_estado_t;


/**
 * Eventos que pueden provocar un cambio de estado.
 */
typedef enum
{
    SM_EV_MARCHA_CONFIRMADA = 0, /**< El detector confirmó que hay marcha. */
    SM_EV_MARCHA_FINALIZADA,     /**< Terminó la marcha. El tiempo corresponde al último paso. */
    SM_EV_MOVIMIENTO,            /**< El sensor detectó movimiento. */
    SM_EV_BOTON_CONFIG,          /**< El usuario solicitó editar la configuración. */
    SM_EV_BOTON_ACEPTAR,         /**< El usuario aceptó la configuración. */
    SM_EV_BOTON_REINICIAR        /**< El usuario solicitó reiniciar la sesión. */
} sm_evento_t;


/**
 * Información interna de la máquina de estados.
 * Los campos deben modificarse mediante las funciones del módulo.
 */
typedef struct
{
    sm_estado_t estado;                /**< Estado actual del dispositivo. */
    uint32_t    t_ref_inactividad_ms;  /**< Instante del último paso o referencia de inactividad [ms]. */
    uint32_t    t_inicio_despertar_ms; /**< Instante en que comenzó el despertar breve [ms]. */
    uint32_t    acciones;              /**< Acciones pendientes para el programa principal. */
} sm_t;


/**
 * Inicializa la máquina de estados en CONFIG.
 *
 * El instante recibido se utiliza como referencia inicial de tiempo.
 */
void sm_init(sm_t *sm, uint32_t ahora_ms);


/**
 * Envía un evento a la máquina de estados.
 *
 * El evento solo se procesa si corresponde al estado actual.
 *
 * Para SM_EV_MARCHA_FINALIZADA, el tiempo recibido debe ser el instante
 * del último paso, ya que se utiliza como referencia para medir la
 * inactividad.
 *
 * Los movimientos aislados y los botones no reinician el tiempo
 * de inactividad.
 */
void sm_evento(sm_t *sm, sm_evento_t ev, uint32_t t_ms);


/**
 * Comprueba si se ha cumplido alguno de los tiempos de espera.
 *
 * Debe llamarse periódicamente desde el ciclo principal para controlar
 * la entrada a bajo consumo y el final de la ventana de despertar.
 */
void sm_actualizar(sm_t *sm, uint32_t ahora_ms);


/**
 * Devuelve el estado actual de la máquina.
 *
 * Si el puntero es NULL, retorna SM_ESTADO_CONFIG.
 */
sm_estado_t sm_obtener_estado(const sm_t *sm);


/**
 * Devuelve las acciones pendientes y limpia sus indicadores.
 *
 * Las acciones se entregan como una máscara de bits SM_ACCION_*.
 * Si no hay acciones pendientes, retorna SM_ACCION_NINGUNA.
 */
uint32_t sm_tomar_acciones(sm_t *sm);


/**
 * Calcula cuánto tiempo falta para el próximo vencimiento.
 *
 * Se utiliza para conocer cuánto puede esperar el microcontrolador antes
 * de que sea necesario volver a ejecutar el programa principal.
 *
 * Retorna true si hay un temporizador activo en los estados INACTIVO
 * o DESPERTAR_BREVE. El tiempo restante se guarda en ms.
 *
 * Retorna false si el estado actual no tiene un temporizador activo.
 */
bool sm_ms_hasta_vencimiento(const sm_t *sm, uint32_t ahora_ms, uint32_t *ms);


/**
 * Devuelve el nombre del estado como una cadena de texto.
 *
 * Es útil para mostrar el estado durante las pruebas y la depuración.
 */
const char *sm_nombre_estado(sm_estado_t estado);


#endif /* STATE_MACHINE_H */