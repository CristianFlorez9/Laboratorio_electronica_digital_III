/**
 * @file    ring_buffer.h
 * @brief   Buffer circular de capacidad fija para compartir datos entre una
 *          interrupción y el programa principal.
 *
 * Este módulo permite almacenar temporalmente datos que llegan desde una ISR
 * para que el programa principal pueda procesarlos después.
 *
 * La interrupción funciona como productor: introduce datos en el buffer sin
 * esperar a que sean procesados. El programa principal funciona como
 * consumidor: extrae los datos pendientes cuando puede atenderlos.
 *
 * Si el buffer se llena, el nuevo dato se descarta y se incrementa un contador
 * de desbordamientos. Así es posible detectar cuándo se han perdido datos.
 *
 * El módulo está pensado para trabajar con un solo productor y un solo
 * consumidor, ejecutándose en el mismo núcleo. Si se utilizan varios
 * productores, consumidores o el segundo núcleo del RP2040, se necesita
 * protección adicional.
 *
 * Se reserva un espacio libre para diferenciar entre buffer lleno y vacío.
 * Por esta razón, la cantidad máxima de elementos almacenados es
 * capacidad - 1. Por ejemplo, para guardar 16 elementos se necesitan
 * 17 espacios.
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>


/**
 * Estructura que contiene la información necesaria para manejar el buffer.
 * Los campos se inicializan mediante ring_buffer_init y no deben modificarse
 * directamente.
 */
typedef struct
{
    uint8_t *datos;                     /**< Memoria donde se almacenan los datos. */

    uint32_t capacidad;                 /**< Número total de espacios disponibles. */
    uint32_t tam_elemento;              /**< Tamaño de cada elemento en bytes. */

    volatile uint32_t head;             /**< Posición de escritura, modificada por la ISR. */
    volatile uint32_t tail;             /**< Posición de lectura, modificada por el main. */
    volatile uint32_t desbordamientos;  /**< Cantidad de datos descartados por buffer lleno. */

} ring_buffer_t;


/**
 * Inicializa el buffer circular con la memoria y el tamaño de elemento
 * proporcionados.
 *
 * La memoria debe tener espacio para capacidad elementos, cada uno de
 * tam_elemento bytes.
 *
 * Debe llamarse antes de habilitar la interrupción que introduce los datos.
 *
 * Retorna true si la inicialización fue correcta o false si algún parámetro
 * es inválido.
 */
bool ring_buffer_init(ring_buffer_t *rb,
                      uint8_t *memoria,
                      uint32_t capacidad,
                      uint32_t tam_elemento);


/**
 * Copia un elemento al buffer circular.
 *
 * Si el buffer está lleno, el dato se descarta y se incrementa el contador
 * de desbordamientos.
 *
 * Está pensada para ejecutarse desde la ISR como única función productora.
 * Su ejecución tiene un tiempo acotado y no utiliza esperas activas.
 *
 * Retorna true si el dato fue almacenado o false si no fue posible hacerlo.
 */
bool ring_buffer_push(ring_buffer_t *rb, const void *dato);


/**
 * Extrae el elemento más antiguo del buffer y lo copia en la dirección
 * indicada.
 *
 * Está pensada para ejecutarse desde el ciclo principal, como única función
 * consumidora.
 *
 * Retorna true si se extrajo un elemento o false si el buffer está vacío
 * o algún parámetro es inválido.
 */
bool ring_buffer_pop(ring_buffer_t *rb, void *dato);


/**
 * Consulta cuántos elementos hay almacenados actualmente.
 *
 * Retorna 0 si el buffer no es válido.
 */
uint32_t ring_buffer_count(const ring_buffer_t *rb);


/**
 * Comprueba si el buffer está vacío.
 *
 * Retorna true si no hay elementos disponibles o si el buffer no es válido.
 */
bool ring_buffer_empty(const ring_buffer_t *rb);


/**
 * Comprueba si el buffer está lleno.
 *
 * Retorna true si no hay espacio para otro elemento.
 */
bool ring_buffer_full(const ring_buffer_t *rb);


/**
 * Consulta cuántos datos se han descartado porque el buffer estaba lleno.
 *
 * El contador se reinicia al inicializar o limpiar el buffer.
 */
uint32_t ring_buffer_overflows(const ring_buffer_t *rb);


/**
 * Vacía el buffer y reinicia el contador de desbordamientos.
 *
 * Como modifica tanto la posición de escritura como la de lectura, debe
 * llamarse únicamente cuando la ISR productora esté deshabilitada.
 */
void ring_buffer_clear(ring_buffer_t *rb);


#endif /* RING_BUFFER_H */