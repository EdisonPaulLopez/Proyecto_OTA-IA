"""!
@file train.py
@brief Entrena una regresión logística (temperatura, humedad) -> riesgo y genera include/model.h.

Uso:
    python train.py              # usa datos sintéticos de ejemplo
    python train.py datos.csv    # usa lecturas propias (columnas: temp,hum,etiqueta con 0 normal / 1 riesgo)

No requiere librerías externas.
"""
import csv
import math
import random
import sys
from pathlib import Path

random.seed(42)


def synthetic_data(n_per_class=300):
    """!
    @brief Genera datos de ejemplo: ambiente normal (0) y ambiente caluroso y húmedo de riesgo (1).
    @return Lista de tuplas (temp, hum, etiqueta).
    """
    data = []
    for _ in range(n_per_class):
        data.append((random.gauss(24, 2.5), random.gauss(50, 8), 0))
        data.append((random.gauss(34, 3.0), random.gauss(80, 7), 1))
    return data


def load_csv(path):
    """!
    @brief Carga lecturas etiquetadas desde un CSV (temp,hum,etiqueta).
    @param path Ruta del archivo CSV con encabezado.
    @return Lista de tuplas (temp, hum, etiqueta).
    """
    data = []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            data.append((float(row["temp"]), float(row["hum"]), int(row["etiqueta"])))
    return data


def sigmoid(z):
    """!
    @brief Función sigmoide numéricamente estable.
    @param z Valor de entrada.
    @return 1 / (1 + e^-z).
    """
    if z >= 0:
        return 1.0 / (1.0 + math.exp(-z))
    e = math.exp(z)
    return e / (1.0 + e)


def train(data, lr=0.1, epochs=3000):
    """!
    @brief Ajusta la regresión logística por descenso de gradiente.
    @param data Lista de (temp, hum, etiqueta).
    @param lr Tasa de aprendizaje.
    @param epochs Número de iteraciones.
    @return Diccionario con medias, desviaciones, pesos, sesgo y exactitud.
    """
    n = len(data)
    mt = sum(d[0] for d in data) / n
    mh = sum(d[1] for d in data) / n
    st = math.sqrt(sum((d[0] - mt) ** 2 for d in data) / n) or 1.0
    sh = math.sqrt(sum((d[1] - mh) ** 2 for d in data) / n) or 1.0
    xs = [((d[0] - mt) / st, (d[1] - mh) / sh, d[2]) for d in data]

    wt = wh = b = 0.0
    for _ in range(epochs):
        gt = gh = gb = 0.0
        for xt, xh, y in xs:
            err = sigmoid(wt * xt + wh * xh + b) - y
            gt += err * xt
            gh += err * xh
            gb += err
        wt -= lr * gt / n
        wh -= lr * gh / n
        b -= lr * gb / n

    correct = sum(1 for xt, xh, y in xs if (sigmoid(wt * xt + wh * xh + b) >= 0.5) == (y == 1))
    return {"mt": mt, "st": st, "mh": mh, "sh": sh, "wt": wt, "wh": wh, "b": b, "acc": correct / n}


def write_header(m, path):
    """!
    @brief Escribe los parámetros del modelo en un encabezado C++ para el ESP32.
    @param m Diccionario devuelto por train().
    @param path Ruta de salida (include/model.h).
    """
    text = f"""/**
 * @file model.h
 * @brief Parámetros de la regresión logística (generado por train/train.py, no editar).
 * Exactitud de entrenamiento: {m['acc'] * 100:.1f} %
 */
#pragma once

#define MODEL_MEAN_T {m['mt']:.6f}f  ///< Media de temperatura del entrenamiento
#define MODEL_STD_T  {m['st']:.6f}f  ///< Desviación de temperatura
#define MODEL_MEAN_H {m['mh']:.6f}f  ///< Media de humedad
#define MODEL_STD_H  {m['sh']:.6f}f  ///< Desviación de humedad
#define MODEL_W_T    {m['wt']:.6f}f  ///< Peso de la temperatura
#define MODEL_W_H    {m['wh']:.6f}f  ///< Peso de la humedad
#define MODEL_BIAS   {m['b']:.6f}f  ///< Sesgo (bias)
"""
    Path(path).write_text(text)


if __name__ == "__main__":
    data = load_csv(sys.argv[1]) if len(sys.argv) > 1 else synthetic_data()
    model = train(data)
    out = Path(__file__).resolve().parent.parent / "include" / "model.h"
    write_header(model, out)
    print(f"Exactitud: {model['acc'] * 100:.1f} %")
    print(f"Pesos: w_t={model['wt']:.3f} w_h={model['wh']:.3f} b={model['b']:.3f}")
    print(f"Modelo guardado en {out}")
