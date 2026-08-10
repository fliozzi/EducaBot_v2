# UML del Proyecto EducaBot v2

> **Autor:** Prof. Fernando Angel Liozzi — 2026
>
> Robot educativo de traccion diferencial controlado por joystick Bluetooth BLE.
> ESP32-S3 + Arduino + PlatformIO. 6 bibliotecas propias + 3 externas.

---

## 1. Diagrama de Arquitectura de Componentes

```mermaid
graph TB
    subgraph "Control Externo"
        GP["GamePadPlus V3<br/>Joystick Bluetooth BLE"]
    end

    subgraph "Comunicacion"
        BLE["JoystickBLE<br/>NimBLE Client<br/>Escaneo - Conexion - Notificaciones"]
    end

    subgraph "Nucleo - main.cpp / loop"
        MAIN["Orquestador Principal<br/>setup / loop"]
    end

    subgraph "Parseo"
        GD["GamepadData<br/>Parseo de reportes HID 10 bytes<br/>Ejes - Botones - D-Pad - Gatillos"]
    end

    subgraph "Servos"
        SD["ServoDriver<br/>MCPWM - 50 Hz<br/>Suavizado x16 submultiplo"]
        SC["ServoCalibration<br/>Maquina de estados<br/>NVS Preferences"]
    end

    subgraph "Iluminacion"
        NPE["NeoPixelEffects<br/>WS2812 - 6 LEDs<br/>Escaneo - Conexion - Estado - Knight Rider"]
    end

    subgraph "Traccion"
        DD["DifferentialDrive<br/>Mezcla arcade<br/>LEDC PWM - Decay Mode"]
    end

    subgraph "Hardware"
        HR["HR8833<br/>Puente H doble"]
        M1["Motor DC Izquierdo"]
        M2["Motor DC Derecho"]
        SV1["Servo MG996R<br/>SAT1 - GPIO14"]
        SV2["Servo MG996R<br/>SAT2 - GPIO15"]
        WS["WS2812 x 6<br/>GPIO2"]
    end

    subgraph "Bibliotecas Externas"
        NIMBLE["NimBLE-Arduino v2.5"]
        NEOP["Adafruit NeoPixel v1.15"]
        ESP32S["ESP32Servo v3.2"]
    end

    GP -->|"BLE HID"| BLE
    BLE -->|"reportes"| GD
    BLE -->|"estado"| MAIN
    GD -->|"GamepadData &"| MAIN
    MAIN -->|"driveFromJoystick"| DD
    MAIN -->|"animar / mostrar"| NPE
    MAIN -->|"writeFromJoystickSmooth"| SD
    MAIN -->|"manejar calibracion"| SC
    SC -->|"writeFromJoystickSmooth"| SD
    SC -->|"NVS"| NVS[("Preferences<br/>NVS Flash")]
    DD -->|"PWM LEDC"| HR
    HR --> M1
    HR --> M2
    SD -->|"MCPWM"| SV1
    SD -->|"MCPWM"| SV2
    NPE -->|"GPIO2"| WS

    BLE -.->|"usa"| NIMBLE
    NPE -.->|"usa"| NEOP
    SD -.->|"usa"| ESP32S

    style MAIN fill:#f9f,stroke:#333,stroke-width:3px
    style BLE fill:#bbf,stroke:#333,stroke-width:2px
    style GD fill:#bbf,stroke:#333,stroke-width:2px
    style DD fill:#bfb,stroke:#333,stroke-width:2px
    style NPE fill:#fbf,stroke:#333,stroke-width:2px
    style SD fill:#ffb,stroke:#333,stroke-width:2px
    style SC fill:#fdb,stroke:#333,stroke-width:2px
```

---

## 2. Diagrama de Clases

