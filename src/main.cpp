// Creado por el Profesor Fernando Angel Liozzi - 2026

/// EducaBot v2 — control de joystick Bluetooth + servos + LED WS2812
/// -----------------------------------------------------------------
/// - JoystickBLE: escaneo, conexión y reportes HID del gamepad.
/// - GamepadData:  parseo del report (ejes, botones, D-pad).
/// - NeoPixelEffects: animaciones y señalización de los 6 LED WS2812.
/// - ServoDriver: capa de hardware para servos (MCPWM).
/// - ServoCalibration: máquina de estados de calibración con NVS.

#include <Arduino.h>

#include "DifferentialDrive.h"
#include "JoystickBLE.h"
#include "NeoPixelEffects.h"
#include "ServoCalibration.h"
#include "ServoDriver.h"

// ---- Configuración de hardware -------------------------------------------
constexpr uint8_t PIN_LEDS = 2; // GPIO del bus WS2812 (ajustar según placa)
constexpr uint8_t CANT_LEDS = 6;
constexpr uint8_t BRILLO_LEDS = 48;

// Servos (SAT1 y SAT2 en conectores RJ11 de la EduPlugPower)
constexpr uint8_t PIN_SERVO1 = 14;
constexpr uint8_t PIN_SERVO2 = 15;

// ---- Objetos globales ----------------------------------------------------
JoystickBLE joystick;
NeoPixelEffects leds;
ServoDriver servo1;
ServoDriver servo2;
ServoCalibration calibracion;

constexpr DifferentialDrive::MotorPins MOTOR_IZQUIERDO(39, 40, 0, 1, false);
constexpr DifferentialDrive::MotorPins MOTOR_DERECHO(41, 42, 2, 3, true);

constexpr DifferentialDrive::Config
    TRACCION_CFG(10000, 8, 255, 150, 128, 16,
                 DifferentialDrive::DecayMode::Slow, false);

DifferentialDrive traccion(MOTOR_IZQUIERDO, MOTOR_DERECHO, TRACCION_CFG);

bool animConectadoDisparada = false;
bool paroTotalActivo = false;
bool yPresionadaAnterior = false;

// ===================================================================
void setup() {
  leds.begin(PIN_LEDS, CANT_LEDS);
  leds.setBrightness(BRILLO_LEDS);
  traccion.begin();

  servo1.begin(PIN_SERVO1);
  servo2.begin(PIN_SERVO2);
  calibracion.begin();

  joystick.iniciar();
  // joystick.setVerbose(false); // descomentar para máxima eficiencia
}

// ===================================================================
void loop() {
  joystick.manejar();

  if (joystick.conectado()) {
    // ----- MODO CONECTADO -----
    if (!animConectadoDisparada) {
      leds.iniciarAnimConectado();
      animConectadoDisparada = true;
    }

    if (!leds.animConectadoTerminada()) {
      leds.actualizarAnimConectado();
      return; // esperar a que termine la animación de conexión
    }

    const GamepadData &g = joystick.estado();

    const bool bloqueoGiro = g.botonPresionado(GamepadData::BTN_L1);
    const bool bloqueoAvance = g.botonPresionado(GamepadData::BTN_R1);
    const bool yPresionada = g.botonPresionado(GamepadData::BTN_Y);

    if (yPresionada && !yPresionadaAnterior) {
      paroTotalActivo = !paroTotalActivo;
    }
    yPresionadaAnterior = yPresionada;

    // ---- Calibración de servos ----
    calibracion.manejar(g, leds.getStrip(), servo1, servo2);

    if (calibracion.enCalibracion()) {
      return; // no mover motores ni LEDs normales durante calibración
    }

    // ---- Mapeo normal de servos (valores calibrados) ----
    servo1.writeFromJoystick(g.ly, calibracion.getZero(0),
                             calibracion.getSpan(0));
    servo2.writeFromJoystick(g.lx, calibracion.getZero(1),
                             calibracion.getSpan(1));

    leds.mostrarEstadoControl(bloqueoGiro, bloqueoAvance, paroTotalActivo);

    if (paroTotalActivo) {
      traccion.brake();
      return;
    }

    const uint8_t rxControl = bloqueoGiro ? 128 : g.rx;
    const uint8_t ryControl = bloqueoAvance ? 128 : g.ry;
    traccion.driveFromJoystick(rxControl, ryControl, g.l2);

  } else {
    // ----- MODO ESCANEO (sin joystick) -----
    animConectadoDisparada = false;
    yPresionadaAnterior = false;
    paroTotalActivo = false;
    traccion.stop();
    leds.animarEscaneo();
  }
}