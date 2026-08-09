# Biblioteca `ServoCalibration` — Calibración de servos con NVS

**Autor:** Prof. Fernando Angel Liozzi — 2026  
**Archivos:** `ServoCalibration.h` / `ServoCalibration.cpp`  
**Dependencias:** `ServoDriver`, `Adafruit_NeoPixel`, `Preferences`

---

## 1. Descripción general

`ServoCalibration` implementa una **máquina de estados** que permite calibrar
el zero y span de los dos servos de potencia (SAT1 y SAT2). Los valores se
persisten en **NVS** (Preferences) y se cargan automáticamente al iniciar.

Durante la calibración los motores de tracción se **desactivan** y los LEDs
WS2812 muestran el estado de cada paso.

---

## 2. Máquina de estados

```
NORMAL ──(SELECT 5s)──▶ CAL_ENTER ──(3s)──▶ CAL_ZERO ──(A)──▶ CAL_SPAN ──(A)──▶ NORMAL
                            │                                      │
                        START: toggle                          guarda en NVS
                        SAT1 ↔ SAT2
```

| Estado | Disparador | LEDs | Acción |
|--------|-----------|------|--------|
| `NORMAL` | — | según `NeoPixelEffects` | Mapeo con valores calibrados |
| `CAL_WAIT` | SELECT presionado | normal | Espera 5 s (si suelta → cancela) |
| `CAL_ENTER` | SELECT 5 s | 6 LEDs flash ~200 Hz blanco (3 s) | START alterna servo |
| `CAL_ZERO` | fin de 3 s | LED⑥ rojo/azul + LED① rojo | Mover stick, A guarda zero |
| `CAL_SPAN` | A en CAL_ZERO | LED⑥ rojo/azul + LED② rojo | Mover stick, A guarda span y sale |

### Códigos de color

| LED | Color | Significado |
|-----|-------|-------------|
| LED ⑥ (idx 5) | 🔴 rojo | Calibrando **SAT1** |
| LED ⑥ (idx 5) | 🔵 azul | Calibrando **SAT2** |
| LED ① (idx 0) | 🔴 rojo | Esperando posición de zero |
| LED ① (idx 0) | 🟢 verde | Zero guardado (breve) |
| LED ② (idx 1) | 🔴 rojo | Esperando posición de span |
| LED ② (idx 1) | 🟢 verde | Span guardado (breve) |
| 6 LEDs | ⚪ blanco ~200 Hz | Modo calibración activo (3 s) |

---

## 3. Persistencia en NVS

| Clave | Tipo | Default | Descripción |
|-------|------|---------|-------------|
| `sat1_zero` | `uint16_t` | 0 | Ángulo mínimo servo 1 |
| `sat1_span` | `uint16_t` | 180 | Ángulo máximo servo 1 |
| `sat2_zero` | `uint16_t` | 0 | Ángulo mínimo servo 2 |
| `sat2_span` | `uint16_t` | 180 | Ángulo máximo servo 2 |

Namespace NVS: `"servocal"`.

---

## 4. API pública

| Método | Descripción |
|--------|-------------|
| `begin()` | Carga zero/span desde NVS |
| `manejar(gamepad, tira, servo1, servo2)` | Tick de la máquina de estados |
| `enCalibracion()` | `true` si está calibrando (bloquear tracción) |
| `getZero(idx)` / `getSpan(idx)` | Valores calibrados (0=SAT1, 1=SAT2) |
| `servoActivo()` | Índice del servo en calibración |

---

## 5. Ejemplo de uso

```cpp
#include "ServoCalibration.h"
#include "ServoDriver.h"

ServoCalibration calib;
calib.begin();

void loop() {
  const GamepadData &g = joystick.estado();

  // Tick de calibración (maneja LEDs y movimiento de servo)
  calib.manejar(g, leds.getStrip(), servo1, servo2);

  if (calib.enCalibracion()) {
    return; // no mover motores durante calibración
  }

  // Mapeo normal con valores calibrados
  servo1.writeFromJoystick(g.ly, calib.getZero(0), calib.getSpan(0));
  servo2.writeFromJoystick(g.lx, calib.getZero(1), calib.getSpan(1));
}
```

---

## 6. Botones usados

| Botón | Máscara | Función |
|-------|---------|---------|
| SELECT | `0x0400` | Entrar en calibración (5 s) |
| START | `0x0800` | Alternar SAT1 ↔ SAT2 |
| A | `0x0001` | Confirmar zero / span |
