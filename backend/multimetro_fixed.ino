/*
 * MULTÍMETRO DIGITAL - VERSIÓN 2.1 CORREGIDA
 *
 * CAMBIO CLAVE: Compensación de resistencia parásita (relay + cables + breadboard)
 *
 * PROBLEMA QUE SE RESOLVIÓ:
 *   El relay y las conexiones suman ~990Ω en serie con R_ref.
 *   Sin compensar: 330Ω leía 299.70Ω (error = 330 × 9810/(9810+990))
 *   Con compensar: 330Ω lee ~330Ω ±1%
 *
 * FÓRMULA (con DEFAULT reference, VCC se cancela):
 *   R_x = ADC × R_EFFECTIVE / (1023 − ADC)
 *   R_EFFECTIVE = R_REFERENCE + R_SERIES (parásita)
 *
 * CALIBRACIÓN RÁPIDA:
 *   1. Conecta resistencia de valor conocido (ej: 330Ω medido con multímetro)
 *   2. Envía por serial: CALIBRATE_SERIES:330
 *   3. ¡Listo! Corrige TODAS las mediciones automáticamente
 *
 * MEJORAS:
 *   - Fórmula directa con ADC (no depende de VREF exacto)
 *   - Auto-calibración de resistencia parásita
 *   - 500 muestras burst mode + media recortada 80%
 *   - Insertion sort (estable, eficiente en memoria)
 *   - Filtro IIR adaptativo (rápido ante cambios bruscos)
 *   - Diagnóstico detallado con DEBUG_RESISTANCE
 */

// ========== CONFIGURACIÓN DE PINES ==========
#define RELAY_VOLTAGE_PIN 22
#define ADC_VOLTAGE_PIN A0
#define RELAY_RESISTANCE_PIN 52
#define ADC_RESISTANCE_PIN A1

// ========== CONSTANTES ==========
const float ADC_MAX = 1023.0;
const float VREF_NOMINAL = 5.0;  // Solo para voltaje y diagnóstico (NO para resistencia)

// Resistencia de referencia nominal (medida con multímetro)
const float R_REFERENCE = 9810.0;

// RESISTENCIA PARÁSITA: relay + cables + breadboard
// Se calibra automáticamente con CALIBRATE_SERIES:xxx
// Después de calibrar, puedes poner aquí el valor para que persista entre resets
float R_SERIES = 0.0;

// R_EFFECTIVE = R_REFERENCE + R_SERIES (se recalcula al calibrar)
float R_EFFECTIVE = 9810.0;

// PARÁMETROS DE MUESTREO - RESISTENCIA
const int RES_NUM_SAMPLES = 500;       // Burst mode: muchas muestras rápidas
const int RES_DISCARD = 30;            // Descartar iniciales
const int RES_SAMPLE_DELAY_US = 200;   // 200µs entre lecturas

// PARÁMETROS DE MUESTREO - VOLTAJE
const int VOLT_NUM_SAMPLES = 200;
const int VOLT_DISCARD = 20;
const int VOLT_SAMPLE_DELAY_MS = 5;

// Constantes de voltaje
const float VOLTAGE_DIVIDER_FACTOR = 6.0;

// FILTRO IIR
const float ALPHA_RESISTANCE = 0.25;
const float ALPHA_VOLTAGE = 0.3;

// ========== CALIBRACIÓN MULTI-PUNTO ==========
struct CalibrationPoint {
  float measured_raw;
  float reference_value;
  bool valid;
};

#define MAX_CALIBRATION_POINTS 10
CalibrationPoint calibrationPoints[MAX_CALIBRATION_POINTS];
int numCalibrationPoints = 0;
bool calibrationEnabled = false;

// ========== ENUMERACIÓN DE MODOS ==========
enum MeasurementMode {
  MODE_NONE,
  MODE_VOLTAGE,
  MODE_RESISTANCE
};

// ========== VARIABLES GLOBALES ==========
MeasurementMode currentMode = MODE_NONE;

