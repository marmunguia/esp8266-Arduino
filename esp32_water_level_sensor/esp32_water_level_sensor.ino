/*
 * ============================================================================
 *  ESP32 - Water Level Sensor ALS-MPM-2F
 * ============================================================================
 *  Sensor: ALS-MPM-2F (Submersible Level Transmitter)
 *    - Output:       4-20 mA
 *    - Supply:       24 VDC
 *    - Range:        0 - 5 m
 *
 *  Conversion 4-20 mA → Voltaje:
 *    Se usa una resistencia de 150 Ω en paralelo con el ADC del ESP32.
 *      4  mA × 150 Ω = 0.60 V  (nivel 0 m)
 *      20 mA × 150 Ω = 3.00 V  (nivel 5 m)
 *    Esto es seguro para el ADC del ESP32 (máx 3.3V).
 *
 * ============================================================================
 *  DIAGRAMA DE CONEXIONES
 * ============================================================================
 *
 *   +----------+          +----------------+         +-----------+
 *   | Fuente   |          | Sensor         |         |   ESP32   |
 *   | 24 VDC   |          | ALS-MPM-2F     |         |           |
 *   |          |          | (4-20 mA)      |         |           |
 *   |  (+24V)--+----+---->| Vcc (+)        |         |           |
 *   |          |    |     |                |         |           |
 *   |          |    |     | Signal (-)--+--+----+--->| GPIO 34   |
 *   |          |    |     +----------------+    |    | (ADC1_CH6)|
 *   |          |    |                          _|_   |           |
 *   |          |    |                         |   |  |           |
 *   |          |    |                  150 Ω  |   |  |           |
 *   |          |    |                         |___|  |           |
 *   |          |    |                           |    |           |
 *   |   (GND)--+----+------ GND ---------------+--->| GND       |
 *   +----------+                                     |           |
 *                                                    |   [LCD    |
 *                                          (3.3V) <--| 3V3  I2C]|
 *                                                    |           |
 *                                    SDA (LCD) <-----| GPIO 21   |
 *                                    SCL (LCD) <-----| GPIO 22   |
 *                                                    +-----------+
 *
 *  NOTA: La resistencia de 150 Ω va entre GPIO34 y GND.
 *        El sensor es un lazo de corriente: +24V → sensor → resistencia → GND.
 *        El LCD I2C (opcional) muestra el nivel en tiempo real.
 *
 * ============================================================================
 *  NOTAS IMPORTANTES:
 *  - El GND del ESP32 DEBE estar conectado al GND de la fuente de 24V.
 *  - NO conectar los 24V directamente al ESP32.
 *  - Usar resistencia de precisión (1%) para mejor exactitud.
 *  - El ESP32 se alimenta por USB o por su propio regulador (5V/3.3V).
 * ============================================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── Configuración de pines ──────────────────────────────────────────────────
#define SENSOR_PIN       34    // GPIO34 (ADC1_CH6, solo lectura analógica)

// ── Parámetros del sensor ALS-MPM-2F ────────────────────────────────────────
#define SENSOR_RANGE_M   5.0   // Rango del sensor: 0 a 5 metros
#define R_SHUNT          150.0 // Resistencia shunt en ohmios

// Voltajes correspondientes a 4 mA y 20 mA con R = 150 Ω
#define V_AT_4MA   (0.004 * R_SHUNT)  // 0.60 V → nivel 0 m
#define V_AT_20MA  (0.020 * R_SHUNT)  // 3.00 V → nivel 5 m

// ── Parámetros del ADC del ESP32 ────────────────────────────────────────────
#define ADC_RESOLUTION   4095.0
#define ADC_VREF         3.3

// ── Configuración de promedios ──────────────────────────────────────────────
#define NUM_SAMPLES      20    // Número de muestras para promediar
#define SAMPLE_DELAY_MS  10    // Delay entre muestras (ms)

// ── Intervalos ──────────────────────────────────────────────────────────────
#define READ_INTERVAL_MS  1000  // Leer sensor cada 1 segundo
#define SERIAL_BAUD       115200

// ── Alarmas de nivel ────────────────────────────────────────────────────────
#define LEVEL_LOW_M       0.5  // Alarma nivel bajo (metros)
#define LEVEL_HIGH_M      4.5  // Alarma nivel alto (metros)

// ── LCD I2C (opcional, comentar si no se usa) ───────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Dirección 0x27, 16 columnas, 2 filas
bool lcdAvailable = false;

// ── Variables globales ──────────────────────────────────────────────────────
unsigned long lastReadTime = 0;
float currentLevel_m   = 0.0;
float currentCurrent_mA = 0.0;
float currentVoltage_V  = 0.0;
float tankPercentage    = 0.0;

// ── Caracteres personalizados para barra de nivel en LCD ────────────────────
byte barFull[8]  = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
byte barEmpty[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// ════════════════════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("=============================================");
  Serial.println("  ESP32 - Sensor de Nivel de Agua");
  Serial.println("  Sensor: ALS-MPM-2F (4-20 mA, 0-5 m)");
  Serial.println("=============================================");

  // Configurar ADC
  analogReadResolution(12);            // Resolución de 12 bits (0-4095)
  analogSetAttenuation(ADC_11db);      // Rango completo ~0-3.3V

  // Inicializar LCD
  Wire.begin();
  Wire.beginTransmission(0x27);
  if (Wire.endTransmission() == 0) {
    lcdAvailable = true;
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, barFull);
    lcd.createChar(1, barEmpty);
    lcd.setCursor(0, 0);
    lcd.print("Nivel de Agua");
    lcd.setCursor(0, 1);
    lcd.print("Iniciando...");
    Serial.println("LCD I2C detectado en 0x27");
  } else {
    Serial.println("LCD I2C no detectado (opcional)");
  }

  Serial.println();
  Serial.print("Resistencia shunt: ");
  Serial.print(R_SHUNT, 0);
  Serial.println(" ohm");
  Serial.print("V @ 4mA:  ");
  Serial.print(V_AT_4MA, 3);
  Serial.println(" V");
  Serial.print("V @ 20mA: ");
  Serial.print(V_AT_20MA, 3);
  Serial.println(" V");
  Serial.println("---------------------------------------------");
  Serial.println();

  delay(2000);
}

// ════════════════════════════════════════════════════════════════════════════
//  LOOP
// ════════════════════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;

    // 1. Leer voltaje promedio del ADC
    currentVoltage_V = readAverageVoltage();

    // 2. Convertir voltaje a corriente (mA)
    currentCurrent_mA = (currentVoltage_V / R_SHUNT) * 1000.0;

    // 3. Convertir corriente a nivel de agua (metros)
    currentLevel_m = mapCurrentToLevel(currentCurrent_mA);

    // 4. Calcular porcentaje del tanque
    tankPercentage = (currentLevel_m / SENSOR_RANGE_M) * 100.0;
    tankPercentage = constrain(tankPercentage, 0.0, 100.0);

    // 5. Mostrar en Serial
    printSerial();

    // 6. Mostrar en LCD
    if (lcdAvailable) {
      updateLCD();
    }

    // 7. Verificar alarmas
    checkAlarms();
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  FUNCIONES
// ════════════════════════════════════════════════════════════════════════════

/**
 * Lee múltiples muestras del ADC y retorna el voltaje promedio.
 */
