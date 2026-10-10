import numpy as np
import matplotlib.pyplot as plt


N_MUESTRAS = 256 
BITS_DAC = 8    
VALOR_MAX = (2**BITS_DAC) - 1


t = np.linspace(0, 2 * np.pi, N_MUESTRAS, endpoint=False)


onda = (np.sin(t) + 1) * (VALOR_MAX / 2)

onda = np.round(onda).astype(int)


plt.plot(onda, marker='.')
plt.title("Tratamiento de Señal: Ciclo de Onda para DAC")
plt.xlabel("Muestras")
plt.ylabel("Amplitud Digital (0-255)")
plt.grid()
plt.show()


codigo_c = f"// Tabla de onda almacenada en Flash\n"
codigo_c += f"const uint8_t tabla_onda[{N_MUESTRAS}] = {{\n    "
for i, valor in enumerate(onda):
    codigo_c += f"{valor},"

    if (i + 1) % 16 == 0 and i != N_MUESTRAS - 1:
        codigo_c += "\n    "
codigo_c += "\n};\n"
print("-" * 40)
print(codigo_c)