// Calibración voltaje
float voltageCalibrationOffset = 0.0;
float voltageCalibrationGain = 1.0;

// Filtros IIR
float previousResistance = 0;
float previousVoltage = 0;
bool firstMeasurement = true;

// Estadísticas última medición (para diagnóstico)
float lastADC_avg = 0;
float lastADC_stddev = 0;

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_VOLTAGE_PIN, OUTPUT);
  pinMode(ADC_VOLTAGE_PIN, INPUT);
  digitalWrite(RELAY_VOLTAGE_PIN, LOW);

  pinMode(RELAY_RESISTANCE_PIN, OUTPUT);
  pinMode(ADC_RESISTANCE_PIN, INPUT);
  digitalWrite(RELAY_RESISTANCE_PIN, LOW);

  // DEFAULT = VCC como referencia. Para resistencia, VCC se cancela en la fórmula
  analogReference(DEFAULT);
  delay(100);

  R_EFFECTIVE = R_REFERENCE + R_SERIES;
  initializeCalibration();

  Serial.print("{\"status\":\"ready\",\"module\":\"multimeter_v2.1\",\"r_ref\":");
  Serial.print(R_REFERENCE, 1);
  Serial.print(",\"r_series\":");
  Serial.print(R_SERIES, 1);
  Serial.print(",\"r_effective\":");
  Serial.print(R_EFFECTIVE, 1);
  Serial.println("}");
  delay(100);
}

// ========== UTILIDADES DE MUESTREO ==========
// Insertion sort: O(n²) pero eficiente en memoria, sin VLA
void insertionSort(int* arr, int n) {
  for (int i = 1; i < n; i++) {
    int key = arr[i];
    int j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j--;
    }
    arr[j + 1] = key;
  }
}

// Lee ADC con alta precisión: N muestras burst + media recortada 80%
float readADCTrimmed(int pin, int numSamples, int discardCount, int delayUs) {
  // Descartar lecturas iniciales (estabilizar S/H y mux)
  for (int i = 0; i < discardCount; i++) {
    analogRead(pin);
    delayMicroseconds(200);
  }

  // Burst sampling
  static int samples[500];
  int n = min(numSamples, 500);
  for (int i = 0; i < n; i++) {
    samples[i] = analogRead(pin);
    if (delayUs > 0) delayMicroseconds(delayUs);
  }

  // Ordenar
  insertionSort(samples, n);

  // Media recortada 80% (descarta 10% inferior y 10% superior)
  int start = n / 10;
  int end = n - start;
  int count = end - start;

  double sum = 0;
  for (int i = start; i < end; i++) {
    sum += samples[i];
  }
  float avg = (float)(sum / count);

  // Calcular stddev para diagnóstico
  double sumSq = 0;
  for (int i = start; i < end; i++) {
    double d = samples[i] - avg;
    sumSq += d * d;
  }
  lastADC_stddev = sqrt(sumSq / count);

  return avg;
}

// ========== LOOP PRINCIPAL ==========
void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }

  switch (currentMode) {
    case MODE_VOLTAGE:
      {
        float voltage = measureVoltageOptimized();
        sendVoltageData(voltage);
        delay(100);
      }
      break;

    case MODE_RESISTANCE:
      {
        float resistance = measureResistance();
        sendResistanceData(resistance);
        delay(100);
      }
      break;

    default:
      delay(10);
      break;
  }
}

