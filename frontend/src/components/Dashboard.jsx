import { View, Text, ScrollView, StyleSheet } from 'react-native';
import MeterDisplay from './MeterDisplay';
import Oscilloscope from './Oscilloscope';
import ControlPanel from './ControlPanel';
import { colors } from '../theme';

export default function Dashboard({
  isConnected,
  currentData,
  oscilloscopeData,
  currentMode,
  onModeChange,
}) {
  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>Multímetro Digital</Text>
        <View style={styles.statusRow}>
          <View
            style={[
              styles.statusDot,
              { backgroundColor: isConnected ? colors.success : colors.error },
            ]}
          />
          <Text style={styles.statusText}>
            {isConnected ? 'Conectado' : 'Desconectado'}
          </Text>
        </View>
      </View>

      {/* Medición Actual */}
      <MeterDisplay data={currentData} mode={currentMode} />

      {/* Panel de Control */}
      <ControlPanel
        currentMode={currentMode}
        onModeChange={onModeChange}
        isConnected={isConnected}
      />

      {/* Osciloscopio */}
      <Oscilloscope data={oscilloscopeData} mode={currentMode} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  scroll: {
    flex: 1,
  },
  content: {
    padding: 16,
    paddingTop: 60,
    gap: 16,
  },
  header: {
    marginBottom: 8,
  },
  title: {
    fontSize: 28,
    fontWeight: '300',
    color: colors.text,
    marginBottom: 8,
  },
  statusRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  statusText: {
    fontSize: 14,
    color: colors.textSecondary,
  },
});
