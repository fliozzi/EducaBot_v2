// Creado por el Profesor Fernando Angel Liozzi - 2026

#include "DifferentialDrive.h"

#include <Arduino.h>

DifferentialDrive::DifferentialDrive(const MotorPins &motorIzquierdo,
                                     const MotorPins &motorDerecho)
    : DifferentialDrive(motorIzquierdo, motorDerecho, Config{}) {}

DifferentialDrive::DifferentialDrive(const MotorPins &motorIzquierdo,
                                     const MotorPins &motorDerecho,
                                     const Config &config)
    : motorIzq(motorIzquierdo), motorDer(motorDerecho), cfg(config) {}

void DifferentialDrive::begin() {
  const MotorPins motores[] = {motorIzq, motorDer};
  for (const MotorPins &motor : motores) {
    pinMode(motor.in1, OUTPUT);
    pinMode(motor.in2, OUTPUT);

    ledcSetup(motor.channel1, cfg.pwmFrequency, cfg.pwmResolutionBits);
    ledcAttachPin(motor.in1, motor.channel1);
    ledcWrite(motor.channel1, 0);

    ledcSetup(motor.channel2, cfg.pwmFrequency, cfg.pwmResolutionBits);
    ledcAttachPin(motor.in2, motor.channel2);
    ledcWrite(motor.channel2, 0);
  }
}

void DifferentialDrive::stop() {
  if (cfg.brakeAtZero) {
    brake();
    return;
  }

  ledcWrite(motorIzq.channel1, 0);
  ledcWrite(motorIzq.channel2, 0);
  ledcWrite(motorDer.channel1, 0);
  ledcWrite(motorDer.channel2, 0);
}

void DifferentialDrive::brake() {
  ledcWrite(motorIzq.channel1, cfg.pwmMax);
  ledcWrite(motorIzq.channel2, cfg.pwmMax);
  ledcWrite(motorDer.channel1, cfg.pwmMax);
  ledcWrite(motorDer.channel2, cfg.pwmMax);
}

void DifferentialDrive::driveFromJoystick(uint8_t rx, uint8_t ry, uint8_t l2) {
  const int16_t avance = axisToSigned(ry, false);
  const int16_t giro = axisToSigned(rx, true);

  int32_t motorIzquierdo = static_cast<int32_t>(avance) + giro;
  int32_t motorDerecho = static_cast<int32_t>(avance) - giro;

  int32_t maxAbs = motorIzquierdo >= 0 ? motorIzquierdo : -motorIzquierdo;
  const int32_t absDerecho = motorDerecho >= 0 ? motorDerecho : -motorDerecho;
  if (absDerecho > maxAbs) {
    maxAbs = absDerecho;
  }
  if (maxAbs < 255) {
    maxAbs = 255;
  }

  motorIzquierdo = (motorIzquierdo * 255) / maxAbs;
  motorDerecho = (motorDerecho * 255) / maxAbs;

  const int16_t salidaIzq =
      applyL2Attenuation(static_cast<int16_t>(motorIzquierdo), l2);
  const int16_t salidaDer =
      applyL2Attenuation(static_cast<int16_t>(motorDerecho), l2);

  writeMotor(motorIzq, salidaIzq);
  writeMotor(motorDer, salidaDer);
}

int16_t DifferentialDrive::axisToSigned(uint8_t raw, bool invertAxis) const {
  int16_t delta = static_cast<int16_t>(raw) - cfg.joystickCenter;
  const int16_t absDelta = delta >= 0 ? delta : -delta;
  if (absDelta <= cfg.joystickDeadzone) {
    return 0;
  }

  if (delta > 0) {
    delta =
        map(delta, cfg.joystickDeadzone + 1, 255 - cfg.joystickCenter, 0, 255);
  } else {
    delta = -map(-delta, cfg.joystickDeadzone + 1, cfg.joystickCenter, 0, 255);
  }

  return invertAxis ? -delta : delta;
}

int16_t DifferentialDrive::applyL2Attenuation(int16_t valor, uint8_t l2) const {
  const int16_t salidaMaxima =
      cfg.pwmMax - ((static_cast<int16_t>(cfg.pwmMax) - cfg.pwmPrecisionMax) *
                    static_cast<int16_t>(l2)) /
                       255;
  return (valor * salidaMaxima) / cfg.pwmMax;
}

void DifferentialDrive::writeMotor(const MotorPins &motor, int16_t comando) {
  if (comando == 0) {
    if (cfg.brakeAtZero) {
      ledcWrite(motor.channel1, cfg.pwmMax);
      ledcWrite(motor.channel2, cfg.pwmMax);
    } else {
      ledcWrite(motor.channel1, 0);
      ledcWrite(motor.channel2, 0);
    }
    return;
  }

  const bool adelante = comando > 0;
  const int16_t absComando = comando >= 0 ? comando : -comando;
  const uint8_t pwm =
      static_cast<uint8_t>(constrain(absComando, 0, cfg.pwmMax));

  uint8_t activeChannel = motor.channel1;
  uint8_t inactiveChannel = motor.channel2;
  if ((!adelante && !motor.invertido) || (adelante && motor.invertido)) {
    activeChannel = motor.channel2;
    inactiveChannel = motor.channel1;
  }

  writeChannels(activeChannel, inactiveChannel, pwm);
}

void DifferentialDrive::writeChannels(uint8_t activeChannel,
                                      uint8_t inactiveChannel,
                                      uint8_t pwm) const {
  if (cfg.decayMode == DecayMode::Fast) {
    ledcWrite(activeChannel, pwm);
    ledcWrite(inactiveChannel, 0);
    return;
  }

  ledcWrite(activeChannel, cfg.pwmMax);
  ledcWrite(inactiveChannel, cfg.pwmMax - pwm);
}