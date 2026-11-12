/**
 * @file config.h
 * @brief Configuración del proyecto: WiFi, servidor, pines, tiempos y diagnóstico de fallas.
 */
#pragma once

#define FW_VERSION "1.0.0"                     ///< Versión del firmware en ejecución

#define WIFI_SSID  "Edison"               ///< Nombre de la red WiFi (2.4 GHz)
#define WIFI_PASS  "012345678"                  ///< Clave de la red WiFi
#define SERVER_URL "http://10.185.99.34:8000"  ///< IP del PC con el servidor (sin / final)

#define PIN_DHT        4    ///< Pin de datos del DHT22
#define PIN_LED_GREEN  25   ///< LED verde: condición normal
#define PIN_LED_YELLOW 26   ///< LED amarillo: OTA en curso
#define PIN_LED_RED    27   ///< LED rojo: condición de riesgo o falla

#define SAMPLE_MS          3000UL   ///< Periodo de lectura del sensor
#define OTA_CHECK_MS       30000UL  ///< Periodo de consulta de firmware nuevo
#define RISK_THRESHOLD     0.5f     ///< Probabilidad a partir de la cual hay riesgo

/** @name Diagnóstico de fallas del sensor y su cableado */
///@{
#define FAULT_NAN_COUNT      3      ///< Lecturas sin respuesta seguidas para declarar falla de cable
#define FAULT_RECOVER_COUNT  3      ///< Lecturas buenas seguidas para dar la falla por resuelta
#define SENSOR_T_MIN        -40.0f  ///< Temperatura mínima posible del DHT22 (°C)
#define SENSOR_T_MAX         80.0f  ///< Temperatura máxima posible del DHT22 (°C)
#define MAX_JUMP_T           10.0f  ///< Salto máximo creíble de temperatura entre lecturas (°C)
#define MAX_JUMP_H           30.0f  ///< Salto máximo creíble de humedad entre lecturas (%)
#define BLINK_CABLE_MS       500UL  ///< Parpadeo del LED rojo por falla de cable
#define BLINK_SENSOR_MS      150UL  ///< Parpadeo rápido del LED rojo por falla de sensor
///@}
