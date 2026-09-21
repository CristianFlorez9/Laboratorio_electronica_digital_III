"""
@package Juego
@brief Lógica central, máquina de estados y reglas del juego.

@details
Este módulo gestiona la lógica interna del juego, controlando los estados 
de la partida (inicio, presentación, espera de inputs, éxito, fallo y fin), 
la generación de la secuencia aleatoria, el sistema de vidas, niveles y el 
cálculo de tiempos en cada etapa.

@date 2026-09-13
"""
import random
import time
from micropython import const

## @biref Estados posibles en el juego
## @brief Estado de reposo o espera a que se presione Start.
INICIO = const(0)
## @brief Muestra la secuencia aleatoria de LEDs correspondiente al nivel actual.
PRESENTACION = const(1)
## @brief Espera y valida la respuesta del usuario a través de los botones.
INPUTS = const(2)
## @brief Pausa de éxito tras replicar correctamente la secuencia actual.
NIVEL_COMPLETO = const(3)
## @brief Pausa de fallo cuando el usuario se equivoca o se agota el tiempo.
NIVEL_FALLIDO = const(4)
## @brief Fin de la partida por perder todas las vidas o completar el juego.
FIN = const(5)

## @brief El botón fue correcto, pero faltan más pasos en la secuencia.
CORRECTO_CONTINUA = const(0)
## @brief El botón fue correcto y se completó exitosamente toda la secuencia.
CORRECTO_NIVEL_COMPLETO = const(1)
## @brief El botón presionado no coincidía con el esperado en la secuencia.
INCORRECTO = const(2)

## @brief Duración de 2 segundos encendidos al ganar el nivel.
DURACION_VICTORIA = const(1000) 
## @brief Duración de 3 parpadeos síncronos (6 x 75 ms).
DURACION_FALLO = const(450)

# Variables del juego
estado_actual = INICIO
estado_botones = CORRECTO_CONTINUA
secuencia_leds = [0] * 9  # Pre-reserva 9 espacios como en C++
nivel_actual = 1
vidas = 3
contador_botones = 0
tiempo_acumulado = 0

# Variables de tiempo
tiempo_presentacion_actual = 0
tiempo_ingreso = 0
aux_tiempo_ingreso = 0
aux_tiempo_presentacion = 0

# Tiempo de efectos
tiempo_nivel_completo = 0
tiempo_nivel_fallido = 0

def generar_siguiente_elemento():
    """
    @brief Genera de forma aleatoria el siguiente LED de la secuencia.
    
    @details Se llama al avanzar de nivel. Asigna un número aleatorio (0 a 3) 
    al índice correspondiente del arreglo `secuencia_leds`.
    """
    global secuencia_leds
    indice = nivel_actual - 1
    if 0 <= indice < 9:
        # random.randrange(4) genera de 0 a 3, equivalente a random(0, 4) en Arduino
        secuencia_leds[indice] = random.randrange(4)

def calcular_tiempo_presentacion():
    """
    @brief Calcula la duración de las fases de presentación e ingreso.
    
    @details La velocidad aumenta según el nivel del jugador. A mayor nivel, 
    menor tiempo para visualizar y responder a la secuencia.
    """
    global tiempo_presentacion_actual, tiempo_ingreso
    frecuencia = 0.0
    
    if nivel_actual <= 2:
        frecuencia = 1.0
    elif nivel_actual <= 4:
        frecuencia = 1.5
    elif nivel_actual <= 6:
        frecuencia = 2.0
    elif nivel_actual <= 8:
        frecuencia = 2.5
    else:
        frecuencia = 3.0
    
    tiempo_presentacion_actual = int((nivel_actual / frecuencia) * 1000.0)
    tiempo_ingreso = int(1.25 * tiempo_presentacion_actual)
    
    # Imprimir tiempo de presentación y tiempo diponible
    print(f"\n--- INICIANDO NIVEL {nivel_actual} ---")
    print(f"Tiempo de presentación de la secuencia: {tiempo_presentacion_actual} ms")
    print(f"Tiempo máximo disponible para responder: {tiempo_ingreso} ms")

