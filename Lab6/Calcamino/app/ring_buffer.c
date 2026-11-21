
/**
 * @file ring_buffer.c
 * @brief Implementación del buffer circular.
 *
 * Implementa una estructura de almacenamiento temporal que permite
 * guardar y recuperar datos siguiendo el orden en que fueron recibidos.
 *
 * El buffer utiliza dos índices:
 * - head: posición donde se almacena el siguiente dato.
 * - tail: posición desde donde se recupera el siguiente dato.
 *
 * Funcionamiento:
 * - El buffer está vacío cuando head y tail son iguales.
 * - El buffer está lleno cuando la siguiente posición de head
 *   coincide con tail.
 * - La escritura modifica únicamente head.
 * - La lectura modifica únicamente tail.
 *
 * Se reserva una posición libre para diferenciar entre los estados
 * de buffer vacío y buffer lleno.
 */

#include "ring_buffer.h"
#include <stddef.h>


/**
 * Calcula la siguiente posición de un índice dentro del buffer.
 *
 * Cuando se alcanza el final de la memoria disponible,
 * el índice vuelve a la primera posición.
 */
static inline uint32_t siguiente_indice(uint32_t indice, uint32_t capacidad)
{
    indice++;

    if (indice == capacidad)
    {
        indice = 0;
    }

    return indice;
}


/**
 * Inicializa el buffer circular.
 *
 * Configura la memoria disponible, la capacidad, el tamaño
 * de cada elemento y los índices de lectura y escritura.
 *
 * Retorna true si la inicialización fue correcta.
 */
bool ring_buffer_init(ring_buffer_t *rb,
                      uint8_t *memoria,
                      uint32_t capacidad,
                      uint32_t tam_elemento)
{
    /* Verificar que los parámetros sean válidos. */
    if ((rb == NULL) || (memoria == NULL) ||
        (capacidad < 2u) || (tam_elemento == 0u))
    {
        return false;
    }

    /* Guardar la configuración del buffer. */
    rb->datos        = memoria;
    rb->capacidad    = capacidad;
    rb->tam_elemento = tam_elemento;

    /* Inicializar los índices y el contador de desbordamientos. */
    rb->head            = 0u;
    rb->tail            = 0u;
    rb->desbordamientos = 0u;

    return true;
}


/**
 * Agrega un elemento al buffer circular.
 *
 * Copia los datos en la posición indicada por head y actualiza
 * el índice de escritura cuando termina la operación.
 *
 * Si el buffer está lleno, no almacena el elemento y aumenta
 * el contador de desbordamientos.
 *
 * Retorna true si el elemento fue almacenado correctamente.
 */
bool ring_buffer_push(ring_buffer_t *rb, const void *dato)
{
    const uint8_t *origen = (const uint8_t *)dato;
    uint32_t       siguiente;
    uint32_t       inicio;
    uint32_t       i;

    if ((rb == NULL) || (dato == NULL))
    {
        return false;
    }

    /* Calcular la siguiente posición de escritura. */
    siguiente = siguiente_indice(rb->head, rb->capacidad);

    /* Comprobar si el buffer está lleno. */
    if (siguiente == rb->tail)
    {
        rb->desbordamientos++;
        return false;
    }

    /* Copiar el elemento en la posición actual de escritura. */
    inicio = rb->head * rb->tam_elemento;

    for (i = 0u; i < rb->tam_elemento; i++)
    {
        rb->datos[inicio + i] = origen[i];
    }

    /* Actualizar el índice después de completar la escritura. */
    rb->head = siguiente;

    return true;
}


/**
 * Recupera un elemento del buffer circular.
 *
 * Copia los datos desde la posición indicada por tail y actualiza
 * el índice de lectura después de completar la operación.
 *
 * Si el buffer está vacío, no realiza ninguna lectura.
 *
 * Retorna true si se recuperó un elemento correctamente.
 */
bool ring_buffer_pop(ring_buffer_t *rb, void *dato)
{
    uint8_t *destino = (uint8_t *)dato;
    uint32_t inicio;
    uint32_t i;

    if ((rb == NULL) || (dato == NULL))
    {
        return false;
    }

    /* Comprobar si el buffer está vacío. */
    if (rb->head == rb->tail)
    {
        return false;
    }

    /* Copiar el elemento desde la posición actual de lectura. */
    inicio = rb->tail * rb->tam_elemento;

    for (i = 0u; i < rb->tam_elemento; i++)
    {
        destino[i] = rb->datos[inicio + i];
    }

    /* Actualizar el índice después de completar la lectura. */
    rb->tail = siguiente_indice(rb->tail, rb->capacidad);

    return true;
}


/**
 * Obtiene la cantidad de elementos almacenados en el buffer.
 *
 * Tiene en cuenta la posibilidad de que los índices hayan
 * dado una vuelta completa por la memoria disponible.
 */
uint32_t ring_buffer_count(const ring_buffer_t *rb)
{
    uint32_t head;
    uint32_t tail;

    if (rb == NULL)
    {
        return 0u;
    }

    head = rb->head;
    tail = rb->tail;

    if (head >= tail)
    {
        return head - tail;
    }

    return rb->capacidad - tail + head;
}


/**
 * Comprueba si el buffer circular está vacío.
 */
bool ring_buffer_empty(const ring_buffer_t *rb)
{
    if (rb == NULL)
    {
        return true;
    }

    return (rb->head == rb->tail);
}


/**
 * Comprueba si el buffer circular está lleno.
 */
bool ring_buffer_full(const ring_buffer_t *rb)
{
    if (rb == NULL)
    {
        return false;
    }

    return (siguiente_indice(rb->head, rb->capacidad) == rb->tail);
}


/**
 * Obtiene la cantidad de elementos que no pudieron almacenarse
 * debido a que el buffer estaba lleno.
 */
uint32_t ring_buffer_overflows(const ring_buffer_t *rb)
{
    if (rb == NULL)
    {
        return 0u;
    }

    return rb->desbordamientos;
}


/**
 * Vacía el buffer circular y reinicia sus índices.
 *
 * También reinicia el contador de desbordamientos.
 *
 * Esta operación debe realizarse con la interrupción encargada
 * de producir datos deshabilitada.
 */
void ring_buffer_clear(ring_buffer_t *rb)
{
    if (rb == NULL)
    {
        return;
    }

    rb->head            = 0u;
    rb->tail            = 0u;
    rb->desbordamientos = 0u;
}
