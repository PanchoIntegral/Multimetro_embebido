import { useState, useEffect } from 'react';
import { View, StyleSheet } from 'react-native';
import { StatusBar } from 'expo-status-bar';
import { io } from 'socket.io-client';
import Dashboard from './src/components/Dashboard';
import { colors } from './src/theme';

const SOCKET_URL = 'http://10.21.38.30:5001';

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

    setSocket(newSocket);

    return () => newSocket.close();
  }, []);

  const handleModeChange = (mode) => {
    setCurrentMode(mode);
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
