# Biblioteca `ServoDriver` — Capa de hardware para servos

**Autor:** Prof. Fernando Angel Liozzi — 2026  
**Archivos:** `ServoDriver.h` / `ServoDriver.cpp`  
**Dependencia:** `madhephaestus/ESP32Servo @ ^3.2.1`

---

## 1. Descripción general

`ServoDriver` es una capa delgada sobre `ESP32Servo` que abstrae la
inicialización y el mapeo joystick→ángulo. En el ESP32-S3 usa automáticamente
el periférico **MCPWM** (no LEDC), por lo que no interfiere con los canales
LEDC 0–3 usados por `DifferentialDrive`.

---

## 2. API pública

| Método | Descripción |
|--------|-------------|
| `begin(pin)` | Inicializa el servo: 50 Hz, pulso 544–2400 µs |
| `writeAngle(angulo)` | Escribe un ángulo absoluto (0–180°) |
| `writeFromJoystick(eje, zero, span)` | Mapea eje (0–255) al rango calibrado [zero, span] |
| `detach()` | Libera el servo (corta la señal PWM) |

---

## 3. Mapeo joystick → ángulo

```
eje (0–255)  ──▶  interpolación lineal  ──▶  ángulo (zero–span)

  Si zero < span:  ángulo = zero + (eje × (span - zero)) / 255
  Si zero > span:  ángulo = span + (eje × (zero - span)) / 255
```

El resultado se limita a [0, 180].

---

## 4. Ejemplo de uso

```cpp
#include "ServoDriver.h"

ServoDriver servo1;
servo1.begin(14);

// Mapear joystick LY (0–255) al rango calibrado 45°–135°
servo1.writeFromJoystick(gamepad.ly, 45, 135);
```
