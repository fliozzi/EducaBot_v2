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

  // Detectar flancos para medir sostenido
  if (start && !startAnterior) {
    startPresionadoMs = millis();
  }
  if (a && !aAnterior) {
    aPresionadoMs = millis();
  }

  switch (estado) {

  // ============================================================
  case Estado::NORMAL: {
    // A 5 s → borrar calibración
    if (a && millis() - aPresionadoMs >= 5000) {
      estado = Estado::CAL_CLEAR;
      tiempoEstado = millis();
      ultimoFlash = millis();
      flashState = false;
      break;
    }
    // SELECT → entrar en calibración
    if (select && !selectAnterior) {
      estado = Estado::CAL_WAIT;
      tiempoEstado = millis();
    }
    break;
  }

  // ============================================================
  case Estado::CAL_CLEAR: {
    // Flash rosa (~6 Hz) en los 6 LEDs durante 3 s
    const unsigned long ahora = millis();
    if (ahora - ultimoFlash >= 80) {
      ultimoFlash = ahora;
      flashState = !flashState;
      if (flashState) {
        for (uint8_t i = 0; i < 6; ++i) {
          tira.setPixelColor(i, tira.Color(255, 20, 147));
        }
      } else {
        tira.clear();
      }
      tira.show();
    }
    // A los 3 s → borrar NVS y volver
    if (ahora - tiempoEstado >= 3000) {
      zero[0] = 0;
      span[0] = 180;
      zero[1] = 0;
      span[1] = 180;
      guardar(); // persistir defaults en NVS (toma efecto inmediato)
      limpiarTira(tira);
      estado = Estado::NORMAL;
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
    // START (flanco): SAT1→toggle SAT2,  SAT2→salir
    if (start && !startAnterior) {
      if (servoIdx == 0) {
        servoIdx = 1;
      } else {
        limpiarTira(tira);
        estado = Estado::NORMAL;
        break;
      }
    }

    // START sostenido 2 s → salir sin calibrar (universal)
    if (start && millis() - startPresionadoMs >= 2000) {
      limpiarTira(tira);
      estado = Estado::NORMAL;
      break;
    }

    // Flash blanco visible (~6 Hz) en los 6 LEDs durante 3 s
    const unsigned long ahora = millis();
    if (ahora - ultimoFlash >= 80) {
      ultimoFlash = ahora;
      flashState = !flashState;
      if (flashState) {
        for (uint8_t i = 0; i < 6; ++i) {
          tira.setPixelColor(i, tira.Color(255, 255, 255));
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
    // START (flanco): SAT1→toggle SAT2,  SAT2→salir
    if (start && !startAnterior) {
      if (servoIdx == 0) {
        servoIdx = 1;
      } else {
        limpiarTira(tira);
        estado = Estado::NORMAL;
        break;
      }
    }

    limpiarTira(tira);

    // LED6 (idx 5): amarillo=SAT1, azul=SAT2
    if (servoIdx == 0) {
      tira.setPixelColor(5, tira.Color(255, 200, 0));
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
      s.writeFromJoystickSmooth(eje, 0, 180, gamepad.r2, servoIdx == 0, true);
    }

    // START sostenido 2 s → salir sin calibrar
    if (start && millis() - startPresionadoMs >= 2000) {
      limpiarTira(tira);
      estado = Estado::NORMAL;
      break;
    }

    // A → guardar zero
    if (a && !aAnterior) {
      const uint8_t eje = (servoIdx == 0) ? gamepad.ly : gamepad.lx;
      if (servoIdx == 0) {
        zero[0] = ((128 - (int32_t)eje) * 180) / 128;
      } else {
        zero[1] = (((int32_t)eje - 128) * 180) / 127;
      }

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
    // START (flanco): SAT1→toggle SAT2 y volver a CAL_ZERO,  SAT2→salir
    if (start && !startAnterior) {
      if (servoIdx == 0) {
        servoIdx = 1;
        estado = Estado::CAL_ZERO;
        tiempoEstado = millis();
        break;
      } else {
        limpiarTira(tira);
        estado = Estado::NORMAL;
        break;
      }
    }

    limpiarTira(tira);

    // LED6 (idx 5): amarillo=SAT1, azul=SAT2
    if (servoIdx == 0) {
      tira.setPixelColor(5, tira.Color(255, 200, 0));
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
      s.writeFromJoystickSmooth(eje, 0, 180, gamepad.r2, servoIdx == 0, true);
    }

    // START sostenido 2 s → salir sin calibrar
    if (start && millis() - startPresionadoMs >= 2000) {
      limpiarTira(tira);
      estado = Estado::NORMAL;
      break;
    }

    // A → guardar span y salir
    if (a && !aAnterior) {
      const uint8_t eje = (servoIdx == 0) ? gamepad.ly : gamepad.lx;
      if (servoIdx == 0) {
        span[0] = ((128 - (int32_t)eje) * 180) / 128;
      } else {
        span[1] = (((int32_t)eje - 128) * 180) / 127;
      }

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
