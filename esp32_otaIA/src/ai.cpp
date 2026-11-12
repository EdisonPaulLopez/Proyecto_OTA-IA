/**
 * @file ai.cpp
 * @brief Inferencia de la regresión logística y diagnóstico de fallas del sensor.
 */
#include "ai.h"
#include "config.h"
#include "model.h"
#include <math.h>

float riskProbability(float temp, float hum) {
  // 1) Estandarizar las entradas con la media y desviación del entrenamiento
  const float xt = (temp - MODEL_MEAN_T) / MODEL_STD_T;
  const float xh = (hum - MODEL_MEAN_H) / MODEL_STD_H;

  // 2) Combinación lineal
  const float z = MODEL_W_T * xt + MODEL_W_H * xh + MODEL_BIAS;

  // 3) Función sigmoide
  return 1.0f / (1.0f + expf(-z));
}

SensorStatus diagnoseSensor(float temp, float hum) {
  static SensorStatus status = SensorStatus::OK;
  static uint8_t nanCount = 0;    // lecturas sin respuesta seguidas
  static uint8_t goodStreak = 0;  // lecturas buenas seguidas durante una falla
  static bool haveLast = false;
  static float lastT = 0.0f, lastH = 0.0f;

  // 1) El sensor no respondió: probable problema de cable
  if (isnan(temp) || isnan(hum)) {
    goodStreak = 0;
    haveLast = false;  // al volver a responder no se compara con una lectura vieja
    if (++nanCount >= FAULT_NAN_COUNT) status = SensorStatus::CABLE_FALLA;
    return status;
  }
  nanCount = 0;

  // 2) Respondió, pero ¿la lectura es creíble?
  const bool outOfRange = temp < SENSOR_T_MIN || temp > SENSOR_T_MAX || hum < 0.0f || hum > 100.0f;
  const bool jump = haveLast && (fabsf(temp - lastT) > MAX_JUMP_T || fabsf(hum - lastH) > MAX_JUMP_H);
  lastT = temp;
  lastH = hum;
  haveLast = true;

  if (outOfRange || jump) {
    goodStreak = 0;
    status = SensorStatus::SENSOR_FALLA;
    return status;
  }

  // 3) Lectura buena: la falla se da por resuelta tras varias seguidas
  if (status != SensorStatus::OK && ++goodStreak >= FAULT_RECOVER_COUNT) {
    status = SensorStatus::OK;
    goodStreak = 0;
  }
  return status;
}
