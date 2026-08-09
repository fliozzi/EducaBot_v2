# Enclavamiento de servos

**Autor:** Prof. Fernando Angel Liozzi — 2026

---

## 1. Descripción

El enclavamiento permite **fijar un servo en una posición** y liberarlo del
control del joystick. Útil para mantener una orientación sin necesidad de
sostener el stick.

---

## 2. Controles

| Botón | Servo          | LED indicador    | Color                    |
| ----- | -------------- | ---------------- | ------------------------ |
| **X** | SAT1 (GPIO 14) | LED ③ (índice 3) | 🟣 Violeta `(255,0,255)` |
| **B** | SAT2 (GPIO 15) | LED ② (índice 2) | 🔴 Rojo `(255,0,0)`      |

- **Primera pulsación**: guarda el ángulo actual y fija el servo.
- **Segunda pulsación**: libera el servo (vuelve al mapeo joystick).

---

## 3. Comportamiento

| Estado    | Servo                                  | LED                           |
| --------- | -------------------------------------- | ----------------------------- |
| Libre     | Responde al joystick con suavizado R2  | Apagado                       |
| Enclavado | Mantiene el ángulo fijo (`writeAngle`) | Encendido (color según servo) |

Al enclavar, se usa `getCurrentAngle()` para leer la posición suavizada actual.
Al liberar, el servo retoma el mapeo joystick→ángulo con suavizado.

---

## 4. Implementación

La lógica está en `main.cpp`:

```cpp
// Flanco de X: toggle enclavamiento SAT1
if (xPresionado && !xAnterior) {
  sat1Enclavado = !sat1Enclavado;
  if (sat1Enclavado) {
    sat1AnguloEnclavado = servo1.getCurrentAngle();
  }
}

// Mapeo condicional
if (sat1Enclavado) {
  servo1.writeAngle(sat1AnguloEnclavado);
} else {
  servo1.writeFromJoystickSmooth(g.ly, ...);
}

// LED indicador (se superpone a mostrarEstadoControl)
if (sat1Enclavado) {
  leds.getStrip().setPixelColor(3, Color(255, 0, 255));
}
```
