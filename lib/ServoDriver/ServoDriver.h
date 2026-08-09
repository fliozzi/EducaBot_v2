// Creado por el Profesor Fernando Angel Liozzi - 2026

/// ServoDriver — capa de hardware para servo con ESP32Servo (MCPWM).
///
/// Encapsula la inicialización y escritura de un servo usando la biblioteca
/// ESP32Servo en modo MCPWM (sin conflicto con los LEDC de los motores).
///
/// Uso típico:
///   ServoDriver servo1;
///   servo1.begin(14);
///   servo1.writeFromJoystick(g.ly, zero, span);

#pragma once

#include <ESP32Servo.h>
#include <stdint.h>

class ServoDriver {
public:
  /// Inicializa el servo en el pin indicado (50 Hz, 544–2400 µs).
  void begin(uint8_t pin);

  /// Escribe un ángulo absoluto (0–180°). Sin suavizado.
  void writeAngle(uint8_t angulo);

  /// Mapea un eje de joystick al rango calibrado [zero, span].
  /// Sin suavizado — usar durante calibración.
  /// @param ejeLY  true=LY (centro→arriba), false=LX (centro→derecha)
  void writeFromJoystick(uint8_t eje, uint16_t zero, uint16_t span, bool ejeLY);

  /// Mapeo con suavizado continuo. R2 controla la velocidad.
  /// @param enCalibracion  true → 1/10 de velocidad (precisión extra)
  void writeFromJoystickSmooth(uint8_t eje, uint16_t zero, uint16_t span,
                               uint8_t r2, bool ejeLY,
                               bool enCalibracion = false);

  /// Ángulo actual (0–180°) según el último suavizado.
  uint8_t getCurrentAngle() const { return currentAngle_x16 / 16; }

  /// Libera el servo (desconecta la señal PWM).
  void detach();

private:
  Servo servo;
  uint8_t pin = 255;

  // Estado para suavizado (ángulo × 16, resolución sub-grado)
  uint16_t currentAngle_x16 = 0;
  unsigned long lastSmoothMs = 0;
  bool smoothInit = false;
};