// ========== PROCESAMIENTO DE COMANDOS ==========
void processCommand(String command) {
  // --- VOLTAJE ---
  if (command == "START_VOLTAGE") {
    activateVoltageMode();
  }
  else if (command == "STOP_VOLTAGE") {
    deactivateVoltageMode();
  }
  // --- RESISTENCIA ---
  else if (command == "START_RESISTANCE") {
    activateResistanceMode();
  }
  else if (command == "STOP_RESISTANCE") {
    deactivateResistanceMode();
  }

  // ─── CALIBRACIÓN DE RESISTENCIA PARÁSITA ───
  // Conecta resistencia conocida → envía su valor → corrige todo automáticamente
  else if (command.startsWith("CALIBRATE_SERIES:")) {
    float knownR = command.substring(17).toFloat();
    calibrateSeriesResistance(knownR);
  }
  // Establecer R_SERIES manualmente (si ya la conoces)
  else if (command.startsWith("SET_R_SERIES:")) {
    R_SERIES = command.substring(13).toFloat();
    R_EFFECTIVE = R_REFERENCE + R_SERIES;
    Serial.print("{\"status\":\"r_series_set\",\"r_series\":");
    Serial.print(R_SERIES, 2);
    Serial.print(",\"r_effective\":");
    Serial.print(R_EFFECTIVE, 2);
    Serial.println("}");
  }

  // --- CALIBRACIÓN MULTI-PUNTO (compatibilidad) ---
  else if (command.startsWith("CALIBRATE_POINT_RESISTANCE:")) {
    float referenceValue = command.substring(28).toFloat();
    addCalibrationPoint(referenceValue);
  }
  else if (command.startsWith("CALIBRATE_POINT_RESISTANCE_LOW:")) {
    float referenceValue = command.substring(32).toFloat();
    addCalibrationPoint(referenceValue);
  }
  else if (command.startsWith("CALIBRATE_POINT_RESISTANCE_HIGH:")) {
    float referenceValue = command.substring(33).toFloat();
    addCalibrationPoint(referenceValue);
  }
  else if (command == "SAVE_CALIBRATION_RESISTANCE") {
    saveCalibration();
  }
  else if (command == "CLEAR_CALIBRATION_RESISTANCE") {
    clearCalibration();
  }
  else if (command == "SHOW_CALIBRATION_RESISTANCE") {
    showCalibration();
  }
  else if (command == "ENABLE_CALIBRATION_RESISTANCE") {
    calibrationEnabled = true;
    Serial.println("{\"status\":\"calibration_enabled\"}");
  }
  else if (command == "DISABLE_CALIBRATION_RESISTANCE") {
    calibrationEnabled = false;
    Serial.println("{\"status\":\"calibration_disabled\"}");
  }

  // --- CALIBRACIÓN VOLTAJE ---
  else if (command == "CALIBRATE_ZERO_VOLTAGE") {
    calibrateZeroVoltage();
  }
  else if (command.startsWith("CALIBRATE_VOLTAGE:")) {
    float referenceVoltage = command.substring(18).toFloat();
    calibrateGainVoltage(referenceVoltage);
  }

  // --- DIAGNÓSTICO ---
  else if (command == "DEBUG_RESISTANCE") {
    debugResistanceDetailed();
  }
  else if (command == "GET_STATUS") {
    sendStatus();
  }
  else if (command == "GET_RAW_ADC") {
    sendRawADC();
  }
  else {
    Serial.println("{\"error\":\"unknown_command\",\"received\":\"" + command + "\"}");
  }
}

// ========== MODO RESISTENCIA ==========
void activateResistanceMode() {
  deactivateAllModes();
  digitalWrite(RELAY_RESISTANCE_PIN, HIGH);
  currentMode = MODE_RESISTANCE;
  firstMeasurement = true;

  delay(150);  // Estabilizar relay

  // Descartar lecturas iniciales
  for (int i = 0; i < 50; i++) {
    analogRead(ADC_RESISTANCE_PIN);
    delayMicroseconds(500);
  }

  Serial.print("{\"status\":\"resistance_mode_active\",\"r_effective\":");
  Serial.print(R_EFFECTIVE, 1);
  Serial.println("}");
}

void deactivateResistanceMode() {
  digitalWrite(RELAY_RESISTANCE_PIN, LOW);
  if (currentMode == MODE_RESISTANCE) {
    currentMode = MODE_NONE;
  }
  Serial.println("{\"status\":\"resistance_mode_inactive\"}");
}