```mermaid
classDiagram
    direction TB

    class GamepadData {
        <<struct>>
        +uint8_t lx, ly, rx, ry
        +uint8_t dpad
        +uint8_t l2, r2
        +uint16_t botones
        +bool parseFromReport(data, len)
        +void printToSerial()
        +void printCompact()
        +bool botonPresionado(mask)
        +static nombreDpad(v) string
        +static nombresBotones(mask, buf, size)
    }

    class JoystickBLE {
        -GamepadData gamepad
        -uint8_t ultimoReport[20]
        -NimBLEAdvertisedDevice* objetivo
        -NimBLEClient* cliente
        -bool encontrado
        -bool pedirReescaneo
        -bool verbose
        +void iniciar()
        +void manejar()
        +bool conectado()
        +GamepadData& estado()
        +void setVerbose(bool)
        +void onNotificacion(car, data, len, isNotif)
        +void onDisconnected(reason)
        +void onScanResultado(disp)
        -void iniciarEscaneo()
        -void conectarYExplorar()
    }

    class NeoPixelEffects {
        -Adafruit_NeoPixel tira
        -uint8_t numLeds
        -uint8_t brilloGlobal
        -uint8_t brilloEscaneo
        -uint8_t tonoBase
        -FaseConectado faseConectado
        +void begin(pin, cantidad)
        +void setBrightness(brillo)
        +void animarEscaneo()
        +void iniciarAnimConectado()
        +bool actualizarAnimConectado()
        +bool animConectadoTerminada()
        +void mostrarEstadoControl(bloqueoGiro, bloqueoAvance, paroTotal)
        +void apagar()
        +Adafruit_NeoPixel& getStrip()
    }

    class DifferentialDrive {
        -MotorPins motorIzq
        -MotorPins motorDer
        -Config cfg
        +DifferentialDrive(izq, der)
        +DifferentialDrive(izq, der, config)
        +void begin()
        +void stop()
        +void brake()
        +void driveFromJoystick(rx, ry, l2)
        +void setDecayMode(mode)
        +void setPrecisionPwmMax(pwm)
        -int16_t axisToSigned(raw, invert)
        -int16_t applyL2Attenuation(valor, l2)
        -void writeMotor(motor, comando)
        -void writeChannels(activeCh, inactiveCh, pwm)
    }

    class DifferentialDrive.MotorPins {
        <<struct>>
        +uint8_t in1, in2
        +uint8_t channel1, channel2
        +bool invertido
    }

    class DifferentialDrive.Config {
        <<struct>>
        +uint32_t pwmFrequency = 10000
        +uint8_t pwmResolutionBits = 8
        +uint8_t pwmMax = 255
        +uint8_t pwmPrecisionMax = 150
        +uint8_t joystickCenter = 128
        +uint8_t joystickDeadzone = 16
        +DecayMode decayMode = Slow
        +bool brakeAtZero = false
    }

    class DifferentialDrive.DecayMode {
        <<enumeration>>
        Slow
        Fast
    }

    class ServoDriver {
        -Servo servo
        -uint8_t pin
        -uint16_t currentAngle_x16
        -unsigned long lastSmoothMs
        -bool smoothInit
        +void begin(pin)
        +void writeAngle(angulo)
        +void writeFromJoystick(eje, zero, span, ejeLY)
        +void writeFromJoystickSmooth(eje, zero, span, r2, ejeLY, enCalibracion)
        +uint8_t getCurrentAngle()
        +void detach()
    }

    class ServoCalibration {
        -Estado estado = NORMAL
        -uint8_t servoIdx
        -uint16_t zero[2], span[2]
        -unsigned long tiempoEstado, ultimoFlash, startPresionadoMs, aPresionadoMs
        -bool flashState, selectAnterior, startAnterior, aAnterior
        +void begin()
        +void manejar(gamepad, tira, servo1, servo2)
        +bool enCalibracion()
        +uint16_t getZero(idx)
        +uint16_t getSpan(idx)
        +uint8_t servoActivo()
        -void guardar()
        -void cargar()
        -void limpiarTira(tira)
    }

    class ServoCalibration.Estado {
        <<enumeration>>
        NORMAL
        CAL_CLEAR
        CAL_WAIT
        CAL_ENTER
        CAL_ZERO
        CAL_SPAN
    }

    class Adafruit_NeoPixel {
        <<externo>>
        begin()
        setPixelColor()
        setBrightness()
        getBrightness()
        clear()
        show()
        Color()
    }

    class Servo {
        <<externo - ESP32Servo>>
        attach()
        write()
        detach()
        setPeriodHertz()
    }

    class Preferences {
        <<externo - NVS>>
        begin()
        getUShort()
        putUShort()
        end()
    }

    %% Relaciones
    JoystickBLE *-- GamepadData : "contiene 1"
    JoystickBLE --> GamepadData : "usa"
    ServoCalibration --> GamepadData : "lee botones"
    ServoCalibration --> ServoDriver : "controla 2"
    ServoCalibration --> Adafruit_NeoPixel : "senaliza"
    ServoCalibration --> Preferences : "persiste"
    NeoPixelEffects --> Adafruit_NeoPixel : "contiene 1"
    ServoDriver --> Servo : "contiene 1"
    DifferentialDrive *-- DifferentialDrive.MotorPins : "2 motores"
    DifferentialDrive *-- DifferentialDrive.Config : "configuracion"

    %% main.cpp instancia
    note for JoystickBLE "Global: joystick"
    note for NeoPixelEffects "Global: leds"
    note for ServoDriver "Global: servo1, servo2"
    note for ServoCalibration "Global: calibracion"
    note for DifferentialDrive "Global: traccion"
```

