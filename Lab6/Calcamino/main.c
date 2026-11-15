/**
 * @file    main.c
 * @brief   Ciclo principal: polling de flags + interrupciones, sin RTOS.
 * @details Inicializa periféricos antes de la sesión y fuera de ISR. En el bucle
 *          consulta el trabajo pendiente (muestras, pasos, batería, pantalla,
 *          estados) y orquesta los módulos. Cuando no hay trabajo, espera una
 *          interrupción sin dejar eventos pendientes. Reporta los contadores de
 *          desbordamiento.
 * @note    No contiene lógica de negocio.
 * @par Requisitos: REQ-2, REQ-38, REQ-39, REQ-42, REQ-44
 */