/*
 * MEDICIÓN DE RESISTENCIA - FÓRMULA DIRECTA
 *
 * Circuito: VCC → R_ref → Relay → [A1] → R_x → GND
 * Con DEFAULT (AREF=VCC), VCC se cancela:
 *   ADC/1023 = R_x / (R_EFFECTIVE + R_x)
 *   R_x = ADC × R_EFFECTIVE / (1023 − ADC)
 */
float measureResistance() {
  // Lectura ADC con media recortada 80%
  float avgADC = readADCTrimmed(ADC_RESISTANCE_PIN, RES_NUM_SAMPLES,
                                 RES_DISCARD, RES_SAMPLE_DELAY_US);
  lastADC_avg = avgADC;

  // Circuito abierto (R_x → ∞, ADC → 0)
  if (avgADC < 0.5) return -1.0;

  // Cortocircuito (R_x → 0, ADC → 1023)
  if (avgADC > 1021.0) return 0.0;

  // FÓRMULA DIRECTA (sin VREF)
  float resistanceRaw = avgADC * R_EFFECTIVE / (ADC_MAX - avgADC);

  // Aplicar calibración multi-punto si habilitada
  float resistanceCorrected;
  if (calibrationEnabled && numCalibrationPoints >= 2) {
    resistanceCorrected = applyCalibration(resistanceRaw);
  } else {
    resistanceCorrected = resistanceRaw;
  }

  // Filtro IIR adaptativo
  if (firstMeasurement) {
    previousResistance = resistanceCorrected;
    firstMeasurement = false;
  } else {
    float diff = abs(resistanceCorrected - previousResistance);
    float threshold = previousResistance * 0.15;  // 15% = cambio brusco

    if (diff > threshold && previousResistance > 1.0) {
      // Cambio grande: aceptar directo sin filtrar
      previousResistance = resistanceCorrected;
    } else {
      // Cambio pequeño: suavizar
      resistanceCorrected = ALPHA_RESISTANCE * resistanceCorrected +
                           (1.0 - ALPHA_RESISTANCE) * previousResistance;
      previousResistance = resistanceCorrected;
    }
  }

  if (resistanceCorrected < 0) resistanceCorrected = 0;
  if (resistanceCorrected > 200000.0) resistanceCorrected = -1.0;

  return resistanceCorrected;
}

void sendResistanceData(float resistance) {
  Serial.print("{\"type\":\"resistance\",\"value\":");

  if (resistance < 0) {
    Serial.print("\"OL\"");
  } else {
    Serial.print(resistance, 2);  // Siempre en Ohms
  }

  Serial.print(",\"unit\":\"");
  if (resistance < 0) {
    Serial.print("OL");
  } else if (resistance < 1000) {
    Serial.print("Ω");
  } else {
    Serial.print("kΩ");
  }

  Serial.print("\",\"raw_adc\":");
  Serial.print(lastADC_avg, 2);
  Serial.print(",\"r_effective\":");
  Serial.print(R_EFFECTIVE, 1);
  Serial.print(",\"calibrated\":");
  Serial.print(calibrationEnabled ? "true" : "false");
  Serial.print(",\"timestamp\":");
  Serial.print(millis());
  Serial.println("}");
}

// ========== CALIBRACIÓN RESISTENCIA PARÁSITA ==========
/*
 * Conecta una resistencia CONOCIDA (ej: 330Ω medida con multímetro bueno).
 * El código calcula R_EFFECTIVE real y compensa automáticamente.
 *
 * Matemáticas:
 *   ADC/1023 = R_known / (R_effective + R_known)
 *   R_effective = R_known × (1023/ADC − 1)
 *   R_series = R_effective − R_reference
 */