float readAverageVoltage() {
  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(SAMPLE_DELAY_MS);
  }
  float avgRaw = (float)sum / NUM_SAMPLES;
  return (avgRaw / ADC_RESOLUTION) * ADC_VREF;
}

/**
 * Convierte corriente (4-20 mA) a nivel de agua (0-5 m).
 * Valores fuera de rango se limitan.
 */
float mapCurrentToLevel(float current_mA) {
  // Mapeo lineal: 4 mA → 0 m, 20 mA → 5 m
  float level = ((current_mA - 4.0) / (20.0 - 4.0)) * SENSOR_RANGE_M;
  return constrain(level, 0.0, SENSOR_RANGE_M);
}

/**
 * Imprime los datos por el puerto serial.
 */
void printSerial() {
  Serial.print("ADC: ");
  Serial.print(currentVoltage_V, 3);
  Serial.print(" V | Corriente: ");
  Serial.print(currentCurrent_mA, 2);
  Serial.print(" mA | Nivel: ");
  Serial.print(currentLevel_m, 2);
  Serial.print(" m | Tanque: ");
  Serial.print(tankPercentage, 1);
  Serial.print(" %");

  // Indicador de estado
  if (currentCurrent_mA < 3.8) {
    Serial.print(" | [ERROR: Sin señal]");
  } else if (currentCurrent_mA > 20.5) {
    Serial.print(" | [ERROR: Sobrecarga]");
  }

  Serial.println();
}

/**
 * Actualiza la pantalla LCD con nivel y porcentaje.
 */
void updateLCD() {
  // Línea 1: Nivel en metros
  lcd.setCursor(0, 0);
  lcd.print("Nivel:");
  lcd.print(currentLevel_m, 2);
  lcd.print("m     ");

  // Línea 2: Porcentaje con barra visual
  lcd.setCursor(0, 1);
  lcd.print(tankPercentage, 0);
  lcd.print("% ");

  // Barra de progreso (10 caracteres)
  int bars = map((int)tankPercentage, 0, 100, 0, 10);
  for (int i = 0; i < 10; i++) {
    lcd.write(i < bars ? byte(0) : byte(1));
  }

  // Indicador de estado
  if (currentCurrent_mA < 3.8) {
    lcd.setCursor(14, 0);
    lcd.print("ER");
  }
}

/**
 * Verifica condiciones de alarma de nivel.
 */
void checkAlarms() {
  // Alarma: corriente fuera de rango (sensor desconectado o dañado)
  if (currentCurrent_mA < 3.8) {
    Serial.println("*** ALARMA: Sensor sin señal (<3.8 mA). Verificar conexiones. ***");
    return;
  }

  if (currentCurrent_mA > 20.5) {
    Serial.println("*** ALARMA: Corriente excesiva (>20.5 mA). Verificar sensor. ***");
    return;
  }

  // Alarma: nivel bajo
  if (currentLevel_m <= LEVEL_LOW_M) {
    Serial.print("*** ALARMA: Nivel BAJO (");
    Serial.print(currentLevel_m, 2);
    Serial.print(" m <= ");
    Serial.print(LEVEL_LOW_M, 1);
    Serial.println(" m) ***");
  }

  // Alarma: nivel alto
  if (currentLevel_m >= LEVEL_HIGH_M) {
    Serial.print("*** ALARMA: Nivel ALTO (");
    Serial.print(currentLevel_m, 2);
    Serial.print(" m >= ");
    Serial.print(LEVEL_HIGH_M, 1);
    Serial.println(" m) ***");
  }
}