---

## 3. Máquina de Estados — Calibración de Servos

```mermaid
stateDiagram-v2
    direction TB

    [*] --> NORMAL

    NORMAL --> CAL_WAIT : SELECT presionado
    NORMAL --> CAL_CLEAR : A sostenido 5 s

    CAL_WAIT --> NORMAL : SELECT soltado<br/>antes de 5 s
    CAL_WAIT --> CAL_ENTER : SELECT 5 s

    CAL_CLEAR --> NORMAL : 3 s (borra NVS)<br/>Flash rosa 6 Hz

    CAL_ENTER --> NORMAL : START (SAT2 activo)<br/>o START 2 s
    CAL_ENTER --> CAL_ZERO : 3 s<br/>Flash blanco 6 Hz<br/>START alterna SAT1 / SAT2

    CAL_ZERO --> NORMAL : START (SAT2 activo)<br/>o START 2 s
    CAL_ZERO --> CAL_SPAN : A confirma zero<br/>LED0 verde

    CAL_SPAN --> NORMAL : START (SAT2 activo)<br/>START 2 s<br/>o A confirma span
    CAL_SPAN --> CAL_ZERO : START (SAT1 activo)<br/>cambia a preparar<br/>zero del otro servo

    note right of CAL_ENTER
        LED5 amarillo = SAT1
        LED5 azul = SAT2
    end note

    note right of CAL_ZERO
        LED0 rojo = esperando zero
        LED5 indica servo activo
        Joystick mueve el servo
    end note

    note right of CAL_SPAN
        LED1 verde = confirmacion span
        LED5 indica servo activo
        Joystick mueve el servo
    end note

    note left of NORMAL
        A 5 s -> borra calibracion
        Vuelve a defaults (0-180 grados)
    end note
```

---

## 4. Flujo Principal del Programa

```mermaid
flowchart TD
    A(["Inicio - setup"]) --> B["leds.begin / traccion.begin<br/>servo1/2.begin / calibracion.begin<br/>joystick.iniciar"]

    B --> C{"joystick.conectado?"}

    C -->|"No"| D["leds.animarEscaneo<br/>arcoiris giratorio<br/>25 fps"]
    D --> C

    C -->|"Si"| E["Animacion de conexion<br/>pulso verde + fade out"]
    E --> F{"Animacion termino?"}
    F -->|"No"| E
    F -->|"Si"| G["Leer GamepadData"]

    G --> H{"SELECT 5 s?"}
    H -->|"Si"| I["ServoCalibration<br/>maquina de estados"]
    I --> C

    H -->|"No"| J{"Enclavamientos?<br/>X -> SAT1 / B -> SAT2"}
    J --> K["Mapear servos<br/>con suavizado"]

    K --> L{"Indicadores activos?<br/>L1-R1-Y-enclavamientos"}
    L -->|"Si"| M["mostrarEstadoControl<br/>LEDs de bloqueo"]

    L -->|"No"| N{"Knight Rider<br/>activo?<br/>toggle L3"}
    N -->|"Si"| O["Efecto Knight Rider<br/>barrido rojo + estela"]
    N -->|"No"| P["LEDs apagados<br/>ahorro bateria"]

    M --> Q{"Paro total?<br/>toggle Y"}
    Q -->|"Si"| R["traccion.brake<br/>freno activo"]

    Q -->|"No"| S["traccion.driveFromJoystick<br/>RX-RY-L2<br/>mezcla arcade<br/>bloqueos L1/R1"]

    R --> C
    S --> C
    O --> C
    P --> C
    I --> C
```

