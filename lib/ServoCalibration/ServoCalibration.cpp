// Creado por el Profesor Fernando Angel Liozzi - 2026

#include "ServoCalibration.h"

#include <Arduino.h>

#include "ServoDriver.h"

// ================================================================
//  NVS
// ================================================================
static const char *NVS_NS = "servocal";

void ServoCalibration::cargar() {
  Preferences prefs;
  if (!prefs.begin(NVS_NS, true)) {
    return;
  }
  zero[0] = prefs.getUShort("sat1_zero", 0);
  span[0] = prefs.getUShort("sat1_span", 180);
  zero[1] = prefs.getUShort("sat2_zero", 0);
  span[1] = prefs.getUShort("sat2_span", 180);
  prefs.end();
}

void ServoCalibration::guardar() {
  Preferences prefs;
  if (!prefs.begin(NVS_NS, false)) {
    return;
  }
  prefs.putUShort("sat1_zero", zero[0]);
  prefs.putUShort("sat1_span", span[0]);
  prefs.putUShort("sat2_zero", zero[1]);
  prefs.putUShort("sat2_span", span[1]);
  prefs.end();
}

// ================================================================
//  Inicialización
// ================================================================
void ServoCalibration::begin() { cargar(); }

// ================================================================
//  Helpers
// ================================================================
void ServoCalibration::limpiarTira(Adafruit_NeoPixel &tira) {
  tira.clear();
  tira.show();
}

// ================================================================
//  Máquina de estados
// ================================================================
void ServoCalibration::manejar(const GamepadData &gamepad,
                               Adafruit_NeoPixel &tira, ServoDriver &servo1,
                               ServoDriver &servo2) {
  const bool select = gamepad.botonPresionado(BTN_SELECT);
  const bool start = gamepad.botonPresionado(BTN_START);
  const bool a = gamepad.botonPresionado(BTN_A);

  switch (estado) {

  // ============================================================
  case Estado::NORMAL: {
    if (select && !selectAnterior) {
      estado = Estado::CAL_WAIT;
      tiempoEstado = millis();
    }
    break;
  }

  // ============================================================
  case Estado::CAL_WAIT: {
    if (!select) {
      // Soltó antes de los 5 s → cancelar
      estado = Estado::NORMAL;
      break;
    }
    if (millis() - tiempoEstado >= 5000) {
      estado = Estado::CAL_ENTER;
      tiempoEstado = millis();
      ultimoFlash = millis();
      flashState = false;
      servoIdx = 0; // empezar siempre con SAT1
    }
    break;
  }

  // ============================================================
  case Estado::CAL_ENTER: {
    // START alterna SAT1 ↔ SAT2 (flanco)
    if (start && !startAnterior) {
      servoIdx = 1 - servoIdx;
    }

    // Flash blanco ~200 Hz en los 6 LEDs durante 3 s
    const unsigned long ahora = millis();
    if (ahora - ultimoFlash >= 3) {
      ultimoFlash = ahora;
      flashState = !flashState;
      if (flashState) {
        for (uint8_t i = 0; i < 6; ++i) {
          tira.setPixelColor(i, tira.Color(80, 80, 80));
        }
      } else {
        tira.clear();
      }
      tira.show();
    }

    // A los 3 s → calibrar zero
    if (ahora - tiempoEstado >= 3000) {
      limpiarTira(tira);
      estado = Estado::CAL_ZERO;
      tiempoEstado = ahora;
    }
    break;
  }

  // ============================================================
  case Estado::CAL_ZERO: {
    limpiarTira(tira);

    // LED6 (idx 5): rojo=SAT1, azul=SAT2
    if (servoIdx == 0) {
      tira.setPixelColor(5, tira.Color(255, 0, 0));
    } else {
      tira.setPixelColor(5, tira.Color(0, 0, 255));
    }
    // LED0 (idx 0): rojo = esperando zero
    tira.setPixelColor(0, tira.Color(255, 0, 0));
    tira.show();

    // Mover servo activo con el joystick (rango completo 0–180°)
    {
      const uint8_t eje = (servoIdx == 0) ? gamepad.ly : gamepad.lx;
      ServoDriver &s = (servoIdx == 0) ? servo1 : servo2;
      s.writeFromJoystick(eje, 0, 180);
    }

    // A → guardar zero
    if (a && !aAnterior) {
      const uint8_t eje = (servoIdx == 0) ? gamepad.ly : gamepad.lx;
      zero[servoIdx] = ((uint16_t)eje * 180) / 255;

      // LED0 verde (confirmación breve)
      tira.setPixelColor(0, tira.Color(0, 255, 0));
      tira.show();

      estado = Estado::CAL_SPAN;
      tiempoEstado = millis();
    }
    break;
  }

  // ============================================================
  case Estado::CAL_SPAN: {
    limpiarTira(tira);

    // LED6 (idx 5): rojo=SAT1, azul=SAT2
    if (servoIdx == 0) {
      tira.setPixelColor(5, tira.Color(255, 0, 0));
    } else {
      tira.setPixelColor(5, tira.Color(0, 0, 255));
    }
    // LED1 (idx 1): rojo = esperando span
    tira.setPixelColor(1, tira.Color(255, 0, 0));
    tira.show();

    // Mover servo activo con el joystick (rango completo)
    {
      const uint8_t eje = (servoIdx == 0) ? gamepad.ly : gamepad.lx;
      ServoDriver &s = (servoIdx == 0) ? servo1 : servo2;
      s.writeFromJoystick(eje, 0, 180);
    }

    // A → guardar span y salir
    if (a && !aAnterior) {
      const uint8_t eje = (servoIdx == 0) ? gamepad.ly : gamepad.lx;
      span[servoIdx] = ((uint16_t)eje * 180) / 255;

      // LED1 verde (confirmación breve)
      tira.setPixelColor(1, tira.Color(0, 255, 0));
      tira.show();

      guardar();
      estado = Estado::NORMAL;
      tiempoEstado = millis();
    }
    break;
  }

  } // switch

  // Guardar flancos para la próxima iteración
  selectAnterior = select;
  startAnterior = start;
  aAnterior = a;
}
