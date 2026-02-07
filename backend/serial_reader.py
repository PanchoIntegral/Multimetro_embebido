import serial
import json
import threading
import time
from config import SERIAL_PORT, SERIAL_BAUDRATE, SERIAL_TIMEOUT

class SerialReader:
    def __init__(self, socketio):
        self.socketio = socketio
        self.serial_port = None
        self.is_running = False
        self.current_mode = 'VOLTAGE'
        self.buffer = []
        
    def connect(self):
        try:
            # Cerrar conexión previa si existe
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
                time.sleep(0.5)  # Esperar para liberar el puerto
            
            self.serial_port = serial.Serial(
                port=SERIAL_PORT,
                baudrate=SERIAL_BAUDRATE,
                timeout=SERIAL_TIMEOUT,
                exclusive=True  # Acceso exclusivo al puerto
            )
            time.sleep(2)  # Esperar para que Arduino se inicialice
            print(f"✓ Conectado a {SERIAL_PORT}")
            return True
        except serial.SerialException as e:
            if "PermissionError" in str(e):
                print(f"✗ Error: Puerto {SERIAL_PORT} en uso. Cierra el Monitor Serial de Arduino.")
            else:
                print(f"✗ Error conectando serial: {e}")
            return False
        except Exception as e:
            print(f"✗ Error conectando serial: {e}")
            return False
    
    def start_reading(self):
        if not self.serial_port or not self.serial_port.is_open:
            if not self.connect():
                return False
        
        self.is_running = True
        self.thread = threading.Thread(target=self._read_loop, daemon=True)
        self.thread.start()
        return True
    
    def _read_loop(self):
        error_count = 0
        max_errors = 5
        
        while self.is_running:
            try:
                if self.serial_port and self.serial_port.is_open and self.serial_port.in_waiting:
                    line = self.serial_port.readline().decode('utf-8').strip()
                    if line:
                        self._process_data(line)
                        error_count = 0  # Reset error count on successful read
                else:
                    time.sleep(0.1)
            except serial.SerialException as e:
                error_count += 1
                if error_count <= max_errors:
                    print(f"Error leyendo serial ({error_count}/{max_errors}): {e}")
                elif error_count == max_errors + 1:
                    print("✗ Demasiados errores seriales. Intentando reconectar...")
                    self._reconnect()
                time.sleep(1)
            except Exception as e:
                error_count += 1
                if error_count <= max_errors:
                    print(f"Error inesperado ({error_count}/{max_errors}): {e}")
                time.sleep(0.5)
    
    def _reconnect(self):
        """Intenta reconectar al puerto serial"""
        self.is_running = False
        if self.serial_port:
            self.serial_port.close()
        time.sleep(2)
        if self.connect():
            self.is_running = True
            print("✓ Reconectado exitosamente")
    
    def _process_data(self, line):
        try:
            # Primero intentar parsear como JSON (formato del Arduino)
            try:
                json_data = json.loads(line)

                # Verificar si es un mensaje de error
                if 'error' in json_data:
                    print(f"⚠️  Arduino error: {json_data.get('error')} - {json_data.get('received', '')}")
                    return

                # Verificar si es un mensaje de estado
                if 'status' in json_data and 'type' not in json_data:
                    print(f"ℹ️  Arduino status: {json_data}")
                    return

                # Procesar datos de medición
                if json_data.get('type') == 'voltage':
                    data = {
                        'type': 'VOLTAGE',
                        'value': float(json_data['value']),
                        'uncertainty': 0.0,  # Arduino no envía uncertainty aún
                        'timestamp': json_data.get('timestamp', time.time() * 1000),
                        'unit': json_data.get('unit', 'V')
                    }

                    # Enviar datos puntuales
                    self.socketio.emit('measurement_data', data)

                    # Actualizar buffer para osciloscopio
                    self.buffer.append(data)
                    if len(self.buffer) > 100:
                        self.buffer.pop(0)

                    self.socketio.emit('oscilloscope_data', {
                        'buffer': self.buffer
                    })

            except json.JSONDecodeError:
                # Si no es JSON, intentar el formato antiguo "TYPE:value:uncertainty"
                parts = line.split(':')
                if len(parts) >= 3:
                    data = {
                        'type': parts[0],
                        'value': float(parts[1]),
                        'uncertainty': float(parts[2]),
                        'timestamp': time.time() * 1000,
                        'unit': self._get_unit(parts[0])
                    }

                    self.socketio.emit('measurement_data', data)

                    self.buffer.append(data)
                    if len(self.buffer) > 100:
                        self.buffer.pop(0)

                    self.socketio.emit('oscilloscope_data', {
                        'buffer': self.buffer
                    })
                else:
                    print(f"📝 Arduino dice: {line}")

        except Exception as e:
            print(f"Error procesando datos: {e}")
            print(f"Línea recibida: {line}")
    
    def _get_unit(self, mtype):
        units = {
            'VOLTAGE': 'V',
            'CURRENT': 'A',
            'RESISTANCE': 'Ω',
            'CAPACITANCE': 'µF',
            'CONTINUITY': '',
            'FREQUENCY': 'Hz'
        }
        return units.get(mtype, '')
    
    def change_mode(self, mode):
        """Envía comando al Arduino para cambiar el modo de medición"""
        if self.serial_port and self.serial_port.is_open:
            # Primero detener el modo anterior si es necesario
            if self.current_mode == 'VOLTAGE':
                self.serial_port.write(b"STOP_VOLTAGE\n")
                time.sleep(0.1)

            # Mapear el modo al comando correcto del Arduino
            command_map = {
                'VOLTAGE': 'START_VOLTAGE',
                'CURRENT': 'START_CURRENT',
                'RESISTANCE': 'START_RESISTANCE',
                'CAPACITANCE': 'START_CAPACITANCE',
                'CONTINUITY': 'START_CONTINUITY',
                'FREQUENCY': 'START_FREQUENCY'
            }

            command = command_map.get(mode, 'GET_STATUS')
            self.serial_port.write(f"{command}\n".encode('utf-8'))
            print(f"📡 Enviando comando: {command}")

            self.current_mode = mode
            self.buffer = []  # Limpiar buffer al cambiar modo
            return True
        return False
    
    def stop(self):
        self.is_running = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