---

## 5. Diagrama de Secuencia — Conexión BLE

```mermaid
sequenceDiagram
    actor U as Usuario
    participant M as main.cpp
    participant JB as JoystickBLE
    participant N as NimBLE Stack
    participant GP as GamePadPlus V3
    participant GD as GamepadData
    participant NPE as NeoPixelEffects

    U->>GP: Modo pairing<br/>(Home + X)

    M->>JB: iniciar()
    JB->>N: NimBLEDevice::init()
    JB->>N: deleteAllBonds()
    JB->>N: startScan(0, false)

    loop Escaneo pasivo
        N-->>JB: onScanResultado(disp)
        JB->>JB: esHID o GamePad?
        JB->>N: stop()
    end

    M->>JB: manejar()
    JB->>N: connect(objetivo)
    N->>GP: Conexion BLE
    GP-->>N: Paired
    JB->>N: secureConnection()
    JB->>N: getServices(true)
    JB->>N: subscribe(notify)

    M->>NPE: iniciarAnimConectado()
    NPE->>NPE: Pulso verde centro -> afuera
    NPE->>NPE: Fade out gradual

    loop Cada report HID (~100 Hz)
        GP-->>N: Notify (10 bytes)
        N-->>JB: onNotificacion(data, len)
        JB->>GD: parseFromReport(data, 10)
        GD-->>JB: lx, ly, rx, ry, dpad, botones, r2, l2
    end

    GP-->>N: Disconnect
    N-->>JB: onDisconnected(reason)
    JB->>JB: pedirReescaneo = true
    JB->>N: deleteAllBonds()
    JB->>N: startScan()
```

---

## 6. Dependencias y Estructura de Archivos

```mermaid
graph LR
    subgraph "EducaBot_v2/"
        direction TB
        PF["platformio.ini"]
        RM["README.md"]
        SRC["src/main.cpp"]
        subgraph "lib/"
            DD_lib["DifferentialDrive/"]
            GD_lib["GamepadData/"]
            JB_lib["JoystickBLE/"]
            NPE_lib["NeoPixelEffects/"]
            SC_lib["ServoCalibration/"]
            SD_lib["ServoDriver/"]
        end
        DOCS["docs/"]
    end

    subgraph "Dependencias Externas"
        NIMBLE["h2zero/NimBLE-Arduino ^2.5.0"]
        NEOPXL["adafruit/Adafruit NeoPixel ^1.15.5"]
        ESP32SERVO["madhephaestus/ESP32Servo ^3.2.1"]
    end

    SRC --> DD_lib
    SRC --> GD_lib
    SRC --> JB_lib
    SRC --> NPE_lib
    SRC --> SC_lib
    SRC --> SD_lib

    JB_lib --> NIMBLE
    JB_lib --> GD_lib
    NPE_lib --> NEOPXL
    SD_lib --> ESP32SERVO
    SC_lib --> SD_lib
    SC_lib --> GD_lib

    PF --> NIMBLE
    PF --> NEOPXL
    PF --> ESP32SERVO
```

---

## 7. Resumen de Relaciones entre Clases

| Clase                 | Depende de                                                       | Rol                                                               |
| --------------------- | ---------------------------------------------------------------- | ----------------------------------------------------------------- |
| **JoystickBLE**       | `GamepadData`, NimBLE                                            | Cliente BLE: escanea, conecta, recibe notificaciones HID          |
| **GamepadData**       | — (struct puro)                                                  | Parseo de reportes HID de 10 bytes -> ejes, botones, D-pad        |
| **DifferentialDrive** | LEDC (ESP32)                                                     | Mezcla arcade + PWM para 2 motores DC vía HR8833                  |
| **NeoPixelEffects**   | `Adafruit_NeoPixel`                                              | Animaciones LED: escaneo arcoíris, conexión, estado, Knight Rider |
| **ServoDriver**       | `ESP32Servo` (MCPWM)                                             | Capa de hardware: attach, write, suavizado continuo               |
| **ServoCalibration**  | `ServoDriver`, `GamepadData`, `Adafruit_NeoPixel`, `Preferences` | Máquina de estados de calibración con persistencia NVS            |
| **main.cpp**          | Todos los anteriores                                             | Orquestador: setup, loop, lógica Knight Rider, enclavamientos     |
