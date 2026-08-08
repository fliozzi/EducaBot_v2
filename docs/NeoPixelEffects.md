# Biblioteca `NeoPixelEffects` — Efectos de Iluminación WS2812

**Autor:** Prof. Fernando Angel Liozzi — 2026  
**Archivos:** `NeoPixelEffects.h` / `NeoPixelEffects.cpp`  
**Dependencia:** `Adafruit_NeoPixel`

---

## 1. Descripción general

`NeoPixelEffects` gestiona una **tira de 6 LEDs WS2812** (NeoPixel) con efectos de iluminación que acompañan el estado operativo del robot EducaBot. Cada efecto está asociado a una fase del ciclo de vida del robot:

| Efecto                    | Fase del robot            |
| ------------------------- | ------------------------- |
| 🌈 Arcoíris giratorio     | Escaneando joystick BLE   |
| 🟢 Pulso verde + apagado  | Conexión establecida      |
| 🔴 Indicadores rojo/ámbar | Control manual (bloqueos) |
| ⚫ Apagado total          | Ahorro de batería         |

---

## 2. Inicialización

```cpp
NeoPixelEffects leds;
leds.begin(pin, cantidad);
```

- `pin`: GPIO al que está conectada la tira (normalmente GPIO 4 en EducaBot).
- `cantidad`: número de LEDs (máximo 6; la biblioteca lo limita automáticamente).
- El brillo global por defecto es `64` (25%).
- La tira usa formato de color **GRB** y frecuencia **800 kHz**.

---

## 3. Efecto 1: Arcoíris giratorio (`animarEscaneo`)

### Propósito

Indicar visualmente que el robot está buscando un gamepad BLE. Es un efecto **hipnótico y llamativo** que facilita identificar el estado del robot a distancia.

### Funcionamiento

1. Se ejecuta a **40 fps** (25 ms entre frames).
2. Cada LED recibe un color distinto de la rueda de arcoíris, desplazado circularmente.
3. El tono base (`tonoBase`, 0–255) se incrementa en cada frame, produciendo un **giro continuo** que completa una vuelta cada ~6.4 segundos.
4. El brillo se fija a `brilloEscaneo = 10` (muy tenue, independiente del brillo global) para no deslumbrar.

### Rueda de arcoíris (implementación propia)

La función `ruedaArcoiris(pos)` genera un color RGB a partir de una posición 0–255 sin depender de `ColorHSV()` de Adafruit:

| Rango de `pos` | Rojo    | Verde   | Azul    |
| -------------- | ------- | ------- | ------- |
| 0–84           | ↓ 255→0 | 0       | ↑ 0→255 |
| 85–169         | 0       | ↑ 0→255 | ↓ 255→0 |
| 170–255        | ↑ 0→255 | ↓ 255→0 | 0       |

```
   R    G    B
   ██..............  pos=0   (rojo)
   ......██........  pos=85  (verde)
   ...........██...  pos=170 (azul)
```

---

## 4. Efecto 2: Pulso de conexión (`iniciarAnimConectado` + `actualizarAnimConectado`)

### Propósito

Celebrar visualmente que el gamepad se ha conectado y luego **apagar los LEDs** para ahorrar batería durante la operación normal.

### Fases

La animación tiene 3 fases controladas por el enum `FaseConectado`:

```
FC_NINGUNA → FC_PULSO_VERDE → FC_APAGANDO → FC_TERMINADO
```

#### Fase 1: `FC_PULSO_VERDE` (pulso verde del centro hacia afuera)

- Brillo base por LED (gradiente fijo):

| LED 0 | LED 1 | LED 2 | LED 3 | LED 4 | LED 5 |
| ----- | ----- | ----- | ----- | ----- | ----- |
| 40    | 90    | 180   | 180   | 90    | 40    |

- Una **onda de brillo extra** (+100) se desplaza del centro (LEDs 2–3) hacia los extremos (LEDs 0 y 5):

```
Frame 0:  [40] [90] [280▶] [280▶] [90] [40]   ← onda en centro
Frame 1:  [40] [190▶] [180] [180] [190▶] [40]  ← onda se expande
Frame 2:  [140▶] [90] [180] [180] [90] [140▶]  ← onda en extremos
```

- Se ejecuta a 25 fps (~40 ms/frame).
- El color es **verde puro** `(0, brillo, 0)`.

#### Fase 2: `FC_APAGANDO` (fade out)

- El gradiente verde se mantiene fijo.
- El brillo global (`setBrightness`) se reduce en 4 unidades cada 30 ms.
- Cuando el brillo ≤ 4, se dispara la fase final.

#### Fase 3: `FC_TERMINADO`

