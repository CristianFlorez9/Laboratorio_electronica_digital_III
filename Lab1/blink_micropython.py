from machine import Pin
import time
import sys
import uselect

LED = Pin(25, Pin.OUT)

frecuencia = 1.0
ultimo_cambio = time.ticks_ms()

# Revisar si hay datos disponibles en el teclado/USB
poll = uselect.poll()
poll.register(sys.stdin, uselect.POLLIN)

print("Ingrese frecuencia (Hz):")

while True:

    # Si el usuario escribió algo
    if poll.poll(0):
        entrada = sys.stdin.readline()

        try:
            f = float(entrada)

            if f > 0:
                frecuencia = f
                print("Frecuencia actual:", frecuencia, "Hz")

        except:
            print("Ingrese un número válido")

    # Medio período en milisegundos
    periodo = 500 / frecuencia

    # Cambiar LED
    if time.ticks_diff(time.ticks_ms(), ultimo_cambio) >= periodo:
        ultimo_cambio = time.ticks_ms()
        LED.toggle()