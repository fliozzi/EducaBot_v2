// Creado por el Profesor Fernando Angel Liozzi - 2026

#include "GamepadData.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>

bool GamepadData::parseFromReport(const uint8_t *datos, size_t largo) {
  if (largo != 10) {
    return false;
  }
  lx = datos[0];
  ly = datos[1];
  rx = datos[2];
  ry = datos[3];
  dpad = datos[4];
  botones = ((uint16_t)datos[6] << 8) | datos[5];
  r2 = datos[7];
  l2 = datos[8];
  return true;
}

void GamepadData::printToSerial() const {
  char buf[96] = "";
  nombresBotones(botones, buf, sizeof(buf));
  Serial.printf(
      "LX=%u LY=%u RX=%u RY=%u | L2=%u R2=%u | DPAD=%s | Botones: %s\n", lx, ly,
      rx, ry, l2, r2, nombreDpad(dpad), buf);
}

void GamepadData::printCompact() const {
  char nombresBtn[17] = "";
  nombresBotones(botones, nombresBtn, sizeof(nombresBtn));
  char buf[96];
  snprintf(buf, sizeof(buf), "L:%3u,%3u R:%3u,%3u T:%3u,%3u D:%-3s B:%-16s", lx,
           ly, rx, ry, l2, r2, nombreDpad(dpad), nombresBtn);
  // Línea de ancho fijo con salto real para que el monitor no se encabalgue.
  Serial.printf("%-80s\r\n", buf);
}

const char *GamepadData::nombreDpad(uint8_t v) {
  switch (v) {
  case 0:
    return "U";
  case 1:
    return "UR";
  case 2:
    return "R";
  case 3:
    return "DR";
  case 4:
    return "D";
  case 5:
    return "DL";
  case 6:
    return "L";
  case 7:
    return "UL";
  default:
    return "-";
  }
}

void GamepadData::nombresBotones(uint16_t mascara, char *buf, size_t bufSize) {
  buf[0] = '\0';
  if (mascara & 0x0001)
    strcat(buf, "A ");
  if (mascara & 0x0002)
    strcat(buf, "B ");
  if (mascara & 0x0008)
    strcat(buf, "X ");
  if (mascara & 0x0010)
    strcat(buf, "Y ");
  if (mascara & 0x0040)
    strcat(buf, "L1 ");
  if (mascara & 0x0080)
    strcat(buf, "R1 ");
  if (mascara & 0x0100)
    strcat(buf, "L2 ");
  if (mascara & 0x0200)
    strcat(buf, "R2 ");
  if (mascara & 0x0400)
    strcat(buf, "Select ");
  if (mascara & 0x0800)
    strcat(buf, "Start ");
  if (mascara & 0x2000)
    strcat(buf, "L3 ");
  if (mascara & 0x4000)
    strcat(buf, "R3 ");
  if (buf[0] == '\0') {
    strncpy(buf, "-", bufSize);
  }
}
