# EducaBot — Lectura de joystick Bluetooth (BLE) con ESP32-S3

Proyecto en **Arduino puro** (PlatformIO) que conecta un joystick Bluetooth
tipo PlayStation al **ESP32-S3** y muestra por el monitor serie el estado de
todos los ejes, gatillos, D-pad y botones.

No usa librerías privadas ni de terceros para el gamepad: solo
`NimBLE-Arduino` como host BLE HID.

---

## Hardware

- **Placa:** ESP32-S3-WROOM-2 (solo tiene Bluetooth LE, no Bluetooth Classic).
- **Joystick:** genérico tipo PlayStation que se anuncia por BLE como
  `GamePadPlus V3`.
- **Modo del joystick:** encender con **Home + X**. En ese modo el mando se
  ofrece como HID por BLE y el ESP32-S3 lo toma.

> El mismo mando en otros combos de encendido puede ir por Bluetooth Classic,
> que el ESP32-S3 no soporta. Por eso se usa Home + X.

---

## Cómo funciona la conexión

Lo que hace falta para que enganche de forma confiable (ya resuelto en el
código):

1. `NimBLEDevice::setSecurityAuth(true, false, true)` — activa **bonding**. Sin
   emparejamiento cifrado, el gamepad HID no envía los reports de botones/ejes.
2. `NimBLEDevice::deleteAllBonds()` al arrancar — borra emparejamientos viejos.
   Un bond previo dejaba el anuncio con dirección `00:00:00:00:00:00` y la
   conexión fallaba con _"Invalid peer address"_.
3. Se guarda el **objeto** del anuncio y se conecta por él (no por la dirección
   extraída), así se conserva el tipo real de dirección aunque se vea nula.
4. Reconexión automática: si el mando se desconecta, vuelve a escanear solo.

Si alguna vez no conecta: olvidá el `GamePadPlus V3` en Windows/teléfono para
que no se reconecte a ellos, y reiniciá el joystick.

---

## Mapa del report HID

El mando envía por la característica **`0x2a4d`** (servicio HID `0x1812`,
handle `0x001b`) un report de **10 bytes**. Todos los valores son de **8 bits
(0..255)**.

| Byte | Campo            | Rango / significado                        |
| ---- | ---------------- | ------------------------------------------ |
| 0    | LX (stick izq X) | 0 = izquierda, 128 = centro, 255 = derecha |
| 1    | LY (stick izq Y) | 0 = arriba, 128 = centro, 255 = abajo      |
| 2    | RX (stick der X) | 0 = izquierda, 128 = centro, 255 = derecha |
| 3    | RY (stick der Y) | 0 = arriba, 128 = centro, 255 = abajo      |
| 4    | D-pad (hat)      | 0..7 direcciones, 255 = sin presionar      |
| 5    | Botones bajos    | máscara de bits (ver tabla)                |
| 6    | Botones altos    | máscara de bits (ver tabla)                |
| 7    | R2 analógico     | 0 = suelto, 255 = a fondo                  |
| 8    | L2 analógico     | 0 = suelto, 255 = a fondo                  |
| 9    | (reservado)      | —                                          |

### D-pad (byte 4)

| Valor | Dirección        |
| ----- | ---------------- |
| 0     | arriba           |
| 1     | arriba-derecha   |
| 2     | derecha          |
| 3     | abajo-derecha    |
| 4     | abajo            |
| 5     | abajo-izquierda  |
| 6     | izquierda        |
| 7     | arriba-izquierda |
| 255   | sin presionar    |

### Botones (bytes 5 y 6, leídos como `BTN = byte6<<8 | byte5`)

| Botón           | Máscara  |
| --------------- | -------- |
| A               | `0x0001` |
| B               | `0x0002` |
| X               | `0x0008` |
| Y               | `0x0010` |
| L1              | `0x0040` |
| R1              | `0x0080` |
| L2 (digital)    | `0x0100` |
| R2 (digital)    | `0x0200` |
| Select          | `0x0400` |
| Start           | `0x0800` |
| L3 (stick izq.) | `0x2000` |
| R3 (stick der.) | `0x4000` |

