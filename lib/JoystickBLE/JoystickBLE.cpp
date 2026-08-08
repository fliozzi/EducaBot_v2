// Creado por el Profesor Fernando Angel Liozzi - 2026

#include "JoystickBLE.h"

#include <Arduino.h>
#include <NimBLEUtils.h>

const NimBLEUUID JoystickBLE::UUID_HID((uint16_t)0x1812);

// ================================================================
//  Callbacks BLE internos (implementación oculta)
// ================================================================
namespace {

// Puntero estático a la instancia activa para el callback de notify
// (evita usar lambdas con captura, que no son portables a todas las
// versiones de NimBLE).
static JoystickBLE *s_activeInstance = nullptr;

static void s_onNotify(NimBLERemoteCharacteristic *car, uint8_t *datos,
                       size_t largo, bool esNotif) {
  if (s_activeInstance) {
    s_activeInstance->onNotificacion(car, datos, largo, esNotif);
  }
}

class ClienteCB_Interno : public NimBLEClientCallbacks {
  JoystickBLE *owner;

public:
  ClienteCB_Interno(JoystickBLE *o) : owner(o) {}
  void onConnect(NimBLEClient *) override { Serial.println(">> Conectado."); }
  void onConnectFail(NimBLEClient *, int reason) override {
    Serial.printf(">> Fallo de conexión (reason=%d).\n", reason);
  }
  void onDisconnect(NimBLEClient *, int reason) override {
    owner->onDisconnected(reason);
  }
  bool onConnParamsUpdateRequest(NimBLEClient *,
                                 const ble_gap_upd_params *) override {
    return true;
  }
};

class ScanCB_Interno : public NimBLEScanCallbacks {
  JoystickBLE *owner;

public:
  ScanCB_Interno(JoystickBLE *o) : owner(o) {}
  void onResult(const NimBLEAdvertisedDevice *disp) override {
    owner->onScanResultado(disp);
  }
};

} // namespace

// ================================================================
//  Implementación de JoystickBLE
// ================================================================

void JoystickBLE::iniciar() {
  Serial.begin(115200);
  Serial.println("Iniciando escaneo BLE...");
  Serial.printf("Buscando \"%s\" o cualquier gamepad HID BLE...\n",
                NOMBRE_JOYSTICK);

  NimBLEDevice::init("");
  NimBLEDevice::setSecurityAuth(true, false, true);
  // Sin intercambio de IRK (solo clave de cifrado): evita la resolución de
  // RPA que devolvía la dirección 00:00:00:00:00:00 y rompía la reconexión.
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC);
  NimBLEDevice::deleteAllBonds();

  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new ScanCB_Interno(this));
  iniciarEscaneo();
}

void JoystickBLE::manejar() {
  if (pedirReescaneo) {
    pedirReescaneo = false;
    NimBLEDevice::deleteAllBonds();
    iniciarEscaneo();
  }

  // Reconecta cuando encontró el joystick y aún no está conectado.
  if (encontrado && !conectado() && (millis() - momentoEncontrado) >= 300) {
    conectarYExplorar();
  }
}

// -- métodos privados --

void JoystickBLE::iniciarEscaneo() {
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(50);
  encontrado = false;
  scan->start(0, false);
  Serial.println("Escaneando joystick (poné el control en modo pairing)...");
}

void JoystickBLE::conectarYExplorar() {
  encontrado = false;

  // Reusar el cliente (patrón NimBLE). Crear uno nuevo en cada intento
  // agota los slots de conexión y por eso no reconectaba sin reset.
  if (cliente == nullptr) {
    cliente = NimBLEDevice::createClient();
    cliente->setClientCallbacks(new ClienteCB_Interno(this), false);
  }

  Serial.println("Conectando al joystick...");
  if (!cliente->connect(objetivo)) {
    Serial.println("No se pudo conectar. Vuelvo a escanear...");
    iniciarEscaneo();
    return;
  }

  if (!cliente->secureConnection()) {
    Serial.println(
        "Aviso: no se pudo emparejar/cifrar (puede que no lleguen datos).");
  }
  Serial.println("Conectado. Explorando servicios...");

  s_activeInstance = this;

  int suscritas = 0;
  for (auto servicio : cliente->getServices(true)) {
    Serial.printf("Servicio: %s\n", servicio->getUUID().toString().c_str());
    for (auto caracteristica : servicio->getCharacteristics(true)) {
      Serial.printf("  Característica: %s (notify=%d)\n",
                    caracteristica->getUUID().toString().c_str(),
                    caracteristica->canNotify());
      if (caracteristica->canNotify()) {
        if (caracteristica->subscribe(true, s_onNotify)) {
          ++suscritas;
        }
      }
    }
  }
  Serial.printf("Listo: %d notificaciones activas. Mové sticks y botones.\n",
                suscritas);
}

void JoystickBLE::onNotificacion(NimBLERemoteCharacteristic * /*car*/,
                                 uint8_t *datos, size_t largo,
                                 bool /*esNotif*/) {
  if (!gamepad.parseFromReport(datos, largo)) {
    return;
  }
  // Imprimimos solo si cambió.
  if (largo == ultimoLargo && memcmp(datos, ultimoReport, largo) == 0) {
    return;
  }
  memcpy(ultimoReport, datos, largo);
  ultimoLargo = largo;
  if (verbose) {
    gamepad.printCompact();
  }
}

void JoystickBLE::onDisconnected(int reason) {
  Serial.printf(">> Desconectado (reason=%d). Reintentaré conectar...\n",
                reason);
  pedirReescaneo = true;
}

void JoystickBLE::onScanResultado(const NimBLEAdvertisedDevice *disp) {
  const std::string nombre = disp->getName();
  const bool esHID = disp->isAdvertisingService(UUID_HID);
  const bool nombreCoincide = (nombre.find("GamePad") != std::string::npos) ||
                              (nombre.find("Gamepad") != std::string::npos);
  if (!esHID && !nombreCoincide) {
    return;
  }
  const NimBLEAddress dir = disp->getAddress();
  Serial.printf(">>> Candidato: \"%s\"  %s  tipo=%u  null=%d  rpa=%d  HID=%d\n",
                nombre.c_str(), dir.toString().c_str(), dir.getType(),
                dir.isNull(), dir.isRpa(), esHID);
  if (objetivo != nullptr) {
    delete objetivo;
  }
  objetivo = new NimBLEAdvertisedDevice(*disp);
  momentoEncontrado = millis();
  encontrado = true;
  NimBLEDevice::getScan()->stop();
}
