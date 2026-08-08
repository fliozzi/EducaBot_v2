// Creado por el Profesor Fernando Angel Liozzi - 2026

#pragma once

#include <stddef.h>
#include <stdint.h>

struct GamepadData {
  static constexpr uint16_t BTN_Y = 0x0010;
  static constexpr uint16_t BTN_L1 = 0x0040;
  static constexpr uint16_t BTN_R1 = 0x0080;
  static constexpr uint16_t BTN_L2 = 0x0100;

  uint8_t lx = 128, ly = 128, rx = 128, ry = 128;
  uint8_t dpad = 255;
  uint8_t l2 = 0, r2 = 0;
  uint16_t botones = 0;

  /// Devuelve false si el report no es de 10 bytes.
  bool parseFromReport(const uint8_t *datos, size_t largo);

  /// Vuelca al Serial una línea con ejes, gatillos, D-pad y botones.
  void printToSerial() const;

  /// Una sola línea de ancho fijo con \r (sobrescribe en lugar).
  void printCompact() const;

  static const char *nombreDpad(uint8_t v);

  /// Llena buf con los nombres de los botones activos (ej: "A X L1").
  static void nombresBotones(uint16_t mascara, char *buf, size_t bufSize);

  bool botonPresionado(uint16_t mascara) const {
    return (botones & mascara) != 0;
  }
};
