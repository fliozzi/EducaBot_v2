# Bibliotecas `JoystickBLE` y `GamepadData` — Control BLE por Gamepad

**Autor:** Prof. Fernando Angel Liozzi — 2026  
**Archivos:**

- `JoystickBLE.h` / `JoystickBLE.cpp` — conexión y gestión BLE
- `GamepadData.h` / `GamepadData.cpp` — parseo y visualización del estado

---

## 1. Descripción general

Estas dos bibliotecas trabajan en conjunto para conectar un **gamepad Bluetooth Low Energy (BLE)** al ESP32 y decodificar sus datos en tiempo real.

- **`JoystickBLE`** → Maneja todo el ciclo de vida BLE: escaneo, conexión, reconexión, suscripción a notificaciones HID.
- **`GamepadData`** → Estructura de datos pura que parsea el report HID de 10 bytes y ofrece funciones de depuración.

El gamepad objetivo es el **"GamePadPlus V3"** (genérico compatible con HID sobre BLE), pero la biblioteca acepta cualquier dispositivo que anuncie el servicio HID (`0x1812`) o cuyo nombre contenga "GamePad".

---

## 2. Arquitectura

```
┌─────────────────────────────────────────────────┐
│                    loop()                        │
│  joystick.manejar()  ←─ atiende eventos BLE     │
│       │                                         │
│       ▼                                         │
│  joystick.conectado() ?                         │
│       │                                         │
│       ▼ sí                                      │
│  GamepadData g = joystick.estado()              │
│  g.lx, g.ly, g.rx, g.ry, g.l2, g.r2,           │
│  g.dpad, g.botones                              │
│       │                                         │
│       ▼                                         │
│  drive.driveFromJoystick(g.rx, g.ry, g.l2)     │
│  leds.mostrarEstadoControl(...)                 │
└─────────────────────────────────────────────────┘
```

---

## 3. `JoystickBLE` — Gestión de la conexión BLE

### 3.1 Ciclo de vida

```
iniciar()
  │
  ├─ init BLE stack (NimBLE)
  ├─ deleteAllBonds() — borra vínculos anteriores
  └─ iniciarEscaneo()
        │
        ▼
   Escaneo activo (scan indefinido)
        │
        ▼ onScanResultado()
   ¿Nombre contiene "GamePad" O servicio HID?
        │ sí
        ▼
   Guarda dispositivo objetivo, detiene scan
   encontrado = true
        │
        ▼ manejar() (en loop)
   Espera 300 ms → conectarYExplorar()
        │
        ├─ connect(objetivo)
        ├─ secureConnection() — emparejamiento
        └─ Itera servicios BLE
              └─ subscribe() a características con notify
                    │
                    ▼ onNotificacion()
                  parseFromReport(datos, 10)
                  ↓
                  gamepad actualizado
        │
        ▼ onDisconnected()
   pedirReescaneo = true → volver a escanear
```

### 3.2 Configuración de seguridad

- **Sin intercambio de IRK** (Identity Resolving Key): evita que NimBLE intente resolver direcciones aleatorias privadas (RPA), lo que en versiones anteriores devolvía `00:00:00:00:00:00` y rompía la reconexión.
- Solo se intercambia **clave de cifrado** (ENC).
- Se borran todos los vínculos al iniciar para partir de un estado limpio.

### 3.3 Reconexión automática

- Al desconectarse, se activa `pedirReescaneo`.
- En el próximo `manejar()`, se borran bonds y se reinicia el escaneo.
- Cuando se re-encuentra el dispositivo, se reutiliza el objeto `NimBLEClient` (crear uno nuevo en cada intento agota los slots de conexión del stack BLE).

### 3.4 Modo verbose

Por defecto imprime en una línea compacta (ancho fijo) el estado del gamepad cada vez que cambia. Se puede desactivar con `joystick.setVerbose(false)`.

---

## 4. `GamepadData` — Parseo del estado del gamepad

### 4.1 Formato del report HID

El gamepad envía reports de **10 bytes** con la siguiente estructura:

