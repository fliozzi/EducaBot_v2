// Creado por el Profesor Fernando Angel Liozzi - 2026

/// JoystickBLE — manejo de escaneo, conexión y reconexión BLE de un gamepad
/// HID. Internamente usa GamepadData para parsear y mostrar el estado.
/// Uso típico:
///   JoystickBLE joystick;
///   joystick.iniciar();          // en setup()
///   joystick.manejar();          // en loop()
///   const GamepadData& g = joystick.estado();  // datos crudos

#pragma once

#include <NimBLEAdvertisedDevice.h>
#include <NimBLEClient.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>

#include "GamepadData.h"

class JoystickBLE {
public:
  static constexpr const char *NOMBRE_JOYSTICK = "GamePadPlus V3";

  /// Inicializa el stack BLE, borra bonds viejos y arranca el escaneo.
  /// Llamar en setup().
  void iniciar();

  /// Atiende eventos pendientes (conexión, reconexión, procesar reports).
  /// Llamar en loop().
  void manejar();

  const GamepadData &estado() const { return gamepad; }

  /// true cuando el joystick está vinculado y listo.
  bool conectado() const {
    return cliente != nullptr && cliente->isConnected();
  }

  /// Activa/desactiva la telemetría inline del joystick (por defecto ON).
  void setVerbose(bool activo) { verbose = activo; }

  // Llamados desde los callbacks BLE internos.
  void onNotificacion(NimBLERemoteCharacteristic *car, uint8_t *datos,
                      size_t largo, bool esNotif);
  void onDisconnected(int reason);
  void onScanResultado(const NimBLEAdvertisedDevice *disp);

private:
  static const NimBLEUUID UUID_HID;

  GamepadData gamepad;
  uint8_t ultimoReport[20] = {0};
  size_t ultimoLargo = 0;

  NimBLEAdvertisedDevice *objetivo = nullptr;
  NimBLEClient *cliente = nullptr;
  volatile bool encontrado = false;
  volatile bool pedirReescaneo = false;
  volatile unsigned long momentoEncontrado = 0;
  bool verbose = true;

  // -- métodos internos --
  void iniciarEscaneo();
  void conectarYExplorar();
};
