# Biblioteca `DifferentialDrive` — Tracción Diferencial

**Autor:** Prof. Fernando Angel Liozzi — 2026  
**Archivos:** `DifferentialDrive.h` / `DifferentialDrive.cpp`

---

## 1. Descripción general

`DifferentialDrive` es una biblioteca para controlar **dos motores DC** en configuración de tracción diferencial (tipo _tanque_ o _robot de 2 ruedas_). Implementa una **mezcla arcade** a partir de un único joystick analógico, permitiendo gobernar avance, reversa y giro simultáneo.

Está diseñada para usarse con el driver de motor **L298N** (o similar de puente H) y utiliza los canales **LEDC** (PWM por hardware) del ESP32.

---

## 2. Estructuras de datos

### 2.1 `MotorPins`

Define los pines y canales PWM de un motor individual:

| Campo       | Tipo      | Descripción                               |
| ----------- | --------- | ----------------------------------------- |
| `in1`       | `uint8_t` | Pin de entrada 1 del puente H             |
| `in2`       | `uint8_t` | Pin de entrada 2 del puente H             |
| `channel1`  | `uint8_t` | Canal LEDC para `in1`                     |
| `channel2`  | `uint8_t` | Canal LEDC para `in2`                     |
| `invertido` | `bool`    | Si es `true`, invierte el sentido de giro |

### 2.2 `Config`

Parámetros de configuración de la tracción:

| Campo               | Valor por defecto | Descripción                                       |
| ------------------- | ----------------- | ------------------------------------------------- |
| `pwmFrequency`      | `10000`           | Frecuencia PWM en Hz                              |
| `pwmResolutionBits` | `8`               | Resolución del PWM (8 bits → 0–255)               |
| `pwmMax`            | `255`             | Valor máximo de PWM (100% potencia)               |
| `pwmPrecisionMax`   | `150`             | PWM máximo en modo precisión (gatillo L2 a fondo) |
| `joystickCenter`    | `128`             | Valor central del joystick (reposo)               |
| `joystickDeadzone`  | `16`              | Zona muerta alrededor del centro                  |
| `decayMode`         | `Slow`            | Modo de decaimiento del puente H                  |
| `brakeAtZero`       | `false`           | Si es `true`, frena activamente en vez de soltar  |

### 2.3 `DecayMode` (enum)

Controla qué sucede en los terminales del motor durante el tiempo de APAGADO del PWM.

- **`Slow` (decaimiento lento de corriente):**
  Durante el OFF, ambos terminales del motor van a Vs → 0 V diferencial → la corriente recircula por el motor con pérdidas mínimas. La corriente **decae muy lentamente** y el motor mantiene torque entre pulsos.
  - ✅ Movimiento **muy suave** a bajas velocidades, respuesta lineal.
  - ✅ Mayor torque sostenido (la corriente nunca llega a cero).
  - ❌ Mayor consumo (la corriente promedio es más alta).
  - ❌ Menor frenado: el motor tiende a "seguir" por inercia si se baja el PWM.

- **`Fast` (decaimiento rápido de corriente):**
  Durante el OFF, ambos terminales van a GND → el motor queda **cortocircuitado** → la f.e.m. inversa del motor disipa energía rápido. La corriente **decae velozmente**.
  - ✅ Menor consumo (corriente promedio más baja).
  - ✅ Mayor **frenado dinámico**: el cortocircuito durante OFF actúa como freno, el motor se detiene más rápido al soltar el joystick.
  - ✅ Mejor respuesta a cambios bruscos de comando (alta velocidad).
  - ❌ Menos suave a bajas velocidades (más ripple de torque, posible ruido audible).

---

## 3. Flujo de control (`driveFromJoystick`)

La función principal `driveFromJoystick(rx, ry, l2)` recibe tres valores crudos del gamepad (0–255) y los transforma en comandos PWM para cada motor:

```
     rx, ry, l2 (uint8_t, 0–255)
              │
   ┌──────────┴──────────┐
   │ axisToSigned()      │  → convierte valor centrado en joystick
   │                     │    a valor signed con deadzone
   ▼                     ▼
  avance (int16_t)     giro (int16_t)
   │                     │
   └──────────┬──────────┘
              ▼
   motorIzq = avance + giro
   motorDer = avance − giro
              │
              ▼
   normalización (escalado para que
   ningún motor sature, manteniendo
   la relación de velocidades)
              │
              ▼
   applyL2Attenuation() → atenúa con L2:
      L2=0   → pwmMax (máxima potencia)
      L2=255 → pwmPrecisionMax (precisión)
              │
              ▼
   writeMotor() × 2 → PWM a los pines
```