> Los gatillos L2/R2 aparecen dos veces: como **bit digital** (presionado sí/no)
> y como **eje analógico** (bytes 7 y 8). No es un error, es diseño del mando.

### Nota sobre la resolución

El mando entrega **8 bits (0..255)**. Otras librerías muestran los sticks como
`-512..512` y los gatillos como `0..512`, pero eso es un **reescalado**, no más
precisión real. Este proyecto muestra el valor crudo tal cual llega.

---

## Salida por consola

Se imprime **solo cuando el estado cambia** (para no saturar el USB del S3 y
perder datos). Ejemplo:

```
LX=128 LY=128 RX=128 RY=128 | L2=0 R2=0 | DPAD=- | Botones: -
LX=128 LY=32 RX=128 RY=128 | L2=0 R2=0 | DPAD=arriba | Botones: A X
LX=128 LY=128 RX=128 RY=128 | L2=255 R2=0 | DPAD=- | Botones: L2
```

- Ejes y gatillos: valor 0..255.
- D-pad: nombre de la dirección.
- Botones: nombres reales de los que están presionados (o `-` si ninguno).

---

## Estructura de archivos

```
EducaBot/
├── src/
│   └── main.cpp                  ← programa actual (joystick por nombres)
├── backups/                      ← versiones de prueba anteriores
│   ├── main_joystick_raw.cpp.bak     ← salida cruda (LX=.. BTN=0x..)
│   ├── main_joystick_raw_hex.cpp.bak ← volcado hex por notificación (era joystick_ident)
│   ├── main_ble_scan_v1.cpp.bak      ← primer escaneo BLE
│   ├── main_servo_scan_backup.cpp.bak← probador de servos (GPIO 14/15)
│   └── main_ws2812_backup.cpp.bak    ← probador de LEDs WS2812
├── platformio.ini
└── README.md
```

---

## Compilar y subir

```powershell
# Compilar
pio run -e esp32-s3-devkitc-1

# Subir (cerrá antes el Monitor Serie para liberar el puerto)
pio run -e esp32-s3-devkitc-1 --target upload

# Monitor serie
pio device monitor -e esp32-s3-devkitc-1
```

Velocidad del monitor: **115200 baudios**.

---

## Dependencias (`platformio.ini`)

- `h2zero/NimBLE-Arduino @ ^2.5.0` — host Bluetooth LE HID.
- `adafruit/Adafruit NeoPixel @ ^1.15.5` — (heredada del proyecto base).

---

## Tracción diferencial

El proyecto ahora usa la biblioteca interna `DifferentialDrive` para manejar el
puente H de dos motores.

- Se usa solo el stick derecho: `RY` = avance/reversa y `RX` = giro.
- La mezcla es tipo arcade, así que puede avanzar mientras gira.
- El motor derecho está invertido por software porque está montado espejado.
- `L2` atenúa linealmente la velocidad máxima: con `L2=0` llega al PWM máximo,
  y con `L2=255` queda limitado a `150` por defecto para maniobras finas.
- `L1` mantiene bloqueado el giro mientras esté presionado.
- `R1` mantiene bloqueado el avance y la reversa mientras esté presionado.
- `Y` conmuta un paro total enclavado: los motores quedan frenados hasta volver
  a presionarlo.
- Indicadores LED durante el control:
  - LED 0 rojo: bloqueo de giro.
  - LED físico 6 rojo: bloqueo de avance/reversa.
  - LED 1 y LED 4 ámbar intermitentes: paro total enclavado.

Constantes ajustables en [src/main.cpp](src/main.cpp):

- `TRACCION_CFG.pwmPrecisionMax`: límite de PWM con `L2` al máximo.
- `TRACCION_CFG.joystickDeadzone`: zona muerta del stick.
- `TRACCION_CFG.decayMode`: `Slow` usa `1/PWM` para mejor maniobra fina a baja
  velocidad y `Fast` usa `PWM/0` para una respuesta más directa.

En este puente H, el cambio de `decayMode` se implementa alternando la fase de
apagado entre coast y brake, que es la forma práctica de variar el decaimiento
de corriente desde firmware.
