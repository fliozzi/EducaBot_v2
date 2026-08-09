# Biblioteca `ServoCalibration` — Calibración de servos con NVS

**Autor:** Prof. Fernando Angel Liozzi — 2026  
**Archivos:** `ServoCalibration.h` / `ServoCalibration.cpp`  
**Dependencias:** `ServoDriver`, `Adafruit_NeoPixel`, `Preferences`

---

## 1. Descripción general

`ServoCalibration` implementa una **máquina de estados** para calibrar el zero
y span de los servos SAT1 y SAT2, con persistencia en **NVS** (Preferences).

Durante la calibración los motores de tracción se desactivan y los 6 LEDs
WS2812 guían visualmente cada paso. El R2 funciona con velocidad reducida a
**1/10** para posicionamiento de precisión.

---

## 2. Máquina de estados

```
┌──────────┐  SELECT      ┌──────────┐   5s   ┌───────────┐   3s   ┌──────────┐   A   ┌──────────┐   A   ┌──────────┐
│  NORMAL  │ ──────────▶ │ CAL_WAIT │ ────▶ │ CAL_ENTER │ ────▶ │ CAL_ZERO │ ───▶ │ CAL_SPAN │ ───▶ │  NORMAL  │
│          │             │(operacion│       │ flash 6Hz │       │ LED① 🔴  │      │ LED② 🔴  │      │ guarda   │
└──────────┘             │  normal) │       │ START:    │       │ stick→   │      │ stick→   │      └──────────┘
  │    ▲                 └──────────┘       │ SAT1→SAT2 │       │ A guarda │      │ A guarda  │
  │    │                      │             │ SAT2→salir│       └──────────┘      └──────────┘
  │    │                      │ suelta SELECT└───────────┘
  │    │                      └──▶ NORMAL (cancela)
  │    │  A 5s      ┌───────────┐
  └───▶│ ─────────▶ │ CAL_CLEAR │ → flash rosa 3s → borra NVS → NORMAL
       └──────────── │           │
                     └───────────┘
```

| Estado      | Disparador        | LEDs                            | Acción                           |
| ----------- | ----------------- | ------------------------------- | -------------------------------- |
| `NORMAL`    | —                 | según `NeoPixelEffects`         | Mapeo con valores calibrados     |
| `CAL_CLEAR` | A 5 s en NORMAL   | 6 LEDs flash rosa ~6 Hz (3 s)   | Borra NVS, vuelve a defaults     |
| `CAL_WAIT`  | SELECT presionado | normal                          | Espera 5 s; si suelta → cancela  |
| `CAL_ENTER` | SELECT 5 s        | 6 LEDs flash blanco ~6 Hz (3 s) | START alterna servo              |
| `CAL_ZERO`  | fin de 3 s        | LED⑥ amarillo/azul + LED① rojo  | Stick mueve servo, A guarda zero |
| `CAL_SPAN`  | A en CAL_ZERO     | LED⑥ amarillo/azul + LED② rojo  | Stick mueve servo, A guarda span |

### Comportamiento de START

| Dónde                                 | Acción                                |
| ------------------------------------- | ------------------------------------- |
| Calibrando **SAT1**                   | Pasa a calibrar SAT2                  |
| Calibrando **SAT2**                   | Sale de calibración (vuelve a NORMAL) |
| START 2 s sostenido (cualquier servo) | Sale de calibración sin guardar       |

### Códigos de color de LEDs

| LED (índice)  | Color                     | Significado                   |
| ------------- | ------------------------- | ----------------------------- |
| 6 LEDs (0–5)  | ⚪ blanco ~6 Hz           | Entrando en calibración (3 s) |
| 6 LEDs (0–5)  | 🩷 rosa ~6 Hz             | Borrando calibración (3 s)    |
| LED ⑥ (idx 5) | 🟡 amarillo `(255,200,0)` | Calibrando **SAT1**           |
| LED ⑥ (idx 5) | 🔵 azul `(0,0,255)`       | Calibrando **SAT2**           |
| LED ① (idx 0) | 🔴 rojo                   | Esperando posición de zero    |
| LED ① (idx 0) | 🟢 verde (breve)          | Zero guardado                 |
| LED ② (idx 1) | 🔴 rojo                   | Esperando posición de span    |
| LED ② (idx 1) | 🟢 verde (breve)          | Span guardado                 |

---

## 3. Persistencia en NVS

Namespace: `"servocal"`.

| Clave       | Tipo       | Default | Descripción                             |
| ----------- | ---------- | ------- | --------------------------------------- |
| `sat1_zero` | `uint16_t` | 0       | Ángulo del servo 1 con stick en centro  |
| `sat1_span` | `uint16_t` | 180     | Ángulo del servo 1 con stick en extremo |
| `sat2_zero` | `uint16_t` | 0       | Ángulo del servo 2 con stick en centro  |
| `sat2_span` | `uint16_t` | 180     | Ángulo del servo 2 con stick en extremo |

Al borrar la calibración (A 5s), los valores se resetean a defaults y el
cambio toma efecto **inmediatamente** (sin necesidad de reiniciar).

---

## 4. API pública

| Método                                   | Descripción                                       |
| ---------------------------------------- | ------------------------------------------------- |
| `begin()`                                | Carga zero/span desde NVS                         |
| `manejar(gamepad, tira, servo1, servo2)` | Tick de la máquina de estados                     |
| `enCalibracion()`                        | `true` si está en cualquier estado de calibración |
| `getZero(idx)` / `getSpan(idx)`          | Valores calibrados (0=SAT1, 1=SAT2)               |
| `servoActivo()`                          | Índice del servo en calibración (0=SAT1, 1=SAT2)  |

---

## 5. Botones usados

| Botón   | Máscara  | Función                                   |
| ------- | -------- | ----------------------------------------- |
| SELECT  | `0x0400` | Entrar en calibración (5 s sostenido)     |
| START   | `0x0800` | Alternar SAT1↔SAT2 / Salir de calibración |
| A       | `0x0001` | Confirmar zero / span                     |
| A (5 s) | `0x0001` | Borrar calibración (en modo NORMAL)       |

---

## 6. Ejemplo de uso

```cpp
#include "ServoCalibration.h"
#include "ServoDriver.h"

ServoCalibration calib;
calib.begin();

void loop() {
  const GamepadData &g = joystick.estado();

  calib.manejar(g, leds.getStrip(), servo1, servo2);

  if (calib.enCalibracion()) {
    return; // no mover motores ni servos durante calibración
  }

  servo1.writeFromJoystickSmooth(g.ly, calib.getZero(0),
                                  calib.getSpan(0), g.r2, true);
  servo2.writeFromJoystickSmooth(g.lx, calib.getZero(1),
                                  calib.getSpan(1), g.r2, false);
}
```
