// Creado por el Profesor Fernando Angel Liozzi - 2026

/// ServoCalibration — máquina de estados de calibración de servos con
/// persistencia en NVS (Preferences).
///
/// Flujo:
///   NORMAL → (SELECT 5s) → CAL_ENTER → (3s) → CAL_ZERO → (A) → CAL_SPAN
///   → (A) → NORMAL
///
/// Durante CAL_ENTER, START alterna entre SAT1 y SAT2.
///
/// Uso típico:
///   ServoCalibration calib;
///   calib.begin();
///   calib.manejar(gamepad, tiraLed, servo1, servo2);
///   if (!calib.enCalibracion()) {
///     servo1.writeFromJoystick(g.ly, calib.getZero(0), calib.getSpan(0));
///   }

#pragma once

#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <stdint.h>

#include "GamepadData.h"

class ServoDriver;

class ServoCalibration {
public:
  enum class Estado {
    NORMAL,
    CAL_CLEAR, // A 5 s → flash rosa 3 s → borrar NVS → NORMAL
    CAL_WAIT,  // SELECT presionado, esperando 5 s (operación normal)
    CAL_ENTER, // 6 LEDs flash blanco, 3 s, START alterna servo
    CAL_ZERO,  // calibrando zero del servo activo
    CAL_SPAN   // calibrando span del servo activo
  };

  /// Carga zero/span desde NVS (default 0–180 si no hay datos).
  void begin();

  /// Tick de la máquina de estados. Llamar en loop().
  /// @param gamepad  estado actual del gamepad
  /// @param tira     tira NeoPixel para LEDs de calibración
  /// @param servo1   servo SAT1 (GPIO14)
  /// @param servo2   servo SAT2 (GPIO15)
  void manejar(const GamepadData &gamepad, Adafruit_NeoPixel &tira,
               ServoDriver &servo1, ServoDriver &servo2);

  /// true si la calibración está activa (bloquear tracción).
  bool enCalibracion() const {
    return estado == Estado::CAL_CLEAR || estado == Estado::CAL_ENTER ||
           estado == Estado::CAL_ZERO || estado == Estado::CAL_SPAN;
  }

  uint16_t getZero(uint8_t idx) const { return zero[idx]; }
  uint16_t getSpan(uint8_t idx) const { return span[idx]; }

  /// Índice del servo que se está calibrando (0=SAT1, 1=SAT2).
  uint8_t servoActivo() const { return servoIdx; }

  // Máscaras de botones usadas por la calibración
  static constexpr uint16_t BTN_SELECT = 0x0400;
  static constexpr uint16_t BTN_START = 0x0800;
  static constexpr uint16_t BTN_A = 0x0001;

private:
  void guardar();
  void cargar();
  void limpiarTira(Adafruit_NeoPixel &tira);

  Estado estado = Estado::NORMAL;
  uint8_t servoIdx = 0; // 0=SAT1, 1=SAT2

  uint16_t zero[2] = {0, 0};
  uint16_t span[2] = {180, 180};

  // Temporización
  unsigned long tiempoEstado = 0;
  unsigned long ultimoFlash = 0;
  unsigned long startPresionadoMs = 0;
  unsigned long aPresionadoMs = 0;
  bool flashState = false;

  // Detección de flancos
  bool selectAnterior = false;
  bool startAnterior = false;
  bool aAnterior = false;
};
