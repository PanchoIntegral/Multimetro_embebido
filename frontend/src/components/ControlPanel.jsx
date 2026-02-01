import { View, Text, Pressable, StyleSheet } from 'react-native';
import { colors } from '../theme';

const modes = [
  { id: 'VOLTAGE', label: 'Voltaje', icon: 'V' },
  { id: 'CURRENT', label: 'Corriente', icon: 'A' },
  { id: 'RESISTANCE', label: 'Resistencia', icon: 'Ω' },
  { id: 'CAPACITANCE', label: 'Capacitancia', icon: 'C' },
  { id: 'CONTINUITY', label: 'Continuidad', icon: '⟿' },
  { id: 'FREQUENCY', label: 'Frecuencia', icon: 'Hz' },
];

export default function ControlPanel({ currentMode, onModeChange, isConnected }) {
  return (
    <View style={styles.card}>
      <Text style={styles.title}>Modo de Medición</Text>

      <View style={styles.grid}>
        {modes.map((mode) => {
          const isActive = currentMode === mode.id;
          return (
            <Pressable
              key={mode.id}
              onPress={() => onModeChange(mode.id)}
              disabled={!isConnected}
              style={[
                styles.button,
                isActive && styles.buttonActive,
                !isConnected && styles.buttonDisabled,
              ]}
            >
              <Text
                style={[
                  styles.buttonIcon,
                  isActive && styles.buttonIconActive,
                ]}
              >
                {mode.icon}
              </Text>
              <Text
                style={[
                  styles.buttonLabel,
                  isActive && styles.buttonLabelActive,
                ]}
              >
                {mode.label}
              </Text>
            </Pressable>
          );
        })}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.card,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: colors.border,
    padding: 16,
  },
  title: {
    fontSize: 16,
    fontWeight: '500',
    color: colors.text,
    marginBottom: 12,
  },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  button: {
    width: '31%',
    paddingVertical: 16,
    borderRadius: 10,
    borderWidth: 2,
    borderColor: colors.border,
    backgroundColor: colors.card,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
  },
  buttonActive: {
    borderColor: colors.accent,
    backgroundColor: colors.accent,
  },
  buttonDisabled: {
    opacity: 0.4,
  },
  buttonIcon: {
    fontSize: 28,
    fontWeight: '300',
    color: colors.accent,
  },
  buttonIconActive: {
    color: '#ffffff',
  },
  buttonLabel: {
    fontSize: 11,
    fontWeight: '500',
    color: colors.textSecondary,
    textAlign: 'center',
  },
  buttonLabelActive: {
    color: '#ffffff',
  },
});
