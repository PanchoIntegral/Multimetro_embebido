# Configuración del puerto serial
SERIAL_PORT = '/dev/tty.usbmodem1101'  # macOS default - update with your Arduino port
# Para encontrar tu puerto en macOS: ls /dev/tty.*
# Para Windows: COM3, COM4, etc.
# Para Linux: /dev/ttyUSB0, /dev/ttyACM0, etc.

SERIAL_BAUDRATE = 115200
SERIAL_TIMEOUT = 1

# Configuración del servidor
HOST = '0.0.0.0'
PORT = 5001
DEBUG = True

# Tipos de medición
MEASUREMENT_TYPES = {
    'VOLTAGE': 'V',
    'CURRENT': 'A',
    'RESISTANCE': 'Ω',
    'CAPACITANCE': 'µF',
    'CONTINUITY': 'CONT',
    'FREQUENCY': 'Hz'
}

# Buffer para osciloscopio
OSCILLOSCOPE_BUFFER_SIZE = 100
