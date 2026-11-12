"""!
@file server.py
@brief Servidor OTA (FastAPI) que corre en el PC.

Endpoints:
  - POST /api/publish  : publica un firmware nuevo (versión + archivo .bin).
  - GET  /api/version  : versión, SHA-256 y tamaño del firmware publicado.
  - GET  /api/firmware : descarga del .bin publicado.

Ejecutar desde la carpeta server/:
    uvicorn server:app --host 0.0.0.0 --port 8000
"""
import hashlib
import json
from pathlib import Path

from fastapi import FastAPI, File, Form, HTTPException, UploadFile
from fastapi.responses import FileResponse

app = FastAPI(title="Servidor OTA ESP32")

FW_DIR = Path(__file__).parent / "firmware"
FW_DIR.mkdir(exist_ok=True)
FW_BIN = FW_DIR / "firmware.bin"
FW_META = FW_DIR / "meta.json"


@app.post("/api/publish")
async def publish(version: str = Form(...), file: UploadFile = File(...)):
    """!
    @brief Publica un firmware nuevo.
    @param version Versión del firmware (debe coincidir con FW_VERSION del .bin).
    @param file Archivo .bin generado por PlatformIO.
    @return Metadatos publicados: version, sha256 y size.
    """
    data = await file.read()
    if not data:
        raise HTTPException(status_code=400, detail="Archivo vacío")
    FW_BIN.write_bytes(data)
    meta = {"version": version, "sha256": hashlib.sha256(data).hexdigest(), "size": len(data)}
    FW_META.write_text(json.dumps(meta))
    return meta


@app.get("/api/version")
def get_version():
    """!
    @brief Devuelve la información del firmware publicado.
    @return Diccionario con version, sha256 y size.
    """
    if not FW_META.exists():
        raise HTTPException(status_code=404, detail="No hay firmware publicado")
    return json.loads(FW_META.read_text())


@app.get("/api/firmware")
def get_firmware():
    """!
    @brief Descarga el binario publicado (con Content-Length para validar el tamaño).
    @return Archivo binario.
    """
    if not FW_BIN.exists():
        raise HTTPException(status_code=404, detail="No hay firmware publicado")
    return FileResponse(FW_BIN, media_type="application/octet-stream")
