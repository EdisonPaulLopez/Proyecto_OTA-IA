/**
 * @file ota.cpp
 * @brief Implementación de OTA con verificación SHA-256 progresiva.
 */
#include "ota.h"
#include "config.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>

/**
 * @brief Obtiene versión, hash y tamaño del firmware publicado en el servidor.
 * @param[out] version Versión publicada.
 * @param[out] sha     SHA-256 publicado (64 caracteres hexadecimales).
 * @param[out] size    Tamaño del binario en bytes.
 * @return true si la respuesta fue válida.
 */
static bool fetchInfo(String &version, String &sha, size_t &size) {
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(String(SERVER_URL) + "/api/version")) return false;
  if (http.GET() != 200) {
    http.end();
    return false;
  }
  const String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  version = doc["version"].as<String>();
  sha = doc["sha256"].as<String>();
  size = doc["size"] | 0;
  return version.length() > 0 && sha.length() == 64 && size > 0;
}

/**
 * @brief Convierte un hash de 32 bytes a texto hexadecimal en minúsculas.
 * @param hash Hash binario.
 * @param out  Buffer de al menos 65 bytes.
 */
static void toHex(const uint8_t *hash, char *out) {
  static const char *d = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    out[2 * i] = d[hash[i] >> 4];
    out[2 * i + 1] = d[hash[i] & 0x0F];
  }
  out[64] = '\0';
}

bool otaCheckAndUpdate() {
  String version, expectedSha;
  size_t size = 0;
  if (!fetchInfo(version, expectedSha, size)) {
    Serial.println("[OTA] No se pudo consultar el servidor");
    return false;
  }
  if (version == FW_VERSION) {
    Serial.println("[OTA] Firmware al dia");
    return false;
  }
  Serial.printf("[OTA] Nueva version %s (actual %s), %u bytes\n",
                version.c_str(), FW_VERSION, (unsigned)size);

  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(String(SERVER_URL) + "/api/firmware") || http.GET() != 200 ||
      (size_t)http.getSize() != size) {
    Serial.println("[OTA] Error al iniciar la descarga");
    http.end();
    return false;
  }
  if (!Update.begin(size)) {  // prepara la particion OTA inactiva
    Serial.println("[OTA] No hay espacio para la actualizacion");
    http.end();
    return false;
  }

  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);  // 0 = SHA-256

  auto fail = [&](const char *msg) {
    Serial.printf("[OTA] Error: %s\n", msg);
    Update.abort();
    mbedtls_sha256_free(&ctx);
    http.end();
    return false;
  };

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buf[1024];
  size_t written = 0;
  uint32_t lastData = millis();

  while ((http.connected() || stream->available()) && written < size) {
    const size_t avail = stream->available();
    if (!avail) {
      if (millis() - lastData > 10000) break;  // sin datos por 10 s
      delay(1);
      continue;
    }
    const int n = stream->readBytes(buf, avail > sizeof(buf) ? sizeof(buf) : avail);
    if (n <= 0) continue;
    if (Update.write(buf, n) != (size_t)n) return fail("escritura en flash");
    mbedtls_sha256_update_ret(&ctx, buf, n);  // hash progresivo
    written += n;
    lastData = millis();
  }
  http.end();

  if (written != size) return fail("descarga incompleta");

  uint8_t hash[32];
  mbedtls_sha256_finish_ret(&ctx, hash);
  mbedtls_sha256_free(&ctx);
  char hex[65];
  toHex(hash, hex);

  expectedSha.toLowerCase();
  if (expectedSha != String(hex)) {
    Serial.println("[OTA] Hash SHA-256 invalido: firmware descartado");
    Update.abort();  // la particion activa no se modifica
    return false;
  }
  if (!Update.end(true)) {
    Serial.println("[OTA] Error al finalizar la actualizacion");
    return false;
  }
  Serial.println("[OTA] Firmware verificado e instalado");
  return true;
}
