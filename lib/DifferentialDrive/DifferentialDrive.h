// Creado por el Profesor Fernando Angel Liozzi - 2026

/// DifferentialDrive — tracción diferencial de dos motores DC con mezcla tipo
/// arcade usando un solo joystick analógico.
///
/// Uso típico:
///   DifferentialDrive drive(izq, der, cfg);
///   drive.begin();
///   drive.driveFromJoystick(g.rx, g.ry, g.l2);

#pragma once

#include <stdint.h>

class DifferentialDrive {
public:
  enum class DecayMode : uint8_t {
    Slow,
    Fast,
  };

  struct MotorPins {
    uint8_t in1;
    uint8_t in2;
    uint8_t channel1;
    uint8_t channel2;
    bool invertido = false;

    constexpr MotorPins(uint8_t pin1, uint8_t pin2, uint8_t ch1, uint8_t ch2,
                        bool invert = false)
        : in1(pin1), in2(pin2), channel1(ch1), channel2(ch2),
          invertido(invert) {}
  };

  struct Config {
    uint32_t pwmFrequency = 10000;
    uint8_t pwmResolutionBits = 8;
    uint8_t pwmMax = 255;
    uint8_t pwmPrecisionMax = 150;
    uint8_t joystickCenter = 128;
    uint8_t joystickDeadzone = 16;
    DecayMode decayMode = DecayMode::Slow;
    bool brakeAtZero = false;

    constexpr Config(uint32_t frequency = 10000, uint8_t resolutionBits = 8,
                     uint8_t maxPwm = 255, uint8_t precisionMaxPwm = 150,
                     uint8_t center = 128, uint8_t deadzone = 16,
                     DecayMode mode = DecayMode::Slow, bool brakeOnZero = false)
        : pwmFrequency(frequency), pwmResolutionBits(resolutionBits),
          pwmMax(maxPwm), pwmPrecisionMax(precisionMaxPwm),
          joystickCenter(center), joystickDeadzone(deadzone), decayMode(mode),
          brakeAtZero(brakeOnZero) {}
  };

  DifferentialDrive(const MotorPins &motorIzquierdo,
                    const MotorPins &motorDerecho);
  DifferentialDrive(const MotorPins &motorIzquierdo,
                    const MotorPins &motorDerecho, const Config &config);

  void begin();
  void stop();
  void brake();

  /// Mezcla arcade: RY = avance/reversa, RX = giro.
  /// L2 atenúa linealmente la salida: 0 -> pwmMax, 255 -> pwmPrecisionMax.
  void driveFromJoystick(uint8_t rx, uint8_t ry, uint8_t l2);

  void setDecayMode(DecayMode mode) { cfg.decayMode = mode; }
  void setPrecisionPwmMax(uint8_t pwm) { cfg.pwmPrecisionMax = pwm; }

private:
  MotorPins motorIzq;
  MotorPins motorDer;
  Config cfg;

  int16_t axisToSigned(uint8_t raw, bool invertAxis) const;
  int16_t applyL2Attenuation(int16_t valor, uint8_t l2) const;
  void writeMotor(const MotorPins &motor, int16_t comando);
  void writeChannels(uint8_t activeChannel, uint8_t inactiveChannel,
                     uint8_t pwm) const;
};