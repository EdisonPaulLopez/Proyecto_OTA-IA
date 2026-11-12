/**
 * @file ota.h
 * @brief Actualización de firmware Over-The-Air (OTA) desde el servidor del PC.
 *
 * Flujo: consulta /api/version; si hay una versión distinta, descarga /api/firmware,
 * lo escribe en la partición OTA inactiva (esquema A/B) y verifica su SHA-256
 * calculado de forma progresiva. Si el hash no coincide, se aborta y se conserva
 * el firmware actual.
 */
#pragma once

/**
 * @brief Busca firmware nuevo en el servidor y lo instala si es válido.
 * @return true si se instaló un firmware nuevo (el llamador debe reiniciar);
 *         false si no había actualización o si ocurrió un error.
 */
bool otaCheckAndUpdate();