void calibrateSeriesResistance(float knownR) {
  if (knownR <= 0) {
    Serial.println("{\"error\":\"resistance_must_be_positive\"}");
    return;
  }

  // Asegurar modo resistencia
  if (currentMode != MODE_RESISTANCE) {
    activateResistanceMode();
    delay(300);
  }

  // Deshabilitar filtros temporalmente
  firstMeasurement = true;
  bool tempCal = calibrationEnabled;
  calibrationEnabled = false;

  delay(500);

  // Tomar 5 mediciones independientes del ADC crudo
  float readings[5];
  for (int m = 0; m < 5; m++) {
    readings[m] = readADCTrimmed(ADC_RESISTANCE_PIN, RES_NUM_SAMPLES,
                                  RES_DISCARD, RES_SAMPLE_DELAY_US);
    delay(100);
  }

  // Promediar las 5
  float avgADC = 0;
  for (int i = 0; i < 5; i++) avgADC += readings[i];
  avgADC /= 5.0;

  if (avgADC < 1.0 || avgADC > 1020.0) {
    calibrationEnabled = tempCal;
    Serial.print("{\"error\":\"invalid_adc\",\"adc\":");
    Serial.print(avgADC, 2);
    Serial.println("}");
    return;
  }

  // Calcular R_EFFECTIVE real del circuito
  float R_eff_calc = knownR * (ADC_MAX / avgADC - 1.0);
  float R_series_calc = R_eff_calc - R_REFERENCE;

  // Validar resultado razonable (-200 a +5000 Ω)
  if (R_series_calc < -200 || R_series_calc > 5000) {
    calibrationEnabled = tempCal;
    Serial.print("{\"error\":\"unreasonable_result\",\"r_series\":");
    Serial.print(R_series_calc, 2);
    Serial.println("}");
    return;
  }

  // Aplicar
  R_SERIES = R_series_calc;
  R_EFFECTIVE = R_REFERENCE + R_SERIES;
  calibrationEnabled = tempCal;
  firstMeasurement = true;

  // Verificar midiendo con la corrección aplicada
  delay(300);
  float verify = measureResistance();
  float errorPct = abs(verify - knownR) / knownR * 100.0;

  Serial.print("{\"status\":\"calibration_complete\"");
  Serial.print(",\"known_resistance\":");
  Serial.print(knownR, 2);
  Serial.print(",\"adc_avg\":");
  Serial.print(avgADC, 3);
  Serial.print(",\"r_effective\":");
  Serial.print(R_EFFECTIVE, 2);
  Serial.print(",\"r_series\":");
  Serial.print(R_SERIES, 2);
  Serial.print(",\"verify\":");
  Serial.print(verify, 2);
  Serial.print(",\"error_pct\":");
  Serial.print(errorPct, 2);
  Serial.println("}");
}

// ========== DIAGNÓSTICO DETALLADO ==========
void debugResistanceDetailed() {
  if (currentMode != MODE_RESISTANCE) {
    activateResistanceMode();
    delay(300);
  }

  // Descartar
  for (int i = 0; i < 50; i++) {
    analogRead(ADC_RESISTANCE_PIN);
    delayMicroseconds(200);
  }

  // Medir 500 muestras crudas
  static int samples[500];
  long rawSum = 0;
  int rawMin = 1024, rawMax = 0;
  for (int i = 0; i < 500; i++) {
    samples[i] = analogRead(ADC_RESISTANCE_PIN);
    rawSum += samples[i];
    if (samples[i] < rawMin) rawMin = samples[i];
    if (samples[i] > rawMax) rawMax = samples[i];
    delayMicroseconds(200);
  }
  float rawAvg = (float)rawSum / 500.0;

  // Ordenar y media recortada
  insertionSort(samples, 500);
  double trimSum = 0;
  for (int i = 50; i < 450; i++) trimSum += samples[i];
  float trimAvg = (float)(trimSum / 400.0);

  // Mediana
  float median = (samples[249] + samples[250]) / 2.0;

  // Resistencias calculadas
  float r_nominal = trimAvg * R_REFERENCE / (ADC_MAX - trimAvg);
  float r_calibrated = trimAvg * R_EFFECTIVE / (ADC_MAX - trimAvg);

  // ¿Qué R_EFFECTIVE necesitarías para que dé exactamente 330Ω?
  float reff_for_330 = (trimAvg > 0) ? 330.0 * (ADC_MAX / trimAvg - 1.0) : 0;

  Serial.print("{\"debug\":{");
  Serial.print("\"adc_raw_avg\":");    Serial.print(rawAvg, 3);
  Serial.print(",\"adc_trimmed\":");   Serial.print(trimAvg, 3);
  Serial.print(",\"adc_median\":");    Serial.print(median, 1);
  Serial.print(",\"adc_min\":");       Serial.print(rawMin);
  Serial.print(",\"adc_max\":");       Serial.print(rawMax);
  Serial.print(",\"adc_spread\":");    Serial.print(rawMax - rawMin);
  Serial.print(",\"r_ref\":");         Serial.print(R_REFERENCE, 1);
  Serial.print(",\"r_series\":");      Serial.print(R_SERIES, 2);
  Serial.print(",\"r_effective\":");   Serial.print(R_EFFECTIVE, 2);
  Serial.print(",\"r_with_nominal\":"); Serial.print(r_nominal, 2);
  Serial.print(",\"r_with_calibrated\":");  Serial.print(r_calibrated, 2);
  Serial.print(",\"reff_needed_for_330\":"); Serial.print(reff_for_330, 2);
  Serial.println("}}");
}

