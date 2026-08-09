# Biblioteca `ServoDriver` — Capa de hardware para servos

**Autor:** Prof. Fernando Angel Liozzi — 2026  
**Archivos:** `ServoDriver.h` / `ServoDriver.cpp`  
**Dependencia:** `madhephaestus/ESP32Servo @ ^3.2.1`

---

## 1. Descripción general

`ServoDriver` encapsula un servo conectado al ESP32-S3 usando la biblioteca
`ESP32Servo`. El periférico utilizado es **MCPWM** (no LEDC), por lo que no
interfiere con los canales LEDC 0–3 reservados para `DifferentialDrive`.

Cada instancia maneja un único servo con:

- Escritura directa de ángulo (`writeAngle`)
- Mapeo **centro→extremo** del joystick al rango calibrado
- Suavizado continuo con velocidad controlada por el gatillo **R2**

---

## 2. API pública

| Método                                                         | Descripción                                         |
| -------------------------------------------------------------- | --------------------------------------------------- |
| `begin(pin)`                                                   | Inicializa el servo: 50 Hz, pulso 544–2400 µs       |
| `writeAngle(angulo)`                                           | Escribe un ángulo absoluto (0–180°). Sin suavizado  |
| `writeFromJoystick(eje, zero, span, ejeLY)`                    | Mapeo centro→extremo sin suavizado (calibración)    |
| `writeFromJoystickSmooth(eje, zero, span, r2, ejeLY, enCalib)` | Mapeo con suavizado continuo. R2 controla velocidad |
| `getCurrentAngle()`                                            | Ángulo actual (0–180°) según el último suavizado    |
| `detach()`                                                     | Libera el servo                                     |

---

## 3. Mapeo centro→extremo

El stick de un gamepad vuelve solo al centro (~128). Para no perder recorrido
al soltarlo, el mapeo usa solo la mitad del stick:

| Eje                 | Parámetro `ejeLY` | Reposo (128) | Extremo                | Fórmula                   |
| ------------------- | ----------------- | ------------ | ---------------------- | ------------------------- |
| **LY** (vertical)   | `true`            | 0°           | 0 (arriba) → `span`    | `(128−eje) × rango / 128` |
| **LX** (horizontal) | `false`           | 0°           | 255 (derecha) → `span` | `(eje−128) × rango / 127` |

- Stick en reposo o dirección opuesta → servo se queda en `zero`.
- Al soltar, el servo vuelve solo a la posición `zero`.

---

## 4. Suavizado continuo (`writeFromJoystickSmooth`)

Para evitar movimientos bruscos, la función interpola suavemente el ángulo
comandado hacia el ángulo objetivo.

### Funcionamiento

1. Se actualiza como máximo cada **5 ms** (200 Hz), independientemente de la
   velocidad del loop.
2. En cada tick, el ángulo avanza hacia el objetivo en pasos controlados por **R2**.
3. Resolución interna: **×16** (~0.06°).

### Control de velocidad por R2

| Modo            | R2=0      | R2=255    | Relación   |
| --------------- | --------- | --------- | ---------- |
| **Normal**      | 1.8°/tick | 0.4°/tick | 1× → 1/5×  |
| **Calibración** | 1.8°/tick | 0.2°/tick | 1× → 1/10× |

Fórmula (solo enteros):

```
paso = 29 − ((29 − mínimo) × R2) / 255
```

Donde `mínimo = 6` en modo normal y `mínimo = 3` en calibración.

### Barrido completo (0°→180°) según R2

| R2  | Normal | Calibración |
| --- | ------ | ----------- |
| 0   | ~0.5 s | ~0.5 s      |
| 128 | ~0.8 s | ~0.9 s      |
| 255 | ~2.4 s | ~4.8 s      |

---

## 5. Ejemplo de uso

```cpp
#include "ServoDriver.h"

ServoDriver servo1;
servo1.begin(14);  // GPIO 14 (SAT1)

// Modo normal con suavizado y R2
servo1.writeFromJoystickSmooth(gamepad.ly, 0, 180, gamepad.r2, true, false);

// Enclavar en posición actual
uint8_t angulo = servo1.getCurrentAngle();
servo1.writeAngle(angulo);

// Escritura directa con mapeo centro→extremo
servo1.writeFromJoystick(gamepad.ly, 45, 135, true);
```
