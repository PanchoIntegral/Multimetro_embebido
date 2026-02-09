/*
 * MULTÍMETRO DIGITAL - VERSIÓN DE PRUEBA CON SIMULACIÓN
 *
 * Esta versión simula mediciones para probar la app SIN necesidad de circuitos físicos
 *
 * Voltaje: Lee del ADC real (pin A0)
 * Resistencia: SIMULADO - genera valores aleatorios entre 1kΩ y 10kΩ
 *
 * Usa este código para probar que la app funciona correctamente antes de
 * construir el circuito de resistencia completo.
 */

// ========== CONFIGURACIÓN DE PINES ==========
#define RELAY_VOLTAGE_PIN 22
#define ADC_VOLTAGE_PIN A0
#define RELAY_RESISTANCE_PIN 23
#define ADC_RESISTANCE_PIN A1

// ========== CONSTANTES ==========
const float VREF = 5.0;
const float ADC_RESOLUTION = 1023.0;
const int NUM_SAMPLES = 50;
const int SAMPLE_DELAY = 2;
const float VOLTAGE_DIVIDER_FACTOR = 6.0;

// ========== MODOS ==========
enum MeasurementMode {
  MODE_NONE,
  MODE_VOLTAGE,
  MODE_RESISTANCE
};

MeasurementMode currentMode = MODE_NONE;

// Calibración
float voltageCalibrationOffset = 0.0;
float voltageCalibrationGain = 1.0;

// Variables para simulación de resistencia
float simulatedResistance = 4700.0;  // Valor inicial: 4.7kΩ
unsigned long lastResistanceChange = 0;

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_VOLTAGE_PIN, OUTPUT);
  pinMode(ADC_VOLTAGE_PIN, INPUT);
  pinMode(RELAY_RESISTANCE_PIN, OUTPUT);
  pinMode(ADC_RESISTANCE_PIN, INPUT);

  digitalWrite(RELAY_VOLTAGE_PIN, LOW);
  digitalWrite(RELAY_RESISTANCE_PIN, LOW);

  analogReference(DEFAULT);

  // Inicializar random seed
  randomSeed(analogRead(A2));

  Serial.println("{\"status\":\"ready\",\"module\":\"multimeter_TEST\",\"version\":\"2.0-SIMULATED\",\"modes\":[\"voltage\",\"resistance\"]}");

  delay(100);
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
        float voltage = measureVoltage();
        sendVoltageData(voltage);
        delay(100);
      }
      break;

    case MODE_RESISTANCE:
      {
        float resistance = measureResistanceSimulated();
        sendResistanceData(resistance);
        delay(100);
      }
      break;

    default:
      delay(10);
      break;
  }
}

// ========== COMANDOS ==========
void processCommand(String command) {
  if (command == "START_VOLTAGE") {
    activateVoltageMode();
  }
  else if (command == "STOP_VOLTAGE") {
    deactivateVoltageMode();
  }
  else if (command == "START_RESISTANCE") {
    activateResistanceMode();
  }
  else if (command == "STOP_RESISTANCE") {
    deactivateResistanceMode();
  }
  else if (command == "GET_STATUS") {
    sendStatus();
  }
  else if (command == "GET_RAW_ADC") {
    sendRawADC();
  }
  else if (command.startsWith("SET_SIM_RESISTANCE:")) {
    // Comando especial para cambiar la resistencia simulada
    // Ejemplo: SET_SIM_RESISTANCE:1000 (para 1kΩ)
    simulatedResistance = command.substring(19).toFloat();
    Serial.print("{\"status\":\"simulation_updated\",\"resistance\":");
    Serial.print(simulatedResistance, 1);
    Serial.println("}");
  }
  else {
    Serial.println("{\"error\":\"unknown_command\",\"received\":\"" + command + "\"}");
  }
}

// ========== MODO VOLTAJE (REAL) ==========
void activateVoltageMode() {
  deactivateAllModes();
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

// ========== MODO RESISTENCIA (SIMULADO) ==========
void activateResistanceMode() {
  deactivateAllModes();
  digitalWrite(RELAY_RESISTANCE_PIN, HIGH);
  currentMode = MODE_RESISTANCE;
  delay(50);
  Serial.println("{\"status\":\"resistance_mode_active\",\"note\":\"SIMULATED MODE\"}");
}

void deactivateResistanceMode() {
  digitalWrite(RELAY_RESISTANCE_PIN, LOW);
  if (currentMode == MODE_RESISTANCE) {
    currentMode = MODE_NONE;
  }
  Serial.println("{\"status\":\"resistance_mode_inactive\"}");
}

float measureResistanceSimulated() {
  // Cambiar el valor simulado cada 5 segundos
  if (millis() - lastResistanceChange > 5000) {
    lastResistanceChange = millis();

    // Generar un valor aleatorio entre 1kΩ y 10kΩ
    simulatedResistance = random(1000, 10000);

    // Agregar algo de variación para simular ruido
    simulatedResistance += random(-50, 50);
  }

  // Agregar pequeño ruido en cada lectura (±20Ω)
  float resistance = simulatedResistance + random(-20, 20);

  // Simular circuito abierto aleatoriamente (10% de probabilidad)
  // if (random(0, 10) == 0) {
  //   return -1.0;  // Circuito abierto
  // }

  return resistance;
}

void sendResistanceData(float resistance) {
  Serial.print("{\"type\":\"resistance\",\"value\":");

  if (resistance < 0) {
    Serial.print("\"OL\"");
  } else if (resistance < 1000) {
    Serial.print(resistance, 1);
  } else if (resistance < 1000000) {
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
  Serial.print(",\"simulated\":true");
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
      Serial.print("resistance_SIMULATED");
      break;
    default:
      Serial.print("none");
      break;
  }

  Serial.print("\",\"simulated_resistance\":");
  Serial.print(simulatedResistance, 1);
  Serial.println("}");
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
  Serial.print(",\"note\":\"Resistance is SIMULATED\"");
  Serial.println("}");
}