// ========== CALIBRACIÓN MULTI-PUNTO ==========
void initializeCalibration() {
  for (int i = 0; i < MAX_CALIBRATION_POINTS; i++) {
    calibrationPoints[i].valid = false;
  }
  numCalibrationPoints = 0;
  calibrationEnabled = false;
}

void addCalibrationPoint(float referenceValue) {
  if (currentMode != MODE_RESISTANCE) {
    Serial.println("{\"error\":\"not_in_resistance_mode\"}");
    return;
  }

  if (numCalibrationPoints >= MAX_CALIBRATION_POINTS) {
    Serial.println("{\"error\":\"max_calibration_points_reached\"}");
    return;
  }

  // Deshabilitar calibración temporalmente para medir valor RAW
  bool tempCalibration = calibrationEnabled;
  calibrationEnabled = false;
  firstMeasurement = true;

  // Estabilizar
  delay(500);

  // Medir valor RAW
  float measuredRaw = measureResistance();

  // Restaurar estado de calibración
  calibrationEnabled = tempCalibration;

  if (measuredRaw < 0) {
    Serial.println("{\"error\":\"invalid_measurement\",\"value\":\"OL\"}");
    return;
  }

  // Guardar punto
  calibrationPoints[numCalibrationPoints].measured_raw = measuredRaw;
  calibrationPoints[numCalibrationPoints].reference_value = referenceValue;
  calibrationPoints[numCalibrationPoints].valid = true;
  numCalibrationPoints++;

  Serial.print("{\"status\":\"calibration_point_added\",\"point\":");
  Serial.print(numCalibrationPoints);
  Serial.print(",\"measured_raw\":");
  Serial.print(measuredRaw, 2);
  Serial.print(",\"reference\":");
  Serial.print(referenceValue, 2);
  Serial.println("}");
}