def acumular_tiempo(tiempo_transcurrido):
    """
    @brief Suma el tiempo consumido durante el turno actual al global.
    
    @param tiempo_transcurrido Milisegundos actuales del sistema.
    """
    global tiempo_acumulado
    usado_ms = tiempo_transcurrido - aux_tiempo_ingreso
    tiempo_acumulado += usado_ms // 1000
    
    # Imprimir el tiempo acumulado
    print(f"El usuario tardó en responder: {usado_ms} ms")
    
    if tiempo_acumulado > 99:
        tiempo_acumulado = 99

def inicio_reset():
    """
    @brief Reinicia todas las variables a su estado de fábrica.
    
    @details Restablece vidas, niveles, tiempos y contadores para volver
    a la pantalla inicial o empezar desde cero.
    """
    global estado_actual, nivel_actual, vidas, contador_botones, tiempo_acumulado
    global tiempo_presentacion_actual, tiempo_ingreso, aux_tiempo_ingreso, aux_tiempo_presentacion
    global tiempo_nivel_completo, tiempo_nivel_fallido
    
    estado_actual = INICIO
    nivel_actual = 1
    vidas = 3
    contador_botones = 0
    tiempo_acumulado = 0
    tiempo_presentacion_actual = 0
    tiempo_ingreso = 0
    aux_tiempo_ingreso = 0
    aux_tiempo_presentacion = 0
    tiempo_nivel_completo = 0
    tiempo_nivel_fallido = 0

def verificacion_estado():
    """
    @brief Retorna el estado actual de la máquina de estados.
    @return Código del estado actual (INICIO, PRESENTACION, etc.).
    """
    return estado_actual

def cambio_estado_tiempo(tiempo_transcurrido):
    """
    @brief Controla las transiciones de estado dependientes del tiempo.
    
    @details Gestiona las pausas entre niveles, tiempos de penalización, 
    y el paso de la fase de presentación a la fase de inputs.
    
    @param tiempo_transcurrido Milisegundos actuales del sistema.
    """
    global estado_actual, nivel_actual, aux_tiempo_presentacion
    global aux_tiempo_ingreso, vidas, tiempo_nivel_fallido

    # Nivel completado
    if estado_actual == NIVEL_COMPLETO:
        if tiempo_transcurrido - tiempo_nivel_completo >= DURACION_VICTORIA:
            nivel_actual += 1
            calcular_tiempo_presentacion()
            generar_siguiente_elemento()
            aux_tiempo_presentacion = tiempo_transcurrido
            estado_actual = PRESENTACION
        return
    
    # Nivel fallido
    if estado_actual == NIVEL_FALLIDO:
        if tiempo_transcurrido - tiempo_nivel_fallido >= DURACION_FALLO:
            if vidas <= 0:
                estado_actual = FIN
                acumular_tiempo(tiempo_transcurrido)
            else:
                aux_tiempo_presentacion = tiempo_transcurrido
                estado_actual = PRESENTACION
        return
    
    # Presentación
    if estado_actual == PRESENTACION:
        if tiempo_transcurrido - aux_tiempo_presentacion >= tiempo_presentacion_actual:
            estado_actual = INPUTS
            aux_tiempo_ingreso = tiempo_transcurrido
            
    # INPUTs
    elif estado_actual == INPUTS:
        if tiempo_transcurrido - aux_tiempo_ingreso >= tiempo_ingreso:
            acumular_tiempo(tiempo_transcurrido)
            contador_botones = 0
            vidas -= 1
            tiempo_nivel_fallido = tiempo_transcurrido
            estado_actual = NIVEL_FALLIDO

