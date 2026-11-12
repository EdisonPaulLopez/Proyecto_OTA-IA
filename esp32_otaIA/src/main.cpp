/**
 * @file main.cpp
 * @brief ESP32 + DHT22 + LEDs: OTA e IA (regresión logística) para gestión del dispositivo.
 *
 * - Lee temperatura y humedad del DHT22.
 * - La regresión logística calcula la probabilidad de condición de riesgo.
 *   Riesgo -> LED rojo y OTA bloqueado. Normal -> LED verde y OTA permitido.
 * - La IA diagnostica fallas del sensor:
 *   falla de cable -> LED rojo parpadeando; falla de sensor -> LED rojo parpadeando rápido.
 *   Con cualquier falla el OTA se aplaza.
 * - Periódicamente consulta al servidor si hay firmware nuevo y lo instala (LED amarillo).
 */
#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include "config.h"
#include "ai.h"
#include "ota.h"

static DHT dht(PIN_DHT, DHT22);     ///< Sensor DHT22 de temperatura y humedad
static bool risk = false;           ///< Resultado de la IA en la última lectura
static SensorStatus sensorStatus = SensorStatus::OK;  ///< Diagnóstico de la IA
static uint32_t lastSample = 0;     ///< Instante de la última lectura
static uint32_t lastOtaCheck = 0;   ///< Instante de la última consulta OTA
static uint32_t lastBlink = 0;      ///< Instante del último cambio del parpadeo
static bool blinkOn = false;        ///< Fase actual del parpadeo del LED rojo

/**
 * @brief Enciende los LEDs indicados y apaga los demás.
 * @param green  Estado del LED verde.
 * @param yellow Estado del LED amarillo.
 * @param red    Estado del LED rojo.
 */
static void setLeds(bool green, bool yellow, bool red) {
  digitalWrite(PIN_LED_GREEN, green);
  digitalWrite(PIN_LED_YELLOW, yellow);
  digitalWrite(PIN_LED_RED, red);
}

/**
 * @brief Hace parpadear el LED rojo mientras haya una falla (sin bloquear el programa).
 *
 * Falla de cable: parpadeo a BLINK_CABLE_MS. Falla de sensor: parpadeo rápido a BLINK_SENSOR_MS.
 * @param now Tiempo actual en ms (millis()).
 */
static void updateFaultLed(uint32_t now) {
  if (sensorStatus == SensorStatus::OK) return;
  const uint32_t period =
      (sensorStatus == SensorStatus::CABLE_FALLA) ? BLINK_CABLE_MS : BLINK_SENSOR_MS;
  if (now - lastBlink >= period) {
    lastBlink = now;
    blinkOn = !blinkOn;
    setLeds(false, false, blinkOn);
  }
}

/** @brief Conecta a la red WiFi (espera hasta 15 s). */
static void connectWifi() {
  Serial.printf("[WiFi] Conectando a %s...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(250);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Conectado, IP %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[WiFi] Sin conexion (se reintentara)");
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\n=== Firmware %s ===\n", FW_VERSION);

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  setLeds(false, false, false);

  dht.begin();
  connectWifi();

  lastOtaCheck = millis() - OTA_CHECK_MS + 5000UL;  // primer chequeo a los 5 s
}

void loop() {
  const uint32_t now = millis();
  updateFaultLed(now);

  // --- IA: diagnosticar el sensor y, si está bien, clasificar el riesgo ---
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    const float t = dht.readTemperature();
    const float h = dht.readHumidity();
    sensorStatus = diagnoseSensor(t, h);

    if (sensorStatus == SensorStatus::CABLE_FALLA) {
      Serial.println("[DIAG] FALLA DE CABLE: el DHT22 no responde (LED rojo parpadeando)");
    } else if (sensorStatus == SensorStatus::SENSOR_FALLA) {
      Serial.printf("[DIAG] FALLA DE SENSOR: lecturas imposibles T=%.1f H=%.1f (LED rojo rapido)\n",
                    t, h);
    } else if (!isnan(t) && !isnan(h)) {
      const float p = riskProbability(t, h);
      risk = (p >= RISK_THRESHOLD);
      setLeds(!risk, false, risk);
      Serial.printf("T=%.1fC H=%.1f%% p(riesgo)=%.2f -> %s\n",
                    t, h, p, risk ? "RIESGO" : "NORMAL");
    } else {
      Serial.println("[DHT22] Sin respuesta (reintentando)");
    }
  }

  // --- OTA: solo si el sensor está bien y la IA indica condición normal ---
  if (now - lastOtaCheck >= OTA_CHECK_MS) {
    lastOtaCheck = now;
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    } else if (sensorStatus != SensorStatus::OK) {
      Serial.println("[OTA] Aplazado: el sensor tiene una falla");
    } else if (risk) {
      Serial.println("[OTA] Aplazado: la IA detecta condicion de riesgo");
    } else {
      setLeds(false, true, false);  // amarillo: actualizando
      if (otaCheckAndUpdate()) {
        delay(500);
        ESP.restart();
      }
      setLeds(!risk, false, risk);
    }
  }
}