float applyCalibration(float rawValue) {
  // Ordenar puntos por valor medido
  sortCalibrationPoints();

  // Extrapolación inferior
  if (rawValue <= calibrationPoints[0].measured_raw) {
    if (numCalibrationPoints >= 2) {
      float slope = (calibrationPoints[1].reference_value - calibrationPoints[0].reference_value) /
                    (calibrationPoints[1].measured_raw - calibrationPoints[0].measured_raw);
      return calibrationPoints[0].reference_value + slope * (rawValue - calibrationPoints[0].measured_raw);
    }
    return calibrationPoints[0].reference_value;
  }

  // Extrapolación superior
  if (rawValue >= calibrationPoints[numCalibrationPoints - 1].measured_raw) {
    if (numCalibrationPoints >= 2) {
      int n = numCalibrationPoints - 1;
      float slope = (calibrationPoints[n].reference_value - calibrationPoints[n-1].reference_value) /
                    (calibrationPoints[n].measured_raw - calibrationPoints[n-1].measured_raw);
      return calibrationPoints[n].reference_value + slope * (rawValue - calibrationPoints[n].measured_raw);
    }
    return calibrationPoints[numCalibrationPoints - 1].reference_value;
  }

  // Interpolación lineal
  for (int i = 0; i < numCalibrationPoints - 1; i++) {
    if (rawValue >= calibrationPoints[i].measured_raw &&
        rawValue <= calibrationPoints[i + 1].measured_raw) {
      float x0 = calibrationPoints[i].measured_raw;
      float y0 = calibrationPoints[i].reference_value;
      float x1 = calibrationPoints[i + 1].measured_raw;
      float y1 = calibrationPoints[i + 1].reference_value;

      return y0 + (y1 - y0) * (rawValue - x0) / (x1 - x0);
    }
  }

  return rawValue;
}

void sortCalibrationPoints() {
  for (int i = 0; i < numCalibrationPoints - 1; i++) {
    for (int j = 0; j < numCalibrationPoints - i - 1; j++) {
      if (calibrationPoints[j].measured_raw > calibrationPoints[j + 1].measured_raw) {
        CalibrationPoint temp = calibrationPoints[j];
        calibrationPoints[j] = calibrationPoints[j + 1];
        calibrationPoints[j + 1] = temp;
      }
    }
  }
}

void saveCalibration() {
  if (numCalibrationPoints < 2) {
    Serial.println("{\"error\":\"need_at_least_2_points\"}");
    return;
  }

  sortCalibrationPoints();
  calibrationEnabled = true;

  Serial.print("{\"status\":\"calibration_saved\",\"points\":");
  Serial.print(numCalibrationPoints);
  Serial.println("}");

  showCalibration();
}

void clearCalibration() {
  initializeCalibration();
  Serial.println("{\"status\":\"calibration_cleared\"}");
}

void showCalibration() {
  Serial.print("{\"calibration\":{\"enabled\":");
  Serial.print(calibrationEnabled ? "true" : "false");
  Serial.print(",\"points\":[");

  for (int i = 0; i < numCalibrationPoints; i++) {
    if (calibrationPoints[i].valid) {
      Serial.print("{\"measured\":");
      Serial.print(calibrationPoints[i].measured_raw, 2);
      Serial.print(",\"reference\":");
      Serial.print(calibrationPoints[i].reference_value, 2);
      Serial.print("}");
      if (i < numCalibrationPoints - 1) Serial.print(",");
    }
  }

  Serial.println("]}}");
}

// ========== MODO VOLTAJE ==========
void activateVoltageMode() {
  deactivateAllModes();
  digitalWrite(RELAY_VOLTAGE_PIN, HIGH);
  currentMode = MODE_VOLTAGE;
  firstMeasurement = true;

  delay(100);
  for (int i = 0; i < 30; i++) {
    analogRead(ADC_VOLTAGE_PIN);
    delay(2);
  }

  Serial.println("{\"status\":\"voltage_mode_active\"}");
}

void deactivateVoltageMode() {
  digitalWrite(RELAY_VOLTAGE_PIN, LOW);
  if (currentMode == MODE_VOLTAGE) {
    currentMode = MODE_NONE;
  }
  Serial.println("{\"status\":\"voltage_mode_inactive\"}");
}

