// Creado por el Profesor Fernando Angel Liozzi - 2026

/// EducaBot v2 — control de joystick Bluetooth + servos + LED WS2812
/// -----------------------------------------------------------------
/// - JoystickBLE: escaneo, conexión y reportes HID del gamepad.
/// - GamepadData:  parseo del report (ejes, botones, D-pad).
/// - NeoPixelEffects: animaciones y señalización de los 6 LED WS2812.

#include <Arduino.h>
#include <ESP32Servo.h>

#include "DifferentialDrive.h"
#include "JoystickBLE.h"
#include "NeoPixelEffects.h"

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
Servo servo1;
Servo servo2;

constexpr DifferentialDrive::MotorPins MOTOR_IZQUIERDO(39, 40, 0, 1, false);
constexpr DifferentialDrive::MotorPins MOTOR_DERECHO(41, 42, 2, 3, true);

constexpr DifferentialDrive::Config
    TRACCION_CFG(10000, 8, 255, 150, 128, 16,
                 DifferentialDrive::DecayMode::Slow, false);

DifferentialDrive traccion(MOTOR_IZQUIERDO, MOTOR_DERECHO, TRACCION_CFG);

bool animConectadoDisparada = false;
bool paroTotalActivo = false;
bool yPresionadaAnterior = false;

// ---- Estado barrido de servos --------------------------------------------
int16_t servoAngulo = 0;
int8_t servoDireccion = 1;
unsigned long ultimoServoMs = 0;

// ===================================================================
void setup() {
  leds.begin(PIN_LEDS, CANT_LEDS);
  leds.setBrightness(BRILLO_LEDS);
  traccion.begin();

  servo1.setPeriodHertz(50);
  servo1.attach(PIN_SERVO1, 544, 2400);
  servo2.setPeriodHertz(50);
  servo2.attach(PIN_SERVO2, 544, 2400);

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

    leds.mostrarEstadoControl(bloqueoGiro, bloqueoAvance, paroTotalActivo);

    // ---- Barrido cíclico de servos (prueba) ----
    if (millis() - ultimoServoMs >= 15) {
      ultimoServoMs = millis();
      servoAngulo += servoDireccion;
      if (servoAngulo >= 180) {
        servoDireccion = -1;
        servoAngulo = 180;
      } else if (servoAngulo <= 0) {
        servoDireccion = 1;
        servoAngulo = 0;
      }
      servo1.write(servoAngulo);
      servo2.write(180 - servoAngulo); // espejado: opuesto al servo 1
    }

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