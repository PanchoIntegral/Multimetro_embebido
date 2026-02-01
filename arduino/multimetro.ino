// Multímetro Industrial - Arduino Mega
// Sistema de medición con 6 modos: Voltaje, Corriente, Resistencia, Capacitancia, Continuidad, Frecuencia

// Definición de pines para relés
#define RELAY_VOLTAGE 22
#define RELAY_CURRENT 24
#define RELAY_RESISTANCE 26
#define RELAY_CAPACITANCE 28
#define RELAY_CONTINUITY 30
#define RELAY_FREQUENCY 32

// Pin de entrada analógica
#define ANALOG_PIN A0

// Variables globales
String currentMode = "VOLTAGE";

void setup() {
  // Inicializar comunicación serial
  Serial.begin(115200);
  
  // Configurar pines de relés como salidas
  pinMode(RELAY_VOLTAGE, OUTPUT);
  pinMode(RELAY_CURRENT, OUTPUT);
  pinMode(RELAY_RESISTANCE, OUTPUT);
  pinMode(RELAY_CAPACITANCE, OUTPUT);
  pinMode(RELAY_CONTINUITY, OUTPUT);
  pinMode(RELAY_FREQUENCY, OUTPUT);
  
  // Iniciar en modo voltaje
  setMode("VOLTAGE");
  
  Serial.println("Sistema de Multímetro Iniciado");
}

void loop() {
  // Leer comandos del serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("MODE:")) {
      String newMode = command.substring(5);
      setMode(newMode);
    }
  }
  
  // Realizar medición según el modo actual
  float value = measureValue();
  float uncertainty = value * 0.04; // 4% de incertidumbre
  
  // Enviar datos en formato: "TYPE:value:uncertainty"
  Serial.print(currentMode);
  Serial.print(":");
  Serial.print(value, 2);
  Serial.print(":");
  Serial.println(uncertainty, 2);
  
  delay(100); // 10 lecturas por segundo
}

void setMode(String mode) {
  // Apagar todos los relés
  digitalWrite(RELAY_VOLTAGE, LOW);
  digitalWrite(RELAY_CURRENT, LOW);
  digitalWrite(RELAY_RESISTANCE, LOW);
  digitalWrite(RELAY_CAPACITANCE, LOW);
  digitalWrite(RELAY_CONTINUITY, LOW);
  digitalWrite(RELAY_FREQUENCY, LOW);
  
  // Activar el relé correspondiente
  if (mode == "VOLTAGE") {
    digitalWrite(RELAY_VOLTAGE, HIGH);
  } else if (mode == "CURRENT") {
    digitalWrite(RELAY_CURRENT, HIGH);
  } else if (mode == "RESISTANCE") {
    digitalWrite(RELAY_RESISTANCE, HIGH);
  } else if (mode == "CAPACITANCE") {
    digitalWrite(RELAY_CAPACITANCE, HIGH);
  } else if (mode == "CONTINUITY") {
    digitalWrite(RELAY_CONTINUITY, HIGH);
  } else if (mode == "FREQUENCY") {
    digitalWrite(RELAY_FREQUENCY, HIGH);
  }
  
  currentMode = mode;
}

float measureValue() {
  // Leer valor analógico (0-1023)
  int rawValue = analogRead(ANALOG_PIN);
  
  // Convertir a voltaje (0-5V)
  float voltage = rawValue * (5.0 / 1023.0);
  
  // Escalar según el modo de medición
  if (currentMode == "VOLTAGE") {
    // Rango 0-20V (factor de escala 4x)
    return voltage * 4.0;
    
  } else if (currentMode == "CURRENT") {
    // Rango 0-1A (usando sensor de corriente)
    // Asumiendo sensor ACS712-5A: 185mV/A, centrado en 2.5V
    float current = (voltage - 2.5) / 0.185;
    return abs(current);
    
  } else if (currentMode == "RESISTANCE") {
    // Rango 0-100kΩ
    // Usando divisor de voltaje con resistencia conocida de 10kΩ
    float R_known = 10000.0; // 10kΩ
    if (voltage < 0.01) return 0; // Evitar división por cero
    float resistance = R_known * ((5.0 / voltage) - 1.0);
    return constrain(resistance, 0, 100000);
    
  } else if (currentMode == "CAPACITANCE") {
    // Rango 0-50µF
    // Método de carga/descarga (simplificado)
    // En implementación real, medir tiempo de carga
    return voltage * 10.0; // Placeholder - requiere implementación específica
    
  } else if (currentMode == "CONTINUITY") {
    // Continuidad: 0 = no continuidad, 1 = continuidad
    return (voltage > 4.0) ? 1.0 : 0.0;
    
  } else if (currentMode == "FREQUENCY") {
    // Rango 0-10kHz
    // Requiere contador de pulsos o interrupción
    // Placeholder - requiere implementación específica
    return voltage * 2000.0; // Simulación
  }
  
  return voltage;
}

// Función auxiliar para medición de capacitancia (implementación completa)
float measureCapacitance() {
  // Descargar el capacitor
  pinMode(ANALOG_PIN, OUTPUT);
  digitalWrite(ANALOG_PIN, LOW);
  delay(10);
  
  // Configurar como entrada y medir tiempo de carga
  pinMode(ANALOG_PIN, INPUT);
  unsigned long startTime = micros();
  
  // Esperar a que alcance ~63.2% de carga (constante de tiempo RC)
  while (analogRead(ANALOG_PIN) < 648) { // 648 ≈ 63.2% de 1023
    if (micros() - startTime > 5000000) { // Timeout 5s
      return 0;
    }
  }
  
  unsigned long elapsedTime = micros() - startTime;
  
  // C = t / R (donde R es la resistencia conocida en el circuito)
  float R_charge = 10000.0; // 10kΩ
  float capacitance = (float)elapsedTime / R_charge; // en µF
  
  return capacitance;
}

// Función auxiliar para medición de frecuencia (implementación completa)
float measureFrequency() {
  unsigned long pulseCount = 0;
  unsigned long startTime = millis();
  unsigned long duration = 1000; // Medir durante 1 segundo
  
  int lastState = digitalRead(ANALOG_PIN);
  
  while (millis() - startTime < duration) {
    int currentState = digitalRead(ANALOG_PIN);
    if (currentState != lastState && currentState == HIGH) {
      pulseCount++;
    }
    lastState = currentState;
  }
  
  // Frecuencia = pulsos / tiempo
  float frequency = pulseCount; // Hz (ya que medimos 1 segundo)
  
  return frequency;
}
