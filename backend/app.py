from flask import Flask, render_template, jsonify, request
from flask_socketio import SocketIO, emit
from flask_cors import CORS
from serial_reader import SerialReader
from config import HOST, PORT, DEBUG, MEASUREMENT_TYPES

app = Flask(__name__)
app.config['SECRET_KEY'] = 'tu-clave-secreta-segura'
CORS(app)

socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

# Inicializar lector serial
serial_reader = SerialReader(socketio)

@app.route('/')
def index():
    return jsonify({
        'status': 'online',
        'measurement_types': list(MEASUREMENT_TYPES.keys())
    })

@socketio.on('connect')
def handle_connect():
    print('✓ Cliente conectado')
    emit('connection_status', {'status': 'connected'})
    
    # Iniciar lectura serial si no está corriendo
    if not serial_reader.is_running:
        if serial_reader.start_reading():
            emit('serial_status', {'status': 'connected'})
        else:
            emit('serial_status', {'status': 'error', 'message': 'No se pudo conectar al puerto serial'})

@socketio.on('disconnect')
def handle_disconnect():
    print('✗ Cliente desconectado')

@socketio.on('change_measurement_mode')
def handle_mode_change(data):
    mode = data.get('mode')
    print(f'📡 Cambio de modo solicitado: {mode}')
    if mode in MEASUREMENT_TYPES:
        # Intentar cambiar modo en Arduino
        serial_reader.change_mode(mode)
        # Siempre emitir éxito para que la UI funcione sin Arduino
        emit('mode_changed', {'mode': mode, 'success': True}, broadcast=True)
        print(f'✓ Modo cambiado a: {mode}')
    else:
        emit('error', {'message': 'Modo de medición no válido'})

@socketio.on('request_current_data')
def handle_data_request():
    # Enviar último dato del buffer si existe
    if serial_reader.buffer:
        emit('measurement_data', serial_reader.buffer[-1])

if __name__ == '__main__':
    print(f"🚀 Servidor iniciando en http://{HOST}:{PORT}")
    print(f"📊 WebSocket disponible en ws://{HOST}:{PORT}")
    socketio.run(app, host=HOST, port=PORT, debug=DEBUG)