- Se llama a `apagar()` → todos los LEDs a 0.
- `animConectadoTerminada()` retorna `true`.
- Durante la operación normal del robot, los LEDs permanecen apagados (ahorro de batería) a menos que se active `mostrarEstadoControl()`.

### Uso

```cpp
// En el momento de detectar la conexión:
leds.iniciarAnimConectado();

// En cada loop():
if (!leds.animConectadoTerminada()) {
  leds.actualizarAnimConectado();
}
```

---

## 5. Efecto 3: Indicadores de estado (`mostrarEstadoControl`)

### Propósito

Durante el control manual del robot, mostrar **señales visuales mínimas** para indicar bloqueos activos. Solo se encienden los LEDs necesarios; el resto permanece apagado.

### Indicadores

| Condición       | LED                  | Color                        | Significado               |
| --------------- | -------------------- | ---------------------------- | ------------------------- |
| `bloqueoGiro`   | LED 0 (extremo izq.) | 🔴 Rojo fijo                 | Giro bloqueado            |
| `bloqueoAvance` | LED 5 (extremo der.) | 🔴 Rojo fijo                 | Avance/reversa bloqueados |
| `paroTotal`     | LEDs 1 y 4           | 🟠 Ámbar intermitente (2 Hz) | Paro total enclavado      |

El parpadeo ámbar usa `(millis() / 500) % 2` para alternar encendido/apagado cada 500 ms.

---

## 6. Control de brillo

La biblioteca mantiene dos niveles de brillo independientes:

| Variable        | Uso                                           | Valor por defecto |
| --------------- | --------------------------------------------- | ----------------- |
| `brilloGlobal`  | Animación de conexión e indicadores de estado | `64`              |
| `brilloEscaneo` | Animación de arcoíris (escaneo)               | `10`              |

- `setBrightness(brillo)` modifica `brilloGlobal` y lo aplica inmediatamente a la tira.
- El brillo de escaneo es fijo y no se ve afectado por `setBrightness()` — esto evita que el fade-out de la animación de conexión deje un brillo residual en el escaneo.

---

## 7. API pública

| Método                                                        | Descripción                                    |
| ------------------------------------------------------------- | ---------------------------------------------- |
| `begin(pin, cantidad)`                                        | Inicializa la tira NeoPixel                    |
| `setBrightness(brillo)`                                       | Ajusta brillo global (0–255)                   |
| `animarEscaneo()`                                             | Ejecuta un frame del arcoíris giratorio        |
| `iniciarAnimConectado()`                                      | Dispara la animación de pulso verde            |
| `actualizarAnimConectado()`                                   | Actualiza un frame; retorna `true` al terminar |
| `animConectadoTerminada()`                                    | `true` si la animación ya finalizó             |
| `mostrarEstadoControl(bloqueoGiro, bloqueoAvance, paroTotal)` | Indicadores de bloqueo                         |
| `apagar()`                                                    | Apaga todos los LEDs inmediatamente            |

---

## 8. Ejemplo de uso completo

```cpp
#include "NeoPixelEffects.h"

NeoPixelEffects leds;

void setup() {
  leds.begin(4, 6);       // GPIO 4, 6 LEDs
  leds.setBrightness(64); // 25% brillo
}

void loop() {
  if (!joystick.conectado()) {
    // Fase de búsqueda: arcoíris giratorio
    leds.animarEscaneo();
    return;
  }

  // ¿Recién conectado?
  if (recienConectado) {
    leds.iniciarAnimConectado();
    recienConectado = false;
  }

  // Actualizar animación de conexión mientras dure
  if (!leds.animConectadoTerminada()) {
    leds.actualizarAnimConectado();
    return;
  }

  // Operación normal: indicadores según botones
  const GamepadData& g = joystick.estado();
  bool bloqueoGiro = g.botonPresionado(GamepadData::BTN_L1);
  bool bloqueoAvance = g.botonPresionado(GamepadData::BTN_R1);
  bool paroTotal = g.botonPresionado(GamepadData::BTN_Y);
  leds.mostrarEstadoControl(bloqueoGiro, bloqueoAvance, paroTotal);
}
```

---

## 9. Resumen visual de LEDs

```
   LED0    LED1    LED2    LED3    LED4    LED5
   [0]     [1]     [2]     [3]     [4]     [5]
    🔴      🟠      🟢      🟢      🟠      🔴
  bloqueo  paro   pulso   pulso   paro   bloqueo
   giro   total  conex.  conex.  total   avance
```

- **LEDs 2 y 3 (centro):** protagonistas de la animación de conexión.
- **LEDs 1 y 4:** parpadeo ámbar de paro total.
- **LEDs 0 y 5:** indicadores rojos de bloqueo.
