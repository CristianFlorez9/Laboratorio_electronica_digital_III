
/**
 * @file    state_machine.h
 * @brief   Máquina de estados del dispositivo.
 * @details Estados: CONFIG, SESION_ACTIVA, INACTIVO, BAJO_CONSUMO y
 *          DESPERTAR_BREVE. La inactividad se mide desde el final de la marcha
 *          detectada. Un movimiento aislado, un botón o un refresco de pantalla
 *          no la reinician. Si el despertar breve no confirma marcha, vuelve a
 *          bajo consumo sin nueva espera de 60 s. Conserva acumulados y
 *          configuración durante el reposo.
 * 
 */