/**
 * @file    interfaz_grafica.c
 * @brief   Implementación de las pantallas de configuración y sesión en OLED.
 * @details Gestiona la composición gráfica basada en texto para pantallas monocromáticas,
 *          organizando los elementos mediante coordenadas de píxeles (X, Y) y utilizando
 *          un búfer intermedio antes de disparar la actualización asíncrona (`oled_show`).
 */

#include "interfaz_grafica.h"
#include "drivers/pantalla_oled.h"
#include <stdio.h>

/**
 * @brief Renderiza y envía a la pantalla la interfaz de configuración de parámetros personales.
 * 
 * @details Dibuja el menú interactivo para ajustar el perfil físico del usuario. Utiliza 
 *          un búfer temporal local (`char buffer[32]`) para formatear numéricamente los 
 *          valores y posicionarlos en coordenadas fijas de la matriz OLED.
 *          
 *          Estructura visual de la pantalla (Coordenadas Y en píxeles):
 *          - Y = 0:  Título "CONFIG USUARIO" (izq.) | Porcentaje de batería "Bat:XX%" (der.)
 *          - Y = 16: Campo de Peso (Muestra el cursor '>' si `campo == 0`)
 *          - Y = 28: Campo de Estatura (Muestra el cursor '>' si `campo == 1`)
 *          - Y = 40: Gasto calórico acumulado actual
 * 
 * @param kcal    Gasto calórico acumulado actual [kcal].
 * @param peso    Masa corporal configurada actualmente [kg].
 * @param altura  Estatura configurada actualmente [cm].
 * @param bateria Porcentaje de batería restante del dispositivo [0 a 100%].
 * @param campo   Selector de campo activo en edición: 
 *                - 0u: El cursor '>' apunta al peso.
 *                - 1u: El cursor '>' apunta a la estatura.
 */
void interfaz_cnfg(uint32_t kcal, uint32_t peso, uint32_t altura, uint32_t bateria, uint8_t campo) {
    char buffer[32]; // Búfer auxiliar para la conversión de texto

    // 1. Limpia el framebuffer local de la pantalla
    oled_clear();

    // 2. Cabecera superior: Título de la sección (X=0, Y=0)
    snprintf(buffer, sizeof(buffer), "CONFIG USUARIO");
    oled_escribir_texto(0, 0, buffer);

    // 3. Cabecera superior: Indicador de batería alineado a la derecha (X=86, Y=0)
    snprintf(buffer, sizeof(buffer), "Bat:%lu%%", (unsigned long)bateria);
    oled_escribir_texto(86, 0, buffer);

    // 4. Parámetro de Peso con indicador de foco dinámico (X=0, Y=16)
    //    Si `campo == 0`, coloca un cursor '>'; de lo contrario, deja un espacio en blanco.
    snprintf(buffer, sizeof(buffer), "%cPeso: %lu kg",
             (campo == 0u) ? '>' : ' ', (unsigned long)peso);
    oled_escribir_texto(0, 16, buffer);

    // 5. Parámetro de Estatura con indicador de foco dinámico (X=0, Y=28)
    //    Si `campo == 1`, coloca un cursor '>'; de lo contrario, deja un espacio en blanco.
    snprintf(buffer, sizeof(buffer), "%cEstatura: %lu cm",
             (campo == 1u) ? '>' : ' ', (unsigned long)altura);
    oled_escribir_texto(0, 28, buffer);

    // 6. Registro de calorías acumuladas estáticas en el perfil (X=0, Y=40)
    snprintf(buffer, sizeof(buffer), " Kcal: %lu kcal", (unsigned long)kcal);
    oled_escribir_texto(0, 40, buffer);

    // 7. Envía el búfer completo a la pantalla física sin bloquear la CPU
    oled_show();
}

/**
 * @brief Renderiza y envía a la pantalla la interfaz principal de la sesión de caminata/ejercicio.
 * 
 * @details Compone el tablero de telemetría en tiempo real para el usuario mientras realiza
 *          actividad física. Muestra métricas de rendimiento y estado del sistema.
 *          
 *          Estructura visual de la pantalla (Coordenadas Y en píxeles):
 *          - Y = 0:  Estado operativo de la máquina de estados "Est:X" | Batería "Bat:XX%"
 *          - Y = 16: Conteo total de pasos acumulados
 *          - Y = 28: Calorías quemadas estimadas [kcal]
 *          - Y = 40: Cadencia de marcha actual en pasos por minuto [SPM]
 * 
 * @param pasos   Número acumulado de pasos válidos en la sesión actual.
 * @param estado  Identificador numérico o código de estado interno de la máquina de estados.
 * @param cadence Cadencia actual calculada en pasos por minuto [SPM].
 * @param kcal    Gasto calórico estimado acumulado durante la sesión [kcal].
 * @param bateria Porcentaje de batería restante del dispositivo [0 a 100%].
 */
void interfaz_sesion(uint32_t pasos, float estado, int cadence, uint32_t kcal, uint32_t bateria) {
    char buffer[32]; // Búfer auxiliar para la conversión de texto

    // 1. Limpia el framebuffer local de la pantalla
    oled_clear();

    // 2. Cabecera superior: Identificador del estado operativo del sistema (X=0, Y=0)
    snprintf(buffer, sizeof(buffer), "Est:%d", (int)estado);
    oled_escribir_texto(0, 0, buffer);

    // 3. Cabecera superior: Nivel de batería alineado a la derecha (X=86, Y=0)
    snprintf(buffer, sizeof(buffer), "Bat:%lu%%", (unsigned long)bateria);
    oled_escribir_texto(86, 0, buffer);

    // 4. Métrica principal: Total de pasos de la sesión (X=0, Y=16)
    snprintf(buffer, sizeof(buffer), "Pasos: %lu", (unsigned long)pasos);
    oled_escribir_texto(0, 16, buffer);

    // 5. Métrica energética: Calorías quemadas en tiempo real (X=0, Y=28)
    snprintf(buffer, sizeof(buffer), "Kcal: %lu kcal", (unsigned long)kcal);
    oled_escribir_texto(0, 28, buffer);

    // 6. Métrica de ritmo: Cadencia actual en Pasos Por Minuto (X=0, Y=40)
    snprintf(buffer, sizeof(buffer), "Cadencia: %d SPM", cadence);
    oled_escribir_texto(0, 40, buffer);

    // 7. Envía el búfer completo a la pantalla física sin bloquear la CPU
    oled_show();
}