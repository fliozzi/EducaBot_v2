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

  /// Escribe un ángulo absoluto (0–180°).
  void writeAngle(uint8_t angulo);

  /// Mapea un eje de joystick (0–255) al rango calibrado [zero, span].
  void writeFromJoystick(uint8_t eje, uint16_t zero, uint16_t span);

  /// Libera el servo (desconecta la señal PWM).
  void detach();

private:
  Servo servo;
  uint8_t pin = 255;
};