float measureVoltageOptimized() {
  float avgADC = readADCTrimmed(ADC_VOLTAGE_PIN, VOLT_NUM_SAMPLES,
                                 VOLT_DISCARD, VOLT_SAMPLE_DELAY_MS * 1000);

  float voltageADC = (avgADC / ADC_MAX) * VREF_NOMINAL;
  float voltage = voltageADC * VOLTAGE_DIVIDER_FACTOR;

  voltage = (voltage + voltageCalibrationOffset) * voltageCalibrationGain;

  if (firstMeasurement) {
    previousVoltage = voltage;
    firstMeasurement = false;
  } else {
    voltage = ALPHA_VOLTAGE * voltage + (1 - ALPHA_VOLTAGE) * previousVoltage;
    previousVoltage = voltage;
  }

  if (voltage < 0) voltage = 0;
  if (voltage > 30.0) voltage = 30.0;

  return voltage;
}

void sendVoltageData(float voltage) {
  Serial.print("{\"type\":\"voltage\",\"value\":");
  Serial.print(voltage, 3);
  Serial.print(",\"unit\":\"V\",\"timestamp\":");
  Serial.print(millis());
  Serial.println("}");
}

// ========== CALIBRACIÓN VOLTAJE ==========
void calibrateZeroVoltage() {
  if (currentMode != MODE_VOLTAGE) {
    activateVoltageMode();
    delay(100);
  }
  firstMeasurement = true;
  delay(200);
  float measuredVoltage = measureVoltageOptimized();
  voltageCalibrationOffset = -measuredVoltage;
  Serial.print("{\"status\":\"calibration_zero_voltage_complete\",\"offset\":");
  Serial.print(voltageCalibrationOffset, 4);
  Serial.println("}");
}

void calibrateGainVoltage(float referenceVoltage) {
  if (currentMode != MODE_VOLTAGE) {
    activateVoltageMode();
    delay(100);
  }
  firstMeasurement = true;
  delay(200);
  float measuredVoltage = measureVoltageOptimized();
  if (measuredVoltage > 0.1) {
    voltageCalibrationGain = referenceVoltage / measuredVoltage;
  }
  Serial.print("{\"status\":\"calibration_gain_voltage_complete\",\"gain\":");
  Serial.print(voltageCalibrationGain, 4);
  Serial.print(",\"reference\":");
  Serial.print(referenceVoltage, 3);
  Serial.println("}");
}

// ========== UTILIDADES ==========
void deactivateAllModes() {
  digitalWrite(RELAY_VOLTAGE_PIN, LOW);
  digitalWrite(RELAY_RESISTANCE_PIN, LOW);
  currentMode = MODE_NONE;
  delay(50);
}

void sendStatus() {
  Serial.print("{\"status\":\"ok\",\"current_mode\":\"");
  switch (currentMode) {
    case MODE_VOLTAGE:    Serial.print("voltage");    break;
    case MODE_RESISTANCE: Serial.print("resistance"); break;
    default:              Serial.print("none");       break;
  }
  Serial.print("\",\"r_reference\":");
  Serial.print(R_REFERENCE, 1);
  Serial.print(",\"r_series\":");
  Serial.print(R_SERIES, 2);
  Serial.print(",\"r_effective\":");
  Serial.print(R_EFFECTIVE, 1);
  Serial.print(",\"calibration_enabled\":");
  Serial.print(calibrationEnabled ? "true" : "false");
  Serial.print(",\"calibration_points\":");
  Serial.print(numCalibrationPoints);
  Serial.println("}");
}

void sendRawADC() {
  int rawVoltageADC = analogRead(ADC_VOLTAGE_PIN);
  int rawResistanceADC = analogRead(ADC_RESISTANCE_PIN);

  Serial.print("{\"type\":\"diagnostic\",\"voltage_adc\":");
  Serial.print(rawVoltageADC);
  Serial.print(",\"resistance_adc\":");
  Serial.print(rawResistanceADC);
  Serial.print(",\"r_effective\":");
  Serial.print(R_EFFECTIVE, 1);
  Serial.print(",\"resistance_calc\":");
  if (rawResistanceADC > 0 && rawResistanceADC < 1023) {
    Serial.print((float)rawResistanceADC * R_EFFECTIVE / (ADC_MAX - rawResistanceADC), 2);
  } else {
    Serial.print("\"OL\"");
  }
  Serial.println("}");
}
