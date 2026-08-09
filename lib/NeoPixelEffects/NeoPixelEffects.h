// Creado por el Profesor Fernando Angel Liozzi - 2026

/// NeoPixelEffects — efectos de iluminación para tira WS2812 de EducaBot.
///
/// Uso:
///   NeoPixelEffects leds;
///   leds.begin(PIN, 6);
///   leds.animarEscaneo();   // arcoíris giratorio mientras busca joystick
///   leds.iniciarAnimConectado();  // pulso verde centro->afuera + apagado

#pragma once

#include <Adafruit_NeoPixel.h>
#include <stdint.h>

class NeoPixelEffects {
public:
  void begin(uint8_t pin, uint8_t cantidad);

  /// Brillo global 0..255 aplicado a todos los efectos.
  void setBrightness(uint8_t brillo);

  /// Efecto de escaneo: arcoíris cíclico con movimiento circular.
  /// Llamar en cada loop mientras no hay joystick conectado.
  void animarEscaneo();

  /// Dispara la animación de conexión (verde, centro fuerte, pulso hacia
  /// afuera, luego apagado). Llamar UNA vez al detectar la conexión.
  void iniciarAnimConectado();

  /// Actualiza la animación de conexión. Devuelve true cuando ya terminó
  /// (luces apagadas para ahorro de batería).
  bool actualizarAnimConectado();

  /// Muestra indicadores de estado durante el control manual.
  /// LED 0 rojo: bloqueo de giro.
  /// LED físico 6 rojo: bloqueo de avance/reversa.
  /// LEDs 1 y 4 ámbar intermitentes: paro total enclavado.
  void mostrarEstadoControl(bool bloqueoGiro, bool bloqueoAvance,
                            bool paroTotal);

  /// Apaga todos los LED inmediatamente.
  void apagar();

  /// Devuelve true si la animación de conexión ya terminó.
  bool animConectadoTerminada() const;

  /// Acceso directo a la tira NeoPixel (para calibración de servos).
  Adafruit_NeoPixel &getStrip() { return tira; }

private:
  Adafruit_NeoPixel tira{6, 4, NEO_GRB + NEO_KHZ800};
  uint8_t numLeds = 6;
  uint8_t brilloGlobal = 64;
  uint8_t brilloEscaneo =
      10; // escaneo suave, independiente del brillo de conexión

  // -- escaneo --
  uint8_t tonoBase = 0;
  unsigned long ultimoFrameEscaneoMs = 0;

  // -- animación conectado --
  enum FaseConectado { FC_NINGUNA, FC_PULSO_VERDE, FC_APAGANDO, FC_TERMINADO };
  FaseConectado faseConectado = FC_NINGUNA;
  unsigned long inicioFaseMs = 0;
  uint8_t pasoConectado = 0;
};
