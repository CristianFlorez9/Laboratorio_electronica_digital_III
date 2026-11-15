
/**
 * @file    ring_buffer.h
 * @brief   Buffer circular de capacidad fija para pasar datos de la ISR al ciclo
 *          principal (productor/consumidor) con detección de pérdidas.
 *
 * @details Justificación de diseño:
 *          - Desacopla los tiempos: la ISR captura datos a la cadencia del
 *            hardware (por ejemplo, muestras del MPU6050), y el ciclo principal
 *            los procesa cuando le toca. Mientras la pantalla se actualiza o se
 *            ejecuta el modelo calórico, las muestras no se pierden.
 *          - Cumple la estrategia polling + interrupciones: la ISR solo escribe
 *            en el buffer (tiempo constante, sin esperas activas) y el main
 *            consulta si hay datos pendientes.
 *          - Preserva la coherencia de los datos compartidos: un único productor
 *            (ISR) modifica el índice de escritura y un único consumidor (main)
 *            modifica el de lectura, por lo que no se necesitan secciones
 *            críticas largas ni bloqueos.
 *          - Permite identificar pérdidas de datos: si el buffer se llena, se
 *            descarta el dato y se incrementa un contador de desbordamientos que
 *            el main puede consultar e informar.
 *          - Evita duplicar la misma lógica en varios drivers: se escribe, se
 *            prueba y se documenta una sola vez, y es reutilizable por el
 *            acelerómetro y por la cola de transacciones del I2C.
 *
 * @note    Contexto: push desde ISR (tiempo acotado); pop y consulta de estado
 *          desde main. Es genérico: no conoce pines ni sensores; el driver que
 *          lo usa define el tamaño y el tipo de dato.
 * @warning Diseñado para un solo productor y un solo consumidor. Si hay más de
 *          uno, requiere protección adicional.
 */