# Circuito para Medición de Resistencia

## Principio de Funcionamiento

El Arduino mide resistencias usando el método de **divisor de voltaje** con una resistencia de referencia conocida.

### Fórmula

```
V_out = V_in × (R_x / (R_ref + R_x))

Despejando R_x:
R_x = (V_out × R_ref) / (V_in - V_out)
```

Donde:
- **V_in** = 5V (voltaje de referencia del Arduino)
- **V_out** = Voltaje medido por el ADC
- **R_ref** = 10kΩ (resistencia de referencia)
- **R_x** = Resistencia desconocida a medir

---

## Circuito Completo

```
                    +5V
                     |
                     |
                  [R_ref]  (10kΩ resistencia de referencia)
                     |
                     +----------> A1 (ADC_RESISTANCE_PIN)
                     |
                  [R_x]   (Resistencia a medir)
                     |
       Relé NC ------+
          |
       Común (C) -----> Terminales de medición
          |
       Relé NO -------> GND


Control del Relé:

  Arduino Pin 23 ----[1kΩ]---- Base (2N2222)
                                 |
                              Colector ----> Relé IN
                                 |
                              Emisor ------> GND

  Relé VCC ----> +5V
  Relé GND ----> GND
```

---

## Componentes Necesarios

### 1. Resistencia de Referencia
- **Valor**: 10kΩ (código de colores: marrón-negro-naranja)
- **Tolerancia**: 1% o mejor (preferible)
- **Potencia**: 1/4W o superior

### 2. Relé
- **Tipo**: Relé SPDT (Single Pole Double Throw) 5V
- **Pines**:
  - **IN**: Control (conectado al colector del 2N2222)
  - **VCC**: +5V Arduino
  - **GND**: GND Arduino
  - **COM**: Común (punto de medición)
  - **NO**: Normalmente abierto (conectado a GND)
  - **NC**: Normalmente cerrado (conectado al divisor de voltaje)

### 3. Transistor 2N2222 (Driver del Relé)
- **Base**: Conectada al pin 23 del Arduino con resistencia de 1kΩ
- **Colector**: Conectado al pin IN del relé
- **Emisor**: Conectado a GND

### 4. Resistencia de Base
- **Valor**: 1kΩ
- **Ubicación**: Entre pin 23 de Arduino y base del 2N2222

---

## Conexiones Físicas

### Conexiones al Arduino Mega

| Arduino Pin | Conexión              |
|-------------|-----------------------|
| Pin 23      | Base 2N2222 (con 1kΩ) |
| A1          | Punto medio divisor   |
| +5V         | VCC relé + R_ref      |
| GND         | GND relé + emisor     |

### Secuencia de Conexión del Divisor de Voltaje

1. **+5V** → Un extremo de **R_ref (10kΩ)**
2. **Otro extremo de R_ref** → **Pin A1** del Arduino
3. **Pin A1** → Un extremo de **R_x** (resistencia a medir)
4. **Otro extremo de R_x** → **Terminal NC del relé**
5. **Terminal COM del relé** → **Salida al usuario** (terminal rojo)
6. **Terminal NO del relé** → **GND**

---

## Rangos de Medición

Con R_ref = 10kΩ:

| Rango         | Precisión     | Notas                           |
|---------------|---------------|---------------------------------|
| 0Ω - 100Ω     | Baja          | Voltaje muy bajo, poco preciso  |
| 100Ω - 1kΩ    | Media         | Rango aceptable                 |
| 1kΩ - 10kΩ    | Alta          | Rango óptimo                    |
| 10kΩ - 100kΩ  | Media-Alta    | Rango aceptable                 |
| > 100kΩ       | Baja          | Se muestra "OL" (OverLoad)      |

### Mejoras Futuras (Múltiples Rangos)

Para medir resistencias más pequeñas o grandes con precisión, puedes agregar:
- **R_ref = 100Ω** (para medir 0-1kΩ)
- **R_ref = 1kΩ** (para medir 100Ω-10kΩ)
- **R_ref = 10kΩ** (para medir 1kΩ-100kΩ) ← **ACTUAL**
- **R_ref = 100kΩ** (para medir 10kΩ-1MΩ)

Cada resistencia de referencia tendría su propio relé controlado por un pin diferente.

---

## Calibración

### Calibración de Cero (Cortocircuito)
1. Conecta los terminales de medición entre sí (cortocircuito)
2. Envía el comando: `CALIBRATE_ZERO_RESISTANCE`
3. El Arduino guardará el offset de calibración

### Calibración con Resistencia Conocida
1. Conecta una resistencia de valor conocido (ej: 4.7kΩ medido con otro multímetro)
2. Envía el comando: `CALIBRATE_RESISTANCE:4700`
3. El Arduino calculará el factor de ganancia

---

## Diagrama de Flujo de Medición

```
Inicio
  ↓
Usuario selecciona modo RESISTANCE
  ↓
Backend envía: START_RESISTANCE
  ↓
Arduino activa relé (pin 23)
  ↓
Divisor de voltaje se conecta
  ↓
Arduino lee ADC en pin A1 (50 muestras)
  ↓
Calcula V_out promedio
  ↓
Aplica fórmula: R_x = (V_out × 10kΩ) / (5V - V_out)
  ↓
Aplica calibración (offset + gain)
  ↓
Envía JSON: {"type":"resistance","value":4.7,"unit":"kΩ"}
  ↓
Frontend muestra: 4.7 kΩ
```

---

## Prueba del Circuito

### Prueba 1: Cortocircuito (0Ω)
- Conecta los terminales entre sí
- Deberías leer: **0.0 Ω** (o muy cercano a 0)

### Prueba 2: Resistencia Conocida
- Conecta una resistencia de 1kΩ (marrón-negro-rojo)
- Deberías leer: **~1.0 kΩ** (±5% si la resistencia es de tolerancia 5%)

### Prueba 3: Circuito Abierto
- No conectes nada entre los terminales
- Deberías leer: **OL** (OverLoad)

---

## Troubleshooting

### Problema: Siempre lee OL
- Verifica que el relé se está activando (escucha el "click")
- Verifica conexión de R_ref (10kΩ) entre +5V y A1
- Verifica que A1 esté conectado correctamente

### Problema: Siempre lee 0Ω
- Verifica que R_x esté conectado en serie con R_ref
- Revisa que no haya cortocircuito en el circuito
- Verifica que el terminal NO del relé esté conectado a GND

### Problema: Lecturas muy imprecisas
- Calibra el cero
- Calibra con resistencia conocida
- Verifica tolerancia de R_ref (debe ser 1% o mejor)
- Evita cables muy largos (agregan resistencia parásita)

---

## Mejoras Recomendadas

1. **Usar resistencia de precisión 1%** para R_ref
2. **Agregar múltiples rangos** con diferentes R_ref
3. **Agregar protección contra sobretensión** en las entradas
4. **Agregar filtrado capacitivo** para reducir ruido (ej: 100nF en paralelo con R_x)

---

## Código Arduino Relacionado

Ver archivo: `multimetro_voltage_resistance.ino`

Funciones clave:
- `activateResistanceMode()` - Activa el relé
- `measureResistance()` - Lee ADC y calcula resistencia
- `sendResistanceData()` - Envía datos al backend
