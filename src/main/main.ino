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

struct DeviceInfo {
  int RSSI;
  int TXPower;
  String name;
};
std::map<String, DeviceInfo> scannedDevices;

std::map<int, int> connIdToIndex;

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
BLEScan *pBLEScan;

int connectedClients = 0;
int lastconnectedClients;

StaticJsonDocument<2048> doc;
JsonArray devices;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ------------------ CALLBACKS -------------------
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) override {
    connectedClients++;
    if (devices.isNull()) devices = doc.createNestedArray("Devices");

    JsonObject device = devices.createNestedObject();
    int connId = pServer->getConnId();
    device["connectedID"] = connId;
    device["connInterval"] = desc->conn_itvl;
    device["mac"] = BLEAddress(desc->peer_ota_addr).toString();
    device["latency"] = desc->conn_latency;
    device["timeout"] = desc->supervision_timeout;

    // if (!scannedDevices.empty()) {
    //   auto strongest = scannedDevices.begin();
    //   for (auto it = scannedDevices.begin(); it != scannedDevices.end(); ++it) {
    //     if (it->second.RSSI > strongest->second.RSSI) strongest = it;
    //   }
    //   device["mac"] = strongest->first;
    //   device["name"] = strongest->second.name;
    //   device["RSSI"] = strongest->second.RSSI;
    //   device["TXPower"] = strongest->second.TXPower;
    // } else {
    //   device["mac"] = "Unknown";
    //   device["name"] = "Unknown";
    // }

    connIdToIndex[connId] = devices.size() - 1;

    doc["Status"] = "NEW_CONNECT";
    doc["connectedClients"] = connectedClients;

    serializeJson(doc, Serial);
    Serial.println();
    BLEDevice::startAdvertising();
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
    String name = advertisedDevice.haveName() ? advertisedDevice.getName() : "Unknown";
    int rssi = advertisedDevice.getRSSI();
    int txPower = advertisedDevice.getTXPower();

    scannedDevices[mac] = { rssi, txPower, name };

    for (auto &pair : connIdToIndex) {
      int index = pair.second;
      JsonObject device = devices[index];
      if (scannedDevices.count(mac)) {
        device["Name"] = scannedDevices[mac].name;
        device["RSSI"] = scannedDevices[mac].RSSI;
        device["TXPower"] = scannedDevices[mac].TXPower;
      }
    }
  }
};

// ------------------ SETUP -------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  BLEDevice::init("ESP32");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

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

// ------------------ LOOP -------------------
unsigned long lastJsonTime = 0;
const unsigned long scanInterval = 1000;

void loop() {
  digitalWrite(LED_BUILTIN, connectedClients > 0 ? HIGH : LOW);

  // kirim update JSON tiap 1 detik
  // if (millis() - lastJsonTime > scanInterval) {
  //   if (connectedClients == 0) {
  //     doc["Status"] = "NO_CONNECTED";
  //   }
  //   lastJsonTime = millis();
  //   doc["connectedClients"] = connectedClients;
  //   // serializeJsonPretty(doc, Serial);
  //   serializeJson(doc, Serial);
  //   Serial.println();
  // }

  if (lastconnectedClients != connectedClients && connectedClients == 0) {
    doc["Status"] = "NO_CONNECTED";
    serializeJson(doc, Serial);
    Serial.println();
    lastconnectedClients = connectedClients;
  }

  // cek input dari serial untuk disconnect
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("DISCONNECT")) {
      int id = input.substring(10).toInt();
      if (id > 0) {
        Serial.printf("Disconnecting client %d...\n", id);
        pServer->disconnect(id);
      } else {
        Serial.println("Format salah! Gunakan: DISCONNECT <connId>");
      }
    }
  }
}
