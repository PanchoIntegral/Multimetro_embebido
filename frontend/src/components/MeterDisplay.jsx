import { View, Text, StyleSheet } from 'react-native';
import Svg, { Line, Circle, G } from 'react-native-svg';
import { colors } from '../theme';

function ContinuityAnimation({ connected }) {
  const color = connected ? '#22c55e' : '#6b7280';
  const rightX = connected ? 106 : 180;

  return (
    <View style={styles.continuityContainer}>
      <Svg width={280} height={120} viewBox="0 0 280 120">
        {/* Left wire */}
        <Line x1="0" y1="60" x2="100" y2="60" stroke={color} strokeWidth="4" strokeLinecap="round" />
        {/* Left connector */}
        <Circle cx="100" cy="60" r="6" fill={color} />
        {/* Right connector */}
        <Circle cx={rightX} cy="60" r="6" fill={color} />
        {/* Right wire */}
        <Line x1={rightX} y1="60" x2="280" y2="60" stroke={color} strokeWidth="4" strokeLinecap="round" />

        {/* Spark when connected */}
        {connected && (
          <Circle cx="103" cy="60" r="10" fill="#22c55e" opacity={0.3} />
        )}

        {/* Gap X marks when disconnected */}
        {!connected && (
          <G>
            <Line x1="115" y1="45" x2="125" y2="75" stroke="#ef4444" strokeWidth="2" opacity={0.6} />
            <Line x1="125" y1="45" x2="115" y2="75" stroke="#ef4444" strokeWidth="2" opacity={0.6} />
            <Line x1="155" y1="45" x2="165" y2="75" stroke="#ef4444" strokeWidth="2" opacity={0.6} />
            <Line x1="165" y1="45" x2="155" y2="75" stroke="#ef4444" strokeWidth="2" opacity={0.6} />
          </G>
        )}
      </Svg>

      <Text style={[styles.continuityText, { color: connected ? '#22c55e' : '#ef4444' }]}>
        {connected ? '● Circuito Cerrado' : '○ Circuito Abierto'}
      </Text>
    </View>
  );
}

export default function MeterDisplay({ data, mode }) {
  const getModeLabel = (m) => {
    const labels = {
      VOLTAGE: 'Voltaje',
      CURRENT: 'Corriente',
      RESISTANCE: 'Resistencia',
      CAPACITANCE: 'Capacitancia',
      CONTINUITY: 'Continuidad',
      FREQUENCY: 'Frecuencia',
    };
    return labels[m] || m;
  };

  const getModeIcon = (m) => {
    const icons = {
      VOLTAGE: 'V',
      CURRENT: 'A',
      RESISTANCE: 'Ω',
      CAPACITANCE: 'C',
      CONTINUITY: '⟿',
      FREQUENCY: 'Hz',
    };
    return icons[m] || '';
  };

  const isContinuity = mode === 'CONTINUITY';
  const hasContinuity = isContinuity && data && data.value === 1;

  // Determinar el valor a mostrar
  const getDisplayValue = () => {
    if (!data) return '---.--';

    // Si hay un display_value especial (ej: "OL"), usarlo
    if (data.display_value !== undefined && typeof data.display_value === 'string') {
      return data.display_value;
    }

    // Si el valor es negativo (indicador de error), mostrar "OL"
    if (data.value < 0) {
      return 'OL';
    }

    // Valor numérico normal
    return data.value.toFixed(2);
  };

  return (
    <View style={styles.card}>
      <Text style={styles.modeLabel}>{getModeLabel(mode)}</Text>

      <View style={styles.valueContainer}>
        {isContinuity ? (
          <ContinuityAnimation connected={hasContinuity} />
        ) : (
          <>
            <View style={styles.valueRow}>
              <Text style={styles.value}>
                {getDisplayValue()}
              </Text>
              <Text style={styles.unit}>{getModeIcon(mode)}</Text>
            </View>
            <Text style={styles.unitLabel}>{data ? data.unit : ''}</Text>
            {data && data.uncertainty != null && data.value >= 0 && (
              <Text style={styles.uncertainty}>
                Incertidumbre: ±{data.uncertainty.toFixed(2)} {data.unit}
              </Text>
            )}
          </>
        )}
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
    padding: 24,
  },
  modeLabel: {
    fontSize: 16,
    fontWeight: '500',
    color: colors.text,
    marginBottom: 16,
  },
  valueContainer: {
    alignItems: 'center',
    paddingVertical: 32,
  },
  valueRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
    marginBottom: 8,
  },
  value: {
    fontSize: 56,
    fontWeight: '300',
    color: colors.text,
    letterSpacing: -1,
  },
  unit: {
    fontSize: 40,
    fontWeight: '300',
    color: colors.accent,
  },
  unitLabel: {
    fontSize: 20,
    fontWeight: '300',
    color: colors.textSecondary,
  },
  uncertainty: {
    marginTop: 16,
    fontSize: 13,
    color: colors.textSecondary,
  },
  continuityContainer: {
    alignItems: 'center',
    gap: 16,
  },
  continuityText: {
    fontSize: 16,
    fontWeight: '500',
  },
});
