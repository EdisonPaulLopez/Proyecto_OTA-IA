/**
 * @file ai.h
 * @brief IA en el ESP32: riesgo con regresión logística y diagnóstico de fallas del sensor.
 *
 * 1) Riesgo. El modelo (entrenado con train/train.py y guardado en model.h) estima la
 *    probabilidad de "condición de riesgo" (ambiente caluroso y húmedo):
 *
 *      p = sigmoide(w_t * x_t + w_h * x_h + b),   x = (valor - media) / desviación
 *
 *    Con p >= RISK_THRESHOLD el dispositivo enciende el LED rojo y bloquea el OTA.
 *
 * 2) Diagnóstico. Revisa si las lecturas son creíbles y distingue:
 *    - falla de cable: el sensor no responde varias veces seguidas,
 *    - falla de sensor: lecturas imposibles (fuera de rango o saltos bruscos).
 */
#pragma once
#include <stdint.h>

/** @brief Estado de salud del sensor y su cableado. */
enum class SensorStatus : uint8_t {
  OK = 0,        ///< Lecturas normales
  CABLE_FALLA,   ///< Sin respuesta del sensor (cable suelto, cortado o mal conectado)
  SENSOR_FALLA   ///< El sensor responde con lecturas imposibles
};

/**
 * @brief Probabilidad de condición de riesgo según el modelo de regresión logística.
 * @param temp Temperatura en °C.
 * @param hum  Humedad relativa en %.
 * @return Probabilidad en el rango [0, 1].
 */
float riskProbability(float temp, float hum);

/**
 * @brief Diagnostica el estado del sensor con la lectura más reciente.
 *
 * Llamar una vez por lectura, incluso si el sensor no respondió (valores NaN).
 * - NaN durante FAULT_NAN_COUNT lecturas seguidas -> CABLE_FALLA.
 * - Valor fuera del rango físico o salto mayor a MAX_JUMP_T / MAX_JUMP_H -> SENSOR_FALLA.
 * - Tras FAULT_RECOVER_COUNT lecturas buenas seguidas vuelve a OK.
 *
 * @param temp Temperatura leída en °C (NaN si no hubo respuesta).
 * @param hum  Humedad leída en % (NaN si no hubo respuesta).
 * @return Estado actual del sensor.
 */
SensorStatus diagnoseSensor(float temp, float hum);