def procesar_boton(boton_presionado, tiempo_transcurrido):
    """
    @brief Evalúa si el botón presionado coincide con la secuencia.
    
    @details Actualiza vidas, contadores y estados de la máquina si el jugador
    acierta, se equivoca o completa el nivel.
    
    @param boton_presionado ID del botón presionado (0-3).
    @param tiempo_transcurrido Milisegundos actuales del sistema.
    
    @return Estado del intento (CORRECTO_CONTINUA, INCORRECTO, etc.).
    """
    global contador_botones, estado_botones, estado_actual, tiempo_nivel_completo, vidas, tiempo_nivel_fallido
    
    if boton_presionado == secuencia_leds[contador_botones]:
        contador_botones += 1
        
        if contador_botones == nivel_actual:
            acumular_tiempo(tiempo_transcurrido)
            contador_botones = 0
            estado_botones = CORRECTO_NIVEL_COMPLETO
            
            if nivel_actual == 9:
                estado_actual = FIN
            else:
                tiempo_nivel_completo = tiempo_transcurrido
                estado_actual = NIVEL_COMPLETO
        else:
            estado_botones = CORRECTO_CONTINUA
            estado_actual = INPUTS
    else:
        acumular_tiempo(tiempo_transcurrido)
        contador_botones = 0
        estado_botones = INCORRECTO
        vidas -= 1
        tiempo_nivel_fallido = tiempo_transcurrido
        estado_actual = NIVEL_FALLIDO
        
    return estado_botones

def iniciar_partida(tiempo_transcurrido):
    """
    @brief Arranca un juego nuevo desde cero.
    
    @param tiempo_transcurrido Milisegundos actuales del sistema.
    """
    global aux_tiempo_presentacion, estado_actual
    inicio_reset()
    calcular_tiempo_presentacion()
    generar_siguiente_elemento()
    aux_tiempo_presentacion = tiempo_transcurrido
    estado_actual = PRESENTACION

def obtener_porcentaje_tiempo(tiempo_transcurrido):
    """
    @brief Calcula el porcentaje de tiempo consumido para ingresar botones.
    
    @param tiempo_transcurrido Milisegundos actuales del sistema.
    @return Valor entre 0 y 100 indicando el tiempo consumido.
    """
    if estado_actual != INPUTS or tiempo_ingreso == 0:
        return 0
    
    tiempo_pasado = tiempo_transcurrido - aux_tiempo_ingreso
    porcentaje = (tiempo_pasado * 100) // tiempo_ingreso
    
    if porcentaje > 100:
        porcentaje = 100
    return porcentaje

def obtener_nivel():
    """
    @brief Retorna el nivel actual del jugador.
    @return Nivel actual (1 a 9).
    """
    return nivel_actual

def obtener_vidas():
    """
    @brief Retorna las vidas restantes.
    @return Vidas (0 a 3).
    """
    return vidas

def obtener_tiempo_acumulado():
    """
    @brief Retorna los segundos consumidos en los inputs durante la partida.
    @return Tiempo acumulado en segundos (Max 99).
    """
    return tiempo_acumulado

def obtener_led_actual_presentacion(tiempo_transcurrido):
    """
    @brief Determina qué LED debe estar encendido en la animación de presentación.
    
    @param tiempo_transcurrido (int) Milisegundos actuales del sistema.
    @return (int) Índice del LED a encender (0 a 3), o -1 si todos deben estar apagados.
    """
    if estado_actual != PRESENTACION:
        return -1
    
    transcurrido = tiempo_transcurrido - aux_tiempo_presentacion
    
    if nivel_actual == 0: 
        return -1
        
    tiempo_por_elemento = tiempo_presentacion_actual // nivel_actual
    
    if tiempo_por_elemento == 0:
        return -1
    
    indice = transcurrido // tiempo_por_elemento
    
    if indice >= nivel_actual:
        return -1
    
    dentro_del_elemento = transcurrido % tiempo_por_elemento
    tiempo_encendido = (tiempo_por_elemento * 70) // 100
    
    if dentro_del_elemento < tiempo_encendido:
        return secuencia_leds[indice]
    
    return -1