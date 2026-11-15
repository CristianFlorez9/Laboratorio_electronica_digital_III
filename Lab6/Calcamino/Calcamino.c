/**
 * @file    Calcamino.c
 * @brief   Ciclo principal: polling de flags + interrupciones, sin RTOS.
 * @details Inicializa periféricos antes de la sesión y fuera de ISR. En el bucle
 *          consulta el trabajo pendiente (muestras, pasos, batería, pantalla,
 *          estados) y orquesta los módulos. Cuando no hay trabajo, espera una
 *          interrupción sin dejar eventos pendientes. Reporta los contadores de
 *          desbordamiento.
 * @note    No contiene lógica de programa
 * 
 */