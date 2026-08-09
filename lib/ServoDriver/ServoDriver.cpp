// Creado por el Profesor Fernando Angel Liozzi - 2026

#include "ServoDriver.h"

void ServoDriver::begin(uint8_t p) {
  pin = p;
  servo.setPeriodHertz(50);
  servo.attach(pin, 544, 2400);
}

void ServoDriver::writeAngle(uint8_t angulo) { servo.write(angulo); }

void ServoDriver::writeFromJoystick(uint8_t eje, uint16_t zero, uint16_t span,
                                    bool ejeLY) {
  // --- Mapeo centro→extremo (solo mitad del recorrido del stick) ---
  //   LY (ejeLY=true):  128→zero,   0→span  (centro→arriba)
  //   LX (ejeLY=false): 128→zero, 255→span  (centro→derecha)
  int32_t rango = (int32_t)span - (int32_t)zero;
  int32_t delta;
  int32_t divisor;

  if (ejeLY) {
    // LY: solo la mitad superior del stick (0..128)
    if (eje >= 128) {
      servo.write((uint8_t)zero);
      return;
    }
    delta = (int32_t)(128 - eje);
    divisor = 128;
  } else {
    // LX: solo la mitad derecha del stick (128..255)
    if (eje <= 128) {
      servo.write((uint8_t)zero);
      return;
    }
    delta = (int32_t)(eje - 128);
    divisor = 127;
  }

  int32_t angulo = (int32_t)zero + (delta * rango) / divisor;
  if (angulo < 0)
    angulo = 0;
  else if (angulo > 180)
    angulo = 180;

  servo.write((uint8_t)angulo);
}

void ServoDriver::writeFromJoystickSmooth(uint8_t eje, uint16_t zero,
                                          uint16_t span, uint8_t r2, bool ejeLY,
                                          bool enCalibracion) {
  // --- Calcular ángulo objetivo (×16) con mapeo centro→extremo ---
  //   LY (ejeLY=true):  128→zero,   0→span
  //   LX (ejeLY=false): 128→zero, 255→span
  int32_t rango = (int32_t)span - (int32_t)zero;
  int32_t delta;
  int32_t divisor;

  if (ejeLY) {
    if (eje >= 128) {
      delta = 0;
      divisor = 1;
    } else {
      delta = (int32_t)(128 - eje);
      divisor = 128;
    }
  } else {
    if (eje <= 128) {
      delta = 0;
      divisor = 1;
    } else {
      delta = (int32_t)(eje - 128);
      divisor = 127;
    }
  }

  int32_t target = (int32_t)zero + (delta * rango) / divisor;
  if (target < 0)
    target = 0;
  else if (target > 180)
    target = 180;
  uint16_t target_x16 = (uint16_t)target * 16;

  const unsigned long ahora = millis();

  // Primera llamada o después de detach: saltar al objetivo
  if (!smoothInit) {
    currentAngle_x16 = target_x16;
    servo.write(currentAngle_x16 / 16);
    lastSmoothMs = ahora;
    smoothInit = true;
    return;
  }

  // --- Actualizar cada 5 ms (200 Hz) como máximo ---
  //    Así el paso es independiente de la velocidad del loop.
  if (ahora - lastSmoothMs < 5) {
    return;
  }
  lastSmoothMs = ahora;

  // --- Tamaño de paso según R2 (lineal, solo enteros) ---
  //    Normal:     R2=0→29 (1.8°/tick), R2=255→6  (0.4°/tick, 1/5×)
  //    Calibración: R2=0→29 (1.8°/tick), R2=255→3  (0.2°/tick, 1/10×)
  const uint16_t maxStep_x16 = 29;
  const uint16_t minStep_x16 = enCalibracion ? 3 : 6;
  uint16_t step_x16 =
      maxStep_x16 - ((uint32_t)(maxStep_x16 - minStep_x16) * r2) / 255;

  // --- Interpolar hacia el objetivo ---
  if (currentAngle_x16 < target_x16) {
    currentAngle_x16 += step_x16;
    if (currentAngle_x16 > target_x16)
      currentAngle_x16 = target_x16;
  } else if (currentAngle_x16 > target_x16) {
    if (step_x16 > currentAngle_x16)
      currentAngle_x16 = 0;
    else
      currentAngle_x16 -= step_x16;
    if (currentAngle_x16 < target_x16)
      currentAngle_x16 = target_x16;
  }

  servo.write(currentAngle_x16 / 16);
}

void ServoDriver::detach() {
  servo.detach();
  smoothInit = false;
}
