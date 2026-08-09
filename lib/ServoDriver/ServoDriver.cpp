// Creado por el Profesor Fernando Angel Liozzi - 2026

#include "ServoDriver.h"

void ServoDriver::begin(uint8_t p) {
  pin = p;
  servo.setPeriodHertz(50);
  servo.attach(pin, 544, 2400);
}

void ServoDriver::writeAngle(uint8_t angulo) {
  servo.write(angulo);
}

void ServoDriver::writeFromJoystick(uint8_t eje, uint16_t zero,
                                    uint16_t span) {
  // El rango puede ser zero < span o zero > span. Calculamos el mapeo
  // lineal de forma que el joystick 0 → zero y joystick 255 → span.
  int32_t rango = (int32_t)span - (int32_t)zero;
  int32_t angulo = (int32_t)zero + ((int32_t)eje * rango) / 255;

  if (angulo < 0)
    angulo = 0;
  else if (angulo > 180)
    angulo = 180;

  servo.write((uint8_t)angulo);
}

void ServoDriver::detach() {
  servo.detach();
}
