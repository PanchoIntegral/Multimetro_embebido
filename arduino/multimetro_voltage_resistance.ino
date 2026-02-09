/*
 * MULTÍMETRO DIGITAL - VOLTAJE Y RESISTENCIA
 * Arduino Mega + Relés con Transistores 2N2222
 *
 * VOLTAJE:
 * - Rango: 0-20V con divisor de voltaje 10kΩ/2kΩ
 * - Pin Relé: 22
 * - Pin ADC: A0
 *
 * RESISTENCIA:
 * - Rango: 0-10kΩ (usando resistencia de referencia 10kΩ)
 * - Pin Relé: 23
 * - Pin ADC: A1
 * - Método: Divisor de voltaje con resistencia conocida
 */

// ========== CONFIGURACIÓN DE PINES ==========
// Voltaje
#define RELAY_VOLTAGE_PIN 22
#define ADC_VOLTAGE_PIN A0

// Resistencia
#define RELAY_RESISTANCE_PIN 23
#define ADC_RESISTANCE_PIN A1

// ========== CONSTANTES ==========
const float VREF = 5.0;
const float ADC_RESOLUTION = 1023.0;
const int NUM_SAMPLES = 50;
const int SAMPLE_DELAY = 2;

// Constantes de voltaje
const float VOLTAGE_DIVIDER_FACTOR = 6.0;  // (10k+2k)/2k = 6

// Constantes de resistencia
const float R_REFERENCE = 10000.0;  // 10kΩ resistencia de referencia

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

// Calibración resistencia
float resistanceCalibrationOffset = 0.0;
float resistanceCalibrationGain = 1.0;

// Variables para simulación de resistencia
unsigned long lastResistanceChange = 0;
float simulatedResistance = 4700.0;

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  // Configurar pines de voltaje
  pinMode(RELAY_VOLTAGE_PIN, OUTPUT);
  pinMode(ADC_VOLTAGE_PIN, INPUT);
  digitalWrite(RELAY_VOLTAGE_PIN, LOW);

  // Configurar pines de resistencia
  pinMode(RELAY_RESISTANCE_PIN, OUTPUT);
  pinMode(ADC_RESISTANCE_PIN, INPUT);
  digitalWrite(RELAY_RESISTANCE_PIN, LOW);

  analogReference(DEFAULT);

  Serial.println("{\"status\":\"ready\",\"module\":\"multimeter\",\"version\":\"2.0\",\"modes\":[\"voltage\",\"resistance\"]}");

  delay(100);
}

// ========== LOOP PRINCIPAL ==========
void loop() {
  // Procesar comandos
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }

  // Realizar medición según el modo activo
  switch (currentMode) {
    case MODE_VOLTAGE:
      {
        float voltage = measureVoltage();
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
  // Comandos de VOLTAJE
  if (command == "START_VOLTAGE") {
    activateVoltageMode();
  }
  else if (command == "STOP_VOLTAGE") {
    deactivateVoltageMode();
  }

  // Comandos de RESISTENCIA
  else if (command == "START_RESISTANCE") {
    activateResistanceMode();
  }
  else if (command == "STOP_RESISTANCE") {
    deactivateResistanceMode();
  }

  // Comandos de calibración voltaje
  else if (command == "CALIBRATE_ZERO_VOLTAGE") {
    calibrateZeroVoltage();
  }
  else if (command.startsWith("CALIBRATE_VOLTAGE:")) {
    float referenceVoltage = command.substring(18).toFloat();
    calibrateGainVoltage(referenceVoltage);
  }

  // Comandos de calibración resistencia
  else if (command == "CALIBRATE_ZERO_RESISTANCE") {
    calibrateZeroResistance();
  }
  else if (command.startsWith("CALIBRATE_RESISTANCE:")) {
    float referenceResistance = command.substring(21).toFloat();
    calibrateGainResistance(referenceResistance);
  }

  // Comandos generales
  else if (command == "GET_STATUS") {
    sendStatus();
  }
  else if (command == "GET_RAW_ADC") {
    sendRawADC();
  }

  // Comando desconocido
  else {
    Serial.println("{\"error\":\"unknown_command\",\"received\":\"" + command + "\"}");
  }
}

// ========== MODO VOLTAJE ==========
void activateVoltageMode() {
  // Desactivar cualquier modo anterior
  deactivateAllModes();

  // Activar modo voltaje
  digitalWrite(RELAY_VOLTAGE_PIN, HIGH);
  currentMode = MODE_VOLTAGE;
  delay(50);
  Serial.println("{\"status\":\"voltage_mode_active\"}");
}

void deactivateVoltageMode() {
  digitalWrite(RELAY_VOLTAGE_PIN, LOW);
  if (currentMode == MODE_VOLTAGE) {
    currentMode = MODE_NONE;
  }
  Serial.println("{\"status\":\"voltage_mode_inactive\"}");
}

float measureVoltage() {
  long sumADC = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    sumADC += analogRead(ADC_VOLTAGE_PIN);
    delay(SAMPLE_DELAY);
  }

  float avgADC = (float)sumADC / NUM_SAMPLES;
  float voltageADC = (avgADC / ADC_RESOLUTION) * VREF;
  float voltage = voltageADC * VOLTAGE_DIVIDER_FACTOR;

  voltage = (voltage + voltageCalibrationOffset) * voltageCalibrationGain;

  if (voltage < 0) voltage = 0;
  if (voltage > 20.0) voltage = 20.0;

  return voltage;
}

