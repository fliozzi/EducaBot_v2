// Creado por el Profesor Fernando Angel Liozzi - 2026

#include "NeoPixelEffects.h"

#include <Arduino.h>

void NeoPixelEffects::begin(uint8_t pin, uint8_t cantidad) {
  numLeds = (cantidad > 6) ? 6 : cantidad;
  tira.setPin(pin);
  tira.begin();
  tira.setBrightness(brilloGlobal);
  tira.clear();
  tira.show();
}

void NeoPixelEffects::setBrightness(uint8_t brillo) {
  brilloGlobal = brillo;
  tira.setBrightness(brilloGlobal);
  tira.show();
}

// ================================================================
//  Rueda de arcoíris (no depende de ColorHSV de la librería)
// ================================================================
static uint32_t ruedaArcoiris(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return Adafruit_NeoPixel::Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return Adafruit_NeoPixel::Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return Adafruit_NeoPixel::Color(pos * 3, 255 - pos * 3, 0);
}

// ================================================================
//  Animación de escaneo (arcoíris giratorio, todos encendidos)
// ================================================================

void NeoPixelEffects::animarEscaneo() {
  const unsigned long ahora = millis();
  if (ahora - ultimoFrameEscaneoMs < 25) {
    return;
  }
  ultimoFrameEscaneoMs = ahora;

  // Brillo suave fijo: no hereda el residual tenue del apagado de conexión.
  tira.setBrightness(brilloEscaneo);

  ++tonoBase; // 0..255, da una vuelta completa cada ~6.4 s

  for (uint8_t i = 0; i < numLeds; ++i) {
    uint8_t pos = (tonoBase + i * (256 / numLeds)) & 0xFF;
    tira.setPixelColor(i, ruedaArcoiris(pos));
  }
  tira.show();
}

// ================================================================
//  Animación de conexión (verde centro->afuera + apagado gradual)
// ================================================================

void NeoPixelEffects::iniciarAnimConectado() {
  faseConectado = FC_PULSO_VERDE;
  pasoConectado = 0;
  inicioFaseMs = millis();
  tira.setBrightness(brilloGlobal);
}

bool NeoPixelEffects::animConectadoTerminada() const {
  return faseConectado == FC_TERMINADO;
}

bool NeoPixelEffects::actualizarAnimConectado() {
  if (faseConectado == FC_NINGUNA || faseConectado == FC_TERMINADO) {
    return false;
  }

  const unsigned long ahora = millis();

  // ------ Fase 1: pulso verde de centro hacia afuera ------
  if (faseConectado == FC_PULSO_VERDE) {
    if (ahora - inicioFaseMs < 40) {
      return false; // 25 fps
    }
    inicioFaseMs = ahora;

    // Gradiente verde fijo: centro fuerte, bordes suaves.
    const uint8_t brilloPorLed[6] = {40, 90, 180, 180, 90, 40};

    // Onda de brillo extra que se mueve del centro hacia afuera.
    // Va del centro (led 2-3) hacia extremos (0,5).
    int8_t onda = pasoConectado;
    for (uint8_t i = 0; i < numLeds; ++i) {
      uint8_t extra = 0;
      if (i == (uint8_t)onda || i == (uint8_t)(numLeds - 1 - onda)) {
        extra = 100;
      }
      uint8_t b = brilloPorLed[i] + extra;
      if (b > 255)
        b = 255;
      tira.setPixelColor(i, tira.Color(0, b, 0));
    }
    tira.show();

    ++pasoConectado;
    if (pasoConectado >= 3) { // ya recorrió centro->extremos
      faseConectado = FC_APAGANDO;
      inicioFaseMs = ahora;
    }
    return false;
  }

  // ------ Fase 2: apagado gradual ------
  if (faseConectado == FC_APAGANDO) {
    if (ahora - inicioFaseMs < 30) {
      return false;
    }
    inicioFaseMs = ahora;

    uint8_t br = tira.getBrightness();
    if (br <= 4) {
      apagar();
      faseConectado = FC_TERMINADO;
      return true; // animación terminada
    }
    tira.setBrightness(br - 4);
    // Mantenemos el gradiente verde
    const uint8_t bpl[6] = {40, 90, 180, 180, 90, 40};
    for (uint8_t i = 0; i < numLeds; ++i) {
      tira.setPixelColor(i, tira.Color(0, bpl[i], 0));
    }
    tira.show();
    return false;
  }

  return false;
}

void NeoPixelEffects::mostrarEstadoControl(bool bloqueoGiro, bool bloqueoAvance,
                                           bool paroTotal) {
  tira.setBrightness(brilloGlobal);
  tira.clear();

  if (bloqueoGiro && numLeds > 0) {
    tira.setPixelColor(0, tira.Color(255, 0, 0));
  }

  // El usuario habla de "LED 6"; como la tira tiene 6 LED, corresponde al
  // último índice válido: 5.
  if (bloqueoAvance && numLeds > 5) {
    tira.setPixelColor(5, tira.Color(255, 0, 0));
  }

  if (paroTotal) {
    const bool encendidas = ((millis() / 500UL) % 2UL) == 0;
    if (encendidas) {
      if (numLeds > 1) {
        tira.setPixelColor(1, tira.Color(255, 96, 0));
      }
      if (numLeds > 4) {
        tira.setPixelColor(4, tira.Color(255, 96, 0));
      }
    }
  }

  tira.show();
}

void NeoPixelEffects::apagar() {
  tira.clear();
  tira.show();
  faseConectado = FC_NINGUNA;
}