### 3.1 Conversión de eje (`axisToSigned`)

1. Resta el centro del joystick (`joystickCenter = 128`).
2. Si el valor absoluto está dentro de la zona muerta (`≤ 16`), retorna `0`.
3. Re-escala el rango útil a `[-255, 255]` usando `map()`.
4. Opcionalmente invierte el signo (el eje RX del gamepad se invierte para que el giro sea intuitivo).

### 3.2 Normalización

Para evitar que la suma `avance + giro` sature (supere ±255), se escala proporcionalmente:

$$\text{factor} = \frac{255}{\max(|\text{motorIzq}|, |\text{motorDer}|, 255)}$$

Esto preserva la **relación de velocidades** entre ambas ruedas (la trayectoria curva se mantiene).

### 3.3 Atenuación por gatillo L2

El gatillo L2 actúa como control de precisión. Interpola linealmente el PWM máximo entre `pwmMax` (255) y `pwmPrecisionMax` (150):

$$\text{salidaMax} = 255 - \frac{(255 - 150) \times L2}{255}$$

A L2=0 (gatillo suelto) la salida máxima es 255 (velocidad total).  
A L2=255 (gatillo a fondo) la salida máxima es 150 (~59% → movimientos finos).

---

## 4. Escritura a los motores (`writeMotor`)

1. Si el comando es `0`:
   - Con `brakeAtZero`: ambos canales del motor reciben `pwmMax` → **freno activo** (cortocircuito del motor).
   - Sin `brakeAtZero`: ambos canales a `0` → **motor libre** (sin par).

2. Si el comando es positivo o negativo:
   - Se determina el sentido (`adelante` o `atrás`) considerando el flag `invertido`.
   - Se asigna el canal **activo** (el que recibe el PWM) y el **inactivo**.
   - Según `decayMode`:
     - **Fast:** canal activo = `pwm`, canal inactivo = `0`. Durante el OFF del PWM ambos terminales van a GND → el motor se **cortocircuita** → frenado dinámico, la corriente decae rápido.
     - **Slow:** canal activo = `pwmMax` (siempre HIGH), canal inactivo = `pwmMax - pwm`. Durante el OFF ambos terminales van a Vs → la corriente **recircula** sin pérdidas → torque sostenido, movimiento suave.

---

## 5. Ejemplo de uso

```cpp
#include "DifferentialDrive.h"

// Motor izquierdo:  IN1=26, IN2=27, canales LEDC 0 y 1
// Motor derecho:    IN1=14, IN2=12, canales LEDC 2 y 3, invertido
DifferentialDrive::MotorPins izq(26, 27, 0, 1);
DifferentialDrive::MotorPins der(14, 12, 2, 3, true);

DifferentialDrive::Config cfg;
cfg.pwmMax = 255;
cfg.pwmPrecisionMax = 150;
cfg.decayMode = DifferentialDrive::DecayMode::Slow;
cfg.brakeAtZero = true;

DifferentialDrive drive(izq, der, cfg);

void setup() {
  drive.begin();
}

void loop() {
  // Leer gamepad...
  uint8_t rx = gamepad.estado().rx;
  uint8_t ry = gamepad.estado().ry;
  uint8_t l2 = gamepad.estado().l2;
  drive.driveFromJoystick(rx, ry, l2);
}
```

---

## 6. API pública

| Método                             | Descripción                                               |
| ---------------------------------- | --------------------------------------------------------- |
| `DifferentialDrive(izq, der)`      | Constructor con configuración por defecto                 |
| `DifferentialDrive(izq, der, cfg)` | Constructor con configuración personalizada               |
| `begin()`                          | Inicializa pines y canales LEDC                           |
| `stop()`                           | Detiene ambos motores (freno o libre según `brakeAtZero`) |
| `brake()`                          | Freno activo en ambos motores                             |
| `driveFromJoystick(rx, ry, l2)`    | Mezcla arcade con atenuación por gatillo                  |
| `setDecayMode(mode)`               | Cambia modo de decaimiento en caliente                    |
| `setPrecisionPwmMax(pwm)`          | Cambia el PWM máximo de precisión en caliente             |