void sendVoltageData(float voltage) {
  Serial.print("{\"type\":\"voltage\",\"value\":");
  Serial.print(voltage, 3);
  Serial.print(",\"unit\":\"V\",\"timestamp\":");
  Serial.print(millis());
  Serial.println("}");
}

// ========== MODO RESISTENCIA ==========
void activateResistanceMode() {
  // Desactivar cualquier modo anterior
  deactivateAllModes();

  // Activar modo resistencia
  digitalWrite(RELAY_RESISTANCE_PIN, HIGH);
  currentMode = MODE_RESISTANCE;
  delay(50);
  Serial.println("{\"status\":\"resistance_mode_active\"}");
}

void deactivateResistanceMode() {
  digitalWrite(RELAY_RESISTANCE_PIN, LOW);
  if (currentMode == MODE_RESISTANCE) {
    currentMode = MODE_NONE;
  }
  Serial.println("{\"status\":\"resistance_mode_inactive\"}");
}

float measureResistance() {
  // ⚠️ MODO SIMULACIÓN - Descomentar esto para probar SIN circuito físico
  // Genera valores aleatorios entre 1kΩ y 10kΩ
  static unsigned long lastChange = 0;
  static float simValue = 4700.0;

  if (millis() - lastChange > 3000) {
    lastChange = millis();
    simValue = random(1000, 10000);
  }

  return simValue + random(-50, 50);

  // ⚠️ CÓDIGO REAL - Comenta las líneas de arriba y descomenta esto cuando tengas el circuito
  /*
  long sumADC = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    sumADC += analogRead(ADC_RESISTANCE_PIN);
    delay(SAMPLE_DELAY);
  }

  float avgADC = (float)sumADC / NUM_SAMPLES;

  // Si ADC es muy bajo, la resistencia es muy alta (circuito abierto)
  if (avgADC < 1.0) {
    return -1.0; // Indicador de circuito abierto
  }

  // Convertir ADC a voltaje
  float voltageADC = (avgADC / ADC_RESOLUTION) * VREF;

  // Calcular resistencia usando divisor de voltaje
  // V_out = V_in * (R_x / (R_ref + R_x))
  // Despejando: R_x = (V_out * R_ref) / (V_in - V_out)

  float resistance = (voltageADC * R_REFERENCE) / (VREF - voltageADC);

  // Aplicar calibración
  resistance = (resistance + resistanceCalibrationOffset) * resistanceCalibrationGain;

  // Limitar a rango válido
  if (resistance < 0) resistance = 0;
  if (resistance > 1000000.0) resistance = -1.0; // Circuito abierto

  return resistance;
  */
}