| Byte | Campo         | Descripción                                                    |
| ---- | ------------- | -------------------------------------------------------------- |
| 0    | `lx`          | Joystick izquierdo, eje X (0–255, centro ≈ 128)                |
| 1    | `ly`          | Joystick izquierdo, eje Y (0–255)                              |
| 2    | `rx`          | Joystick derecho, eje X (0–255)                                |
| 3    | `ry`          | Joystick derecho, eje Y (0–255)                                |
| 4    | `dpad`        | Cruceta digital (0–7 = direcciones, 255 = ninguna)             |
| 5–6  | `botones`     | Máscara de botones (16 bits, little-endian: byte bajo primero) |
| 7    | `r2`          | Gatillo derecho (0–255, analógico)                             |
| 8    | `l2`          | Gatillo izquierdo (0–255, analógico)                           |
| 9    | _(reservado)_ | No usado                                                       |

### 4.2 Campos de la estructura

| Campo      | Tipo       | Descripción                                                  |
| ---------- | ---------- | ------------------------------------------------------------ |
| `lx`, `ly` | `uint8_t`  | Ejes del joystick izquierdo                                  |
| `rx`, `ry` | `uint8_t`  | Ejes del joystick derecho                                    |
| `dpad`     | `uint8_t`  | Cruceta: 0=U, 1=UR, 2=R, 3=DR, 4=D, 5=DL, 6=L, 7=UL, 255=OFF |
| `l2`, `r2` | `uint8_t`  | Gatillos analógicos (0=suelto, 255=a fondo)                  |
| `botones`  | `uint16_t` | Máscara de bits de botones digitales                         |

### 4.3 Máscara de botones

Cada bit del campo `botones` representa un botón físico:

| Bit | Constante | Botón                     |
| --- | --------- | ------------------------- |
| 0   | `0x0001`  | A                         |
| 1   | `0x0002`  | B                         |
| 3   | `0x0008`  | X                         |
| 4   | `0x0010`  | Y                         |
| 6   | `0x0040`  | L1                        |
| 7   | `0x0080`  | R1                        |
| 8   | `0x0100`  | L2 (digital)              |
| 9   | `0x0200`  | R2 (digital)              |
| 10  | `0x0400`  | Select                    |
| 11  | `0x0800`  | Start                     |
| 13  | `0x2000`  | L3 (presionar stick izq.) |
| 14  | `0x4000`  | R3 (presionar stick der.) |

> ⚠️ Los gatillos L2/R2 aparecen **dos veces**: como valor analógico (bytes 7–8) y como bit en la máscara de botones (indica si están presionados hasta el fondo con "click").

---

## 5. API pública

### `JoystickBLE`

| Método             | Descripción                                        |
| ------------------ | -------------------------------------------------- |
| `iniciar()`        | Inicializa stack BLE, borra bonds, arranca escaneo |
| `manejar()`        | Atiende eventos BLE pendientes. Llamar en `loop()` |
| `estado()`         | Retorna `const GamepadData&` con el último estado  |
| `conectado()`      | `true` si el gamepad está vinculado y listo        |
| `setVerbose(bool)` | Activa/desactiva telemetría por Serial             |

### `GamepadData`

| Método                               | Descripción                                         |
| ------------------------------------ | --------------------------------------------------- |
| `parseFromReport(datos, largo)`      | Parsea un report HID de 10 bytes                    |
| `printToSerial()`                    | Vuelca estado completo al Serial (varias líneas)    |
| `printCompact()`                     | Una sola línea de ancho fijo con `\r` (sobrescribe) |
| `nombreDpad(v)`                      | Convierte valor de cruceta a string ("U", "UR", …)  |
| `nombresBotones(mascara, buf, size)` | Llena `buf` con nombres de botones activos          |
| `botonPresionado(mascara)`           | `true` si el botón está presionado                  |

---

## 6. Ejemplo de uso

```cpp
#include "JoystickBLE.h"

JoystickBLE joystick;

void setup() {
  Serial.begin(115200);
  joystick.iniciar();
}

void loop() {
  joystick.manejar();

  if (!joystick.conectado()) {
    // Mostrar animación de búsqueda...
    return;
  }

  const GamepadData& g = joystick.estado();

  // Usar joystick derecho para controlar motores
  drive.driveFromJoystick(g.rx, g.ry, g.l2);

  // Verificar botones
  if (g.botonPresionado(GamepadData::BTN_Y)) {
    // Activar modo autónomo
  }
  if (g.botonPresionado(GamepadData::BTN_L1)) {
    // Bloquear giro
  }
}
```
