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
constexpr uint8_t KNIGHT_RIDER_MS = 120;

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
bool knightRiderActivo = false;
bool l3Anterior = false;

// ---- Enclavamiento de servos ---------------------------------------------
bool sat1Enclavado = false;
bool sat2Enclavado = false;
uint8_t sat1AnguloEnclavado = 0;
uint8_t sat2AnguloEnclavado = 0;
bool xAnterior = false;
bool bAnterior = false;

// ===================================================================
void setup() {
  leds.begin(PIN_LEDS, CANT_LEDS);
  leds.setBrightness(BRILLO_LEDS);
  traccion.begin();

  servo1.begin(PIN_SERVO1);
  servo2.begin(PIN_SERVO2);
  calibracion.begin();

  joystick.iniciar();
  joystick.setVerbose(false); // descomentar para máxima eficiencia
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
    const bool l3Presionada = g.botonPresionado(GamepadData::BTN_L3);

    if (yPresionada && !yPresionadaAnterior) {
      paroTotalActivo = !paroTotalActivo;
    }
    yPresionadaAnterior = yPresionada;

    if (l3Presionada && !l3Anterior) {
      knightRiderActivo = !knightRiderActivo;
    }
    l3Anterior = l3Presionada;

    // ---- Calibración de servos ----
    calibracion.manejar(g, leds.getStrip(), servo1, servo2);

    if (calibracion.enCalibracion()) {
      return; // no mover motores ni LEDs normales durante calibración
    }

    // ---- Mapeo normal de servos (valores calibrados) ----
    // ---- Enclavamiento de servos ----
    bool xPresionado = g.botonPresionado(GamepadData::BTN_X);
    bool bPresionado = g.botonPresionado(GamepadData::BTN_B);

    if (xPresionado && !xAnterior) {
      sat1Enclavado = !sat1Enclavado;
      if (sat1Enclavado) {
        sat1AnguloEnclavado = servo1.getCurrentAngle();
      }
    }
    xAnterior = xPresionado;

    if (bPresionado && !bAnterior) {
      sat2Enclavado = !sat2Enclavado;
      if (sat2Enclavado) {
        sat2AnguloEnclavado = servo2.getCurrentAngle();
      }
    }
    bAnterior = bPresionado;

    // ---- Mapeo de servos (con enclavamiento) ----
    if (sat1Enclavado) {
      servo1.writeAngle(sat1AnguloEnclavado);
    } else {
      servo1.writeFromJoystickSmooth(g.ly, calibracion.getZero(0),
                                     calibracion.getSpan(0), g.r2, true);
    }
    if (sat2Enclavado) {
      servo2.writeAngle(sat2AnguloEnclavado);
    } else {
      servo2.writeFromJoystickSmooth(g.lx, calibracion.getZero(1),
                                     calibracion.getSpan(1), g.r2, false);
    }

    // ---- LEDs: prioridad indicadores > Knight Rider > apagado ----
    const bool hayIndicador = bloqueoGiro || bloqueoAvance || paroTotalActivo ||
                              sat1Enclavado || sat2Enclavado;

    if (hayIndicador) {
      leds.mostrarEstadoControl(bloqueoGiro, bloqueoAvance, paroTotalActivo);
      if (sat1Enclavado)
        leds.getStrip().setPixelColor(3, leds.getStrip().Color(255, 0, 255));
      if (sat2Enclavado)
        leds.getStrip().setPixelColor(2, leds.getStrip().Color(255, 0, 0));
      if (sat1Enclavado || sat2Enclavado)
        leds.getStrip().show();
    } else if (knightRiderActivo) {
      // ---- Knight Rider: barrido rojo direccional con estela ----
      static uint8_t krPos = 0;
      static int8_t krDir = 1;
      static unsigned long krLast = 0;

      if (millis() - krLast >= KNIGHT_RIDER_MS) {
        krLast = millis();
        Adafruit_NeoPixel &t = leds.getStrip();
        t.setBrightness(BRILLO_LEDS);
        t.clear();

        for (uint8_t i = 0; i < CANT_LEDS; ++i) {
          int detras = ((int)krPos - (int)i) * krDir;
          if (detras < 0)
            continue;
          uint8_t b = 0;
          if (detras == 0)
            b = 255;
          else if (detras == 1)
            b = 100;
          else if (detras == 2)
            b = 40;
          else if (detras == 3)
            b = 12;
          else
            continue;
          t.setPixelColor(i, t.Color(b, 0, 0));

          if ((krDir > 0 && krPos >= CANT_LEDS - 2) ||
              (krDir < 0 && krPos <= 1)) {
            int8_t ad = (int8_t)krPos + krDir;
            if (ad >= 0 && ad < CANT_LEDS)
              t.setPixelColor(ad, t.Color(60, 0, 0));
          }
        }
        t.show();

        if (krDir > 0) {
          if (krPos >= CANT_LEDS - 1) {
            krDir = -1;
            --krPos;
          } else
            ++krPos;
        } else {
          if (krPos == 0) {
            krDir = 1;
            ++krPos;
          } else
            --krPos;
        }
      }
    } else {
      leds.getStrip().clear();
      leds.getStrip().show();
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
    l3Anterior = false;
    paroTotalActivo = false;
    knightRiderActivo = false;
    traccion.stop();
    leds.animarEscaneo();
  }
}