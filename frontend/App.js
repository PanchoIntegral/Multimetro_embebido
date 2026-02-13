import { useState, useEffect } from 'react';
import { View, StyleSheet } from 'react-native';
import { StatusBar } from 'expo-status-bar';
import { io } from 'socket.io-client';
import Dashboard from './src/components/Dashboard';
import { colors } from './src/theme';

const SOCKET_URL = 'http://10.21.5.17:5001';

export default function App() {
  const [socket, setSocket] = useState(null);
  const [isConnected, setIsConnected] = useState(false);
  const [currentData, setCurrentData] = useState(null);
  const [oscilloscopeData, setOscilloscopeData] = useState([]);
  const [currentMode, setCurrentMode] = useState('VOLTAGE');

  useEffect(() => {
    const newSocket = io(SOCKET_URL, {
      transports: ['websocket'],
      reconnection: true,
    });

    newSocket.on('connect', () => {
      setIsConnected(true);
    });

    newSocket.on('disconnect', () => {
      setIsConnected(false);
    });

    newSocket.on('measurement_data', (data) => {
      setCurrentData(data);
    });

    newSocket.on('oscilloscope_data', (data) => {
      setOscilloscopeData(data.buffer);
    });

    newSocket.on('mode_changed', (data) => {
      if (data.success) {
        setCurrentMode(data.mode);
      }
    });

    // Limpiar datos cuando se cambia de modo
    newSocket.on('clear_measurement', (data) => {
      console.log('Limpiando mediciones para modo:', data.mode);
      setCurrentData(null);
      setOscilloscopeData([]);
    });

    // Mostrar advertencia cuando el modo no está implementado
    newSocket.on('mode_not_implemented', (data) => {
      console.warn('Modo no implementado:', data.mode, '-', data.message);
      // Aquí podrías mostrar un toast o alerta al usuario
    });

    setSocket(newSocket);

    return () => newSocket.close();
  }, []);

  const handleModeChange = (mode) => {
    setCurrentMode(mode);
    // Limpiar datos inmediatamente al cambiar de modo
    setCurrentData(null);
    setOscilloscopeData([]);

    if (socket && isConnected) {
      socket.emit('change_measurement_mode', { mode });
    }
  };

  return (
    <View style={styles.container}>
      <StatusBar style="dark" />
      <Dashboard
        isConnected={isConnected}
        currentData={currentData}
        oscilloscopeData={oscilloscopeData}
        currentMode={currentMode}
        onModeChange={handleModeChange}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: colors.bg,
  },
});
