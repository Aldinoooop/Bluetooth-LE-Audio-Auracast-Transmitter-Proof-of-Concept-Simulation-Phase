#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <map>

#define AUDIO_FRAME_SIZE 60
uint8_t audioBuffer[AUDIO_FRAME_SIZE];
int bufferIndex = 0;
bool readingAudio = false;

// Simpan data scan: MAC -> RSSI/TX/Name
struct DeviceInfo {
  int RSSI;
  int TXPower;
  String name;
};
std::map<String, DeviceInfo> scannedDevices;

// Simpan device yang connect: connId -> JSON index
std::map<int, int> connIdToIndex;

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;

int connectedClients = 0;

StaticJsonDocument<2048> doc;
JsonArray devices;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) override {
    connectedClients++;

    if (devices.isNull()) devices = doc.createNestedArray("Devices");

    JsonObject device = devices.createNestedObject();
    int connId = pServer->getConnId();
    device["connectedID"] = connId;
    device["connInterval"] = desc->conn_itvl;
    device["latency"] = desc->conn_latency;
    device["timeout"] = desc->supervision_timeout;

    // Gunakan MAC dari scannedDevices jika ada RSSI terakhir
    // Biasanya kita tidak punya MAC real connect, bisa pakai RSSI terkuat
    if (!scannedDevices.empty()) {
      auto strongest = scannedDevices.begin();
      for (auto it = scannedDevices.begin(); it != scannedDevices.end(); ++it) {
        if (it->second.RSSI > strongest->second.RSSI) strongest = it;
      }
      device["mac"] = strongest->first;
      device["name"] = strongest->second.name;
      device["RSSI"] = strongest->second.RSSI;
      device["TXPower"] = strongest->second.TXPower;
    } else {
      device["mac"] = "Unknown";
      device["name"] = "Unknown";
    }

    connIdToIndex[connId] = devices.size() - 1;

    doc["Status"] = "NEW_CONNECT";
    doc["connectedClients"] = connectedClients;

    serializeJson(doc, Serial);
    Serial.println();
  }

  void onDisconnect(BLEServer *pServer) override {
    connectedClients--;
    doc["Status"] = "DEVICE_DISCONNECTED";
    doc["connectedClients"] = connectedClients;

    int connId = pServer->getConnId();
    if (connIdToIndex.count(connId)) {
      int index = connIdToIndex[connId];
      JsonArray newDevices = doc.createNestedArray("Devices");
      for (int i = 0; i < devices.size(); i++) {
        if (i == index) continue;
        JsonObject oldDevice = devices[i];
        JsonObject newDevice = newDevices.createNestedObject();
        newDevice["connectedID"] = oldDevice["connectedID"];
        newDevice["connInterval"] = oldDevice["connInterval"];
        newDevice["latency"] = oldDevice["latency"];
        newDevice["timeout"] = oldDevice["timeout"];
        if (oldDevice.containsKey("RSSI")) newDevice["RSSI"] = oldDevice["RSSI"];
        if (oldDevice.containsKey("TXPower")) newDevice["TXPower"] = oldDevice["TXPower"];
        if (oldDevice.containsKey("mac")) newDevice["mac"] = oldDevice["mac"];
        if (oldDevice.containsKey("name")) newDevice["name"] = oldDevice["name"];
      }
      devices = newDevices;
      connIdToIndex.erase(connId);
    }

    serializeJson(doc, Serial);
    Serial.println();
  }
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    String mac = advertisedDevice.getAddress().toString();
    // String name = advertisedDevice.getName();
    String name = advertisedDevice.haveName() ? advertisedDevice.getName() : "Unknown";
    int rssi = advertisedDevice.getRSSI();
    int txPower = advertisedDevice.getTXPower();

    // Simpan ke scannedDevices
    scannedDevices[mac] = { rssi, txPower, name };

    for (auto &pair : connIdToIndex) {
      int index = pair.second;
      JsonObject device = devices[index];
      // Di sini kita tidak bisa cocokkan MAC karena connect MAC random,
      // tapi bisa update semua RSSI terakhir dari scannedDevices
      if (scannedDevices.count(mac)) {
        device["Name"] = scannedDevices[mac].name;
        device["RSSI"] = scannedDevices[mac].RSSI;
        device["TXPower"] = scannedDevices[mac].TXPower;
      }
    }
  }
};

BLEScan *pBLEScan;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  BLEDevice::init("ESP32");

  // Setup scan
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  // Setup server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
}

unsigned long lastScanTime = 0;
const unsigned long scanInterval = 1000;  // 1 detik

void loop() {
  digitalWrite(LED_BUILTIN, connectedClients > 0 ? HIGH : LOW);

  // Scan BLE setiap 1 detik
  // if (millis() - lastScanTime > scanInterval) {
  //   lastScanTime = millis();
  //   pBLEScan->start(0, nullptr, false);  // non-blocking scan
  // }

  pBLEScan->start(2, false);  // non-blocking scan
  // Kirim JSON update ke Serial
  static unsigned long lastJsonTime = 0;
  if (millis() - lastJsonTime > scanInterval) {
    lastJsonTime = millis();
    doc["connectedClients"] = connectedClients;
    serializeJson(doc, Serial);
    Serial.println();
  }
}