void sendResistanceData(float resistance) {
  Serial.print("{\"type\":\"resistance\",\"value\":");

  if (resistance < 0) {
    Serial.print("\"OL\"");  // OverLoad / Open Circuit
  } else if (resistance < 1000) {
    // Mostrar en Ω
    Serial.print(resistance, 1);
  } else if (resistance < 1000000) {
    // Mostrar en kΩ
    Serial.print(resistance / 1000.0, 3);
  }

  Serial.print(",\"unit\":\"");
  if (resistance < 0) {
    Serial.print("OL");
  } else if (resistance < 1000) {
    Serial.print("Ω");
  } else {
    Serial.print("kΩ");
  }
  Serial.print("\",\"timestamp\":");
  Serial.print(millis());
  Serial.println("}");
}

// ========== CALIBRACIÓN ==========
void calibrateZeroVoltage() {
  if (currentMode != MODE_VOLTAGE) {
    activateVoltageMode();
    delay(100);
  }

  float measuredVoltage = measureVoltage();
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

  float measuredVoltage = measureVoltage();

  if (measuredVoltage > 0.1) {
    voltageCalibrationGain = referenceVoltage / measuredVoltage;
  }

  Serial.print("{\"status\":\"calibration_gain_voltage_complete\",\"gain\":");
  Serial.print(voltageCalibrationGain, 4);
  Serial.print(",\"reference\":");
  Serial.print(referenceVoltage, 3);
  Serial.println("}");
}

void calibrateZeroResistance() {
  if (currentMode != MODE_RESISTANCE) {
    activateResistanceMode();
    delay(100);
  }

  float measuredResistance = measureResistance();
  resistanceCalibrationOffset = -measuredResistance;

  Serial.print("{\"status\":\"calibration_zero_resistance_complete\",\"offset\":");
  Serial.print(resistanceCalibrationOffset, 4);
  Serial.println("}");
}

void calibrateGainResistance(float referenceResistance) {
  if (currentMode != MODE_RESISTANCE) {
    activateResistanceMode();
    delay(100);
  }

  float measuredResistance = measureResistance();

  if (measuredResistance > 1.0) {
    resistanceCalibrationGain = referenceResistance / measuredResistance;
  }

  Serial.print("{\"status\":\"calibration_gain_resistance_complete\",\"gain\":");
  Serial.print(resistanceCalibrationGain, 4);
  Serial.print(",\"reference\":");
  Serial.print(referenceResistance, 3);
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
    case MODE_VOLTAGE:
      Serial.print("voltage");
      break;
    case MODE_RESISTANCE:
      Serial.print("resistance");
      break;
    default:
      Serial.print("none");
      break;
  }

  Serial.print("\",\"voltage_calibration\":{\"offset\":");
  Serial.print(voltageCalibrationOffset, 4);
  Serial.print(",\"gain\":");
  Serial.print(voltageCalibrationGain, 4);
  Serial.print("},\"resistance_calibration\":{\"offset\":");
  Serial.print(resistanceCalibrationOffset, 4);
  Serial.print(",\"gain\":");
  Serial.print(resistanceCalibrationGain, 4);
  Serial.println("}}");
}

void sendRawADC() {
  int rawVoltageADC = analogRead(ADC_VOLTAGE_PIN);
  int rawResistanceADC = analogRead(ADC_RESISTANCE_PIN);

  Serial.print("{\"type\":\"diagnostic\",\"voltage_adc\":");
  Serial.print(rawVoltageADC);
  Serial.print(",\"voltage_v\":");
  Serial.print((rawVoltageADC / ADC_RESOLUTION) * VREF, 4);
  Serial.print(",\"resistance_adc\":");
  Serial.print(rawResistanceADC);
  Serial.print(",\"resistance_v\":");
  Serial.print((rawResistanceADC / ADC_RESOLUTION) * VREF, 4);
  Serial.println("}");
}
