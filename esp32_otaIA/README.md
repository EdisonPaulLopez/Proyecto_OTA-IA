# ESP32: OTA e IA con regresión logística

ESP32 con sensor DHT22 y tres LEDs. Proyecto PlatformIO (VS Code) con servidor FastAPI en el PC.

## Qué hace

1. **OTA:** el ESP32 consulta `GET /api/version`. Si la versión es distinta, descarga `/api/firmware`, lo escribe en la partición OTA inactiva (A/B) y verifica el **SHA-256** calculado progresivamente. Si el hash falla, se descarta y se conserva el firmware actual.
2. **IA para gestión del dispositivo:** una **regresión logística** estima la probabilidad de condición de riesgo (ambiente caluroso y húmedo):

   `p = sigmoide(w_t·x_t + w_h·x_h + b)`, con `x = (valor − media) / desviación`

   - `p < 0.5`: NORMAL, LED verde y OTA permitido.
   - `p ≥ 0.5`: RIESGO, LED rojo y OTA bloqueado.
   - LED amarillo durante una actualización OTA.
3. **Diagnóstico de fallas:** la IA revisa si las lecturas del DHT22 son creíbles y avisa con el LED rojo:

   | Falla | Cómo se detecta | LED rojo |
   |---|---|---|
   | Cable | El sensor no responde 3 lecturas seguidas (cable suelto o cortado) | Parpadea (500 ms) |
   | Sensor | Lecturas imposibles: fuera de rango (T −40 a 80 °C, H 0 a 100 %) o saltos bruscos (más de 10 °C o 30 % entre lecturas) | Parpadea rápido (150 ms) |

   Con una falla el OTA se aplaza. Cuando vuelven 3 lecturas buenas seguidas, el sistema se recupera solo.

El modelo se entrena en el PC con `train/train.py` (sin librerías externas), que genera `include/model.h`. En el ESP32 solo se ejecuta la inferencia. Los umbrales del diagnóstico están en `include/config.h`.

## Conexiones

| Componente | ESP32 |
|---|---|
| DHT22 VCC / GND | 3V3 / GND |
| DHT22 DATA | GPIO4 |
| LED verde (+220 Ω) | GPIO25 |
| LED amarillo (+220 Ω) | GPIO26 |
| LED rojo (+220 Ω) | GPIO27 |

## Pasos

1. Instalar en VS Code la extensión **PlatformIO IDE** y abrir esta carpeta.
2. (Opcional) Reentrenar el modelo: `python train/train.py` (o `python train/train.py datos.csv` con lecturas propias etiquetadas `temp,hum,etiqueta`).
3. Editar `include/config.h`: `WIFI_SSID`, `WIFI_PASS` y `SERVER_URL` (IP del PC, con `ipconfig`).
4. Servidor, desde `server/`:
   ```
   pip install -r requirements.txt
   uvicorn server:app --host 0.0.0.0 --port 8000
   ```
5. Cargar por USB el firmware 1.0.0 (Upload) y abrir el Serial Monitor.
6. Probar el OTA: cambiar `FW_VERSION` a `1.0.1`, compilar (Build) y publicar desde la raíz del proyecto:
   ```
   curl.exe -F "version=1.0.1" -F "file=@.pio/build/esp32dev/firmware.bin" http://localhost:8000/api/publish
   ```
7. Probar el riesgo: soplar sobre el DHT22 para subir temperatura y humedad. Pasa a RIESGO, enciende el LED rojo y el OTA se aplaza.
8. Probar la falla de cable: con el ESP32 funcionando, desconectar el cable DATA del DHT22. A los 9 s el LED rojo parpadea. Al reconectarlo, vuelve a la normalidad.
9. Probar la falla de sensor: bajar `MAX_JUMP_T` a `0.5f` en `config.h`, cargar el firmware y soplar sobre el DHT22. Al detectar un salto brusco, el LED rojo parpadea rápido. Después restaurar el valor `10.0f`.

## Documentación

`doxygen Doxyfile` genera `docs/html/index.html`.
