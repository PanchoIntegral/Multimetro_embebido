import { View, Text, StyleSheet } from 'react-native';
import Svg, { Polyline, Line, Text as SvgText } from 'react-native-svg';
import { colors } from '../theme';

export default function Oscilloscope({ data }) {
  const values = data.map((item) => item.value);

  // Configuración del gráfico
  const width = 320;
  const height = 250;
  const padding = { top: 20, right: 20, bottom: 40, left: 50 };
  const chartWidth = width - padding.left - padding.right;
  const chartHeight = height - padding.top - padding.bottom;

  // Calcular escala
  const getScaledData = () => {
    if (values.length === 0) return '';

    const minValue = Math.min(...values);
    const maxValue = Math.max(...values);
    const range = maxValue - minValue || 1;

    const points = values.map((value, index) => {
      const x = padding.left + (index / (values.length - 1 || 1)) * chartWidth;
      const y = padding.top + chartHeight - ((value - minValue) / range) * chartHeight;
      return `${x},${y}`;
    }).join(' ');

    return points;
  };

  // Etiquetas del eje Y
  const getYAxisLabels = () => {
    if (values.length === 0) return [];
    const minValue = Math.min(...values);
    const maxValue = Math.max(...values);
    const steps = 5;
    const labels = [];
    
    for (let i = 0; i <= steps; i++) {
      const value = minValue + (maxValue - minValue) * (i / steps);
      const y = padding.top + chartHeight - (i / steps) * chartHeight;
      labels.push({ value: value.toFixed(1), y });
    }
    
    return labels;
  };

  return (
    <View style={styles.card}>
      <View style={styles.header}>
        <Text style={styles.title}>Osciloscopio</Text>
        <View style={styles.badge}>
          <Text style={styles.badgeText}>{data.length}/100 muestras</Text>
        </View>
      </View>

      <View style={styles.chartContainer}>
        {values.length > 1 ? (
          <Svg width={width} height={height}>
            {/* Grid líneas horizontales */}
            {[0, 1, 2, 3, 4, 5].map((i) => {
              const y = padding.top + (chartHeight / 5) * i;
              return (
                <Line
                  key={`h-${i}`}
                  x1={padding.left}
                  y1={y}
                  x2={padding.left + chartWidth}
                  y2={y}
                  stroke={colors.border}
                  strokeWidth="1"
                  strokeDasharray="3,3"
                />
              );
            })}

            {/* Grid líneas verticales */}
            {[0, 1, 2, 3, 4].map((i) => {
              const x = padding.left + (chartWidth / 4) * i;
              return (
                <Line
                  key={`v-${i}`}
                  x1={x}
                  y1={padding.top}
                  x2={x}
                  y2={padding.top + chartHeight}
                  stroke={colors.border}
                  strokeWidth="1"
                  strokeDasharray="3,3"
                />
              );
            })}

            {/* Línea de datos */}
            <Polyline
              points={getScaledData()}
              fill="none"
              stroke={colors.accent}
              strokeWidth="2"
            />

            {/* Etiquetas eje Y */}
            {getYAxisLabels().map((label, i) => (
              <SvgText
                key={`label-${i}`}
                x={padding.left - 10}
                y={label.y + 4}
                fontSize="10"
                fill={colors.textSecondary}
                textAnchor="end"
              >
                {label.value}
              </SvgText>
            ))}

            {/* Borde del gráfico */}
            <Line
              x1={padding.left}
              y1={padding.top}
              x2={padding.left}
              y2={padding.top + chartHeight}
              stroke={colors.text}
              strokeWidth="1"
            />
            <Line
              x1={padding.left}
              y1={padding.top + chartHeight}
              x2={padding.left + chartWidth}
              y2={padding.top + chartHeight}
              stroke={colors.text}
              strokeWidth="1"
            />
          </Svg>
        ) : (
          <View style={styles.noData}>
            <Text style={styles.noDataText}>Esperando datos...</Text>
          </View>
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
    marginBottom: 32,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  title: {
    fontSize: 16,
    fontWeight: '500',
    color: colors.text,
  },
  badge: {
    backgroundColor: '#f3f4f6',
    paddingHorizontal: 12,
    paddingVertical: 4,
    borderRadius: 20,
  },
  badgeText: {
    fontSize: 11,
    color: colors.textSecondary,
  },
  chartContainer: {
    alignItems: 'stretch',
  },
  noData: {
    height: 250,
    justifyContent: 'center',
    alignItems: 'center',
  },
  noDataText: {
    color: colors.textSecondary,
    fontSize: 14,
  },
});
