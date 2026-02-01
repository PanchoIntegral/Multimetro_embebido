# Sistema de Multímetro Embebido con Interfaz Web en Tiempo Real

Sistema completo de multímetro industrial con Arduino Mega, backend Flask y frontend React para mediciones en tiempo real con visualización web.

## 🎯 Características

- **6 Modos de Medición**: Voltaje, Corriente, Resistencia, Capacitancia, Continuidad, Frecuencia
- **Interfaz Web en Tiempo Real**: Actualización instantánea vía WebSockets
- **Vista Puntual**: Lectura actual con incertidumbre calculada
- **Osciloscopio**: Gráfica en tiempo real de las últimas 100 muestras
- **Control Remoto**: Cambio de modo de medición desde la interfaz web
- **Precisión**: ±4% en todas las mediciones

## 📊 Especificaciones Técnicas

| Parámetro | Rango | Precisión |
|-----------|-------|-----------|
| Voltaje | 0-20V | ±4% |
| Corriente | 0-1A | ±4% |
| Resistencia | 0-100kΩ | ±4% |
| Capacitancia | 0-50µF | ±4% |
| Continuidad | Sí/No | - |
| Frecuencia | 0-10kHz | ±4% |

## 🏗️ Arquitectura

```
Arduino Mega (Hardware)
    ↓ Serial USB
Backend Flask + Flask-SocketIO
    ↓ WebSocket
Frontend React + Tailwind CSS
```

## 📂 Estructura del Proyecto

```
multimetro/
├── backend/
│   ├── app.py              # Servidor Flask principal
│   ├── serial_reader.py    # Lectura del puerto serial
│   ├── config.py           # Configuración
│   └── requirements.txt    # Dependencias Python
│
├── frontend/
│   ├── src/
│   │   ├── components/
│   │   │   ├── Dashboard.jsx
│   │   │   ├── MeterDisplay.jsx
│   │   │   ├── Oscilloscope.jsx
│   │   │   └── ControlPanel.jsx
│   │   ├── App.jsx
│   │   └── main.jsx
│   ├── package.json
│   └── tailwind.config.js
│
└── arduino/
    └── multimetro.ino      # Firmware Arduino
```

## 🚀 Instalación y Despliegue

### 1. Arduino

1. Abrir `arduino/multimetro.ino` en Arduino IDE
2. Conectar Arduino Mega vía USB
3. Verificar que los pines de relés coincidan con tu hardware:
   - Voltaje: Pin 22
   - Corriente: Pin 24
   - Resistencia: Pin 26
   - Capacitancia: Pin 28
   - Continuidad: Pin 30
   - Frecuencia: Pin 32
4. Subir el sketch al Arduino

### 2. Backend (Flask)

```bash
cd backend

# Crear entorno virtual (recomendado)
python3 -m venv venv
source venv/bin/activate  # En Windows: venv\Scripts\activate

# Instalar dependencias
pip install -r requirements.txt

# Configurar puerto serial
# Editar config.py y actualizar SERIAL_PORT

# En macOS, encontrar el puerto:
ls /dev/tty.*
# Buscar algo como: /dev/tty.usbmodem1101

# Iniciar servidor
python app.py
```

El servidor estará disponible en `http://localhost:5000`

### 3. Frontend (React)

```bash
cd frontend

# Instalar dependencias
npm install

# Iniciar servidor de desarrollo
npm run dev
```

El frontend estará disponible en `http://localhost:5173` (o el puerto que indique Vite)

## 🔧 Configuración

### Puerto Serial

Editar `backend/config.py`:

```python
# macOS
SERIAL_PORT = '/dev/tty.usbmodem1101'

# Windows
SERIAL_PORT = 'COM3'

# Linux
SERIAL_PORT = '/dev/ttyUSB0'
```

Para encontrar tu puerto:
- **macOS**: `ls /dev/tty.*`
- **Windows**: Administrador de dispositivos → Puertos COM
- **Linux**: `ls /dev/ttyUSB* /dev/ttyACM*`

### Servidor Backend

En `backend/config.py`:

```python
HOST = '0.0.0.0'  # Permitir acceso desde otras máquinas
PORT = 5000       # Puerto del servidor
DEBUG = True      # Modo debug (cambiar a False en producción)
```

## 📡 Protocolo de Comunicación

### Arduino → Backend

Formato: `TYPE:value:uncertainty\n`

Ejemplo:
```
VOLTAGE:12.45:0.50
CURRENT:0.35:0.01
RESISTANCE:4700.00:188.00
```

### Backend ↔ Frontend (WebSocket)

**Eventos del servidor:**
- `connection_status`: Estado de conexión
- `serial_status`: Estado del puerto serial
- `measurement_data`: Datos de medición puntual
- `oscilloscope_data`: Buffer para osciloscopio
- `mode_changed`: Confirmación de cambio de modo

**Eventos del cliente:**
- `change_measurement_mode`: Solicitar cambio de modo
- `request_current_data`: Solicitar datos actuales

## 🧪 Uso

1. **Conectar el Arduino** vía USB
2. **Iniciar el backend**: `python backend/app.py`
3. **Iniciar el frontend**: `npm run dev` en la carpeta frontend
4. **Abrir el navegador** en `http://localhost:5173`
5. **Verificar conexión**: El indicador debe mostrar "Conectado" en verde
6. **Seleccionar modo**: Hacer clic en el botón del modo deseado
7. **Ver mediciones**: Los valores se actualizan automáticamente

## 🛠️ Solución de Problemas

### Backend no se conecta al Arduino

- Verificar que el Arduino esté conectado: `ls /dev/tty.*` (macOS)
- Verificar que el puerto en `config.py` sea correcto
- Verificar que no haya otro programa usando el puerto serial (cerrar Arduino IDE)
- Verificar permisos: `sudo chmod 666 /dev/tty.usbmodem*` (Linux/macOS)

### Frontend no se conecta al backend

- Verificar que el backend esté corriendo en `http://localhost:5000`
- Verificar que no haya firewall bloqueando el puerto 5000
- Revisar la consola del navegador para errores de WebSocket
- Verificar que la URL en `App.jsx` sea correcta

### No aparecen datos en la interfaz

- Verificar que el Arduino esté enviando datos (abrir Serial Monitor en Arduino IDE)
- Verificar que el formato de datos sea correcto: `TYPE:value:uncertainty`
- Revisar logs del backend para errores de parsing
- Verificar que el modo seleccionado coincida con el circuito conectado

## 🔌 Circuitos de Medición

### Voltaje (0-20V)
- Divisor de voltaje 4:1 para reducir 20V a 5V máximo
- R1 = 30kΩ, R2 = 10kΩ

### Corriente (0-1A)
- Sensor ACS712-5A o similar
- Salida analógica proporcional a la corriente

### Resistencia (0-100kΩ)
- Divisor de voltaje con resistencia conocida de 10kΩ
- Medir voltaje para calcular resistencia desconocida

### Capacitancia (0-50µF)
- Circuito RC con resistencia conocida
- Medir tiempo de carga para calcular capacitancia

### Continuidad
- Resistencia pull-up
- Detectar cortocircuito (voltaje alto = continuidad)

### Frecuencia (0-10kHz)
- Entrada digital con contador de pulsos
- Medir frecuencia durante 1 segundo

## 📝 Licencia

Este proyecto es de código abierto. Úsalo y modifícalo según tus necesidades.

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor, abre un issue o pull request para mejoras.

## 📧 Soporte

Para preguntas o problemas, abre un issue en el repositorio del proyecto.
