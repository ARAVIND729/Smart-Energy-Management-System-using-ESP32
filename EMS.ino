/*********************************************************
 SMART ENERGY MANAGEMENT SYSTEM
 ESP32 + ACS712 + ZMPT101B + DHT11 + Relay + Blynk
 ML-ready (Edge Impulse compatible)
**********************************************************/

/* ---------------- BLYNK CONFIGURATION ---------------- */
#define BLYNK_TEMPLATE_ID   "TMPL3BeIPU9vq"
#define BLYNK_TEMPLATE_NAME "Energy Meter"
#define BLYNK_AUTH_TOKEN    "HPYVQzOjrHuOI6LABihvMGEaZ.mOgJA"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

/* ---------------- WIFI CREDENTIALS ---------------- */
char ssid[] = "Your_WiFi_Name";
char pass[] = "Your_WiFi_Password";

/* ---------------- PIN DEFINITIONS ---------------- */
// Analog pins
#define CURRENT_PIN 34        // ACS712
#define VOLTAGE_PIN 35        // ZMPT101B

// Digital pins
#define DHTPIN 4              // DHT11 data pin
#define RELAY_PIN 18          // Relay channel 1

/* ---------------- DHT CONFIG ---------------- */
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

/* ---------------- CONSTANTS ---------------- */
#define ADC_RESOLUTION 4095.0
#define REF_VOLTAGE 3.3

// ACS712 calibration (5A version)
#define ACS_OFFSET 2.5
#define ACS_SENSITIVITY 0.185   // V/A

// ZMPT101B calibration
#define VOLTAGE_CALIBRATION 230.0

/* ---------------- VARIABLES ---------------- */
float voltage_rms = 0.0;
float current_rms = 0.0;
float power = 0.0;
float temperature = 0.0;

bool anomaly_detected = false;
bool manual_relay = false;

BlynkTimer timer;

/* ---------------- SENSOR FUNCTIONS ---------------- */

// Read current from ACS712
float readCurrent() {
  int adc = analogRead(CURRENT_PIN);
  float voltage = (adc * REF_VOLTAGE) / ADC_RESOLUTION;
  float current = (voltage - ACS_OFFSET) / ACS_SENSITIVITY;
  return abs(current);
}

// Read voltage from ZMPT101B
float readVoltage() {
  int adc = analogRead(VOLTAGE_PIN);
  float voltage = (adc * REF_VOLTAGE) / ADC_RESOLUTION;
  return voltage * (VOLTAGE_CALIBRATION / REF_VOLTAGE);
}

// Read temperature from DHT11
float readTemperature() {
  return dht.readTemperature();
}

/* ---------------- ML LOGIC (PLACEHOLDER) ---------------- */
/*
 Replace this function with Edge Impulse inference later.
*/
bool runMLModel(float v, float i, float t) {
  if (i > 2.0 || t > 45.0) {   // Abnormal conditions
    return true;
  }
  return false;
}

/* ---------------- BLYNK BUTTON (MANUAL CONTROL) ---------------- */
BLYNK_WRITE(V4) {
  manual_relay = param.asInt();
  digitalWrite(RELAY_PIN, manual_relay ? HIGH : LOW);
}

/* ---------------- SENSOR + CONTROL TASK ---------------- */
void readSensorsAndControl() {

  voltage_rms = readVoltage();
  current_rms = readCurrent();
  temperature = readTemperature();
  power = voltage_rms * current_rms;

  anomaly_detected = runMLModel(voltage_rms, current_rms, temperature);

  // Automatic control (only if manual override OFF)
  if (!manual_relay) {
    if (anomaly_detected) {
      digitalWrite(RELAY_PIN, LOW);   // Turn OFF load
    } else {
      digitalWrite(RELAY_PIN, HIGH);  // Turn ON load
    }
  }

  // Send data to Blynk
  Blynk.virtualWrite(V0, voltage_rms);
  Blynk.virtualWrite(V1, current_rms);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, temperature);

  // Serial output (for debugging & ML data logging)
  Serial.print("V: "); Serial.print(voltage_rms);
  Serial.print(" | I: "); Serial.print(current_rms);
  Serial.print(" | P: "); Serial.print(power);
  Serial.print(" | T: "); Serial.print(temperature);
  Serial.print(" | Status: ");
  Serial.println(anomaly_detected ? "ABNORMAL" : "NORMAL");
}

/* ---------------- SETUP ---------------- */
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  dht.begin();

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Read sensors every 2 seconds
  timer.setInterval(2000L, readSensorsAndControl);

  Serial.println("Smart Energy Management System Started");
}

/* ---------------- LOOP ---------------- */
void loop() {
  Blynk.run();
  timer.run();
}
