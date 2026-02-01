import { View, Text, StyleSheet } from 'react-native';
import { LineChart, Grid, XAxis, YAxis } from 'react-native-svg-charts';
import { colors } from '../theme';

export default function Oscilloscope({ data }) {
  const values = data.map((item) => item.value);

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
          <View style={{ height: 250, flexDirection: 'row' }}>
            <YAxis
              data={values}
              contentInset={{ top: 20, bottom: 20 }}
              svg={{ fontSize: 10, fill: colors.textSecondary }}
              numberOfTicks={6}
            />
            <View style={{ flex: 1, marginLeft: 8 }}>
              <LineChart
                style={{ flex: 1 }}
                data={values}
                svg={{ stroke: colors.accent, strokeWidth: 2 }}
                contentInset={{ top: 20, bottom: 20 }}
              >
                <Grid svg={{ stroke: colors.border, strokeDasharray: [3, 3] }} />
              </LineChart>
              <XAxis
                data={values}
                formatLabel={(value) => value}
                contentInset={{ left: 10, right: 10 }}
                svg={{ fontSize: 10, fill: colors.textSecondary }}
                numberOfTicks={5}
              />
            </View>
          </View>
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
