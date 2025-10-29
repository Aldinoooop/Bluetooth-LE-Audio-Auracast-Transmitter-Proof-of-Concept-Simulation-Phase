#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <map>

std::map<int, int> connIdToIndex;  // mapping connId → index di array Devices

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
int connectedClients = 0;
bool deviceConnected = false;
uint32_t value = 0;
unsigned int lastTime = 0;

StaticJsonDocument<512> doc;                           // dokumen utama
JsonArray devices = doc.createNestedArray("Devices");  // array untuk semua device

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) override {
    connectedClients++;

    // Buat object untuk device baru
    JsonObject device = devices.createNestedObject();
    device["mac"] = BLEAddress(desc->peer_ota_addr).toString().c_str();
    device["connectedID"] = pServer->getConnId();
    device["connInterval"] = desc->conn_itvl;
    device["latency"] = desc->conn_latency;
    device["timeout"] = desc->supervision_timeout;

    // Simpan mapping connId → index device
    connIdToIndex[pServer->getConnId()] = devices.size() - 1;

    // Update status umum
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
      // Hapus device dari array (buat ulang array)
      JsonArray newDevices = doc.createNestedArray("Devices");
      for (int i = 0; i < devices.size(); i++) {
        if (i == index) continue;  // skip device yang disconnect
        JsonObject oldDevice = devices[i];
        JsonObject newDevice = newDevices.createNestedObject();
        newDevice["mac"] = oldDevice["mac"];
        newDevice["connectedID"] = oldDevice["connectedID"];
        newDevice["connInterval"] = oldDevice["connInterval"];
        newDevice["latency"] = oldDevice["latency"];
        newDevice["timeout"] = oldDevice["timeout"];
      }
      devices = newDevices;         // update devices
      connIdToIndex.erase(connId);  // hapus mapping
    }

    serializeJson(doc, Serial);
    Serial.println();
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  BLEDevice::init("ESP32");

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

  // Serial.println("Waiting for client connections to notify...");
  // Serial.println("Type 'disconnect <connId>' to disconnect a client.");
}

void loop() {

  if (connectedClients > 0) {
    pCharacteristic->setValue((uint8_t *)&value, 4);
    pCharacteristic->notify();
    value++;
    delay(100);
    

  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    digitalWrite(LED_BUILTIN, HIGH); 
    if (cmd.startsWith("DISCONNECT:")) {
      
      int connId = cmd.substring(11).toInt();
      // Serial.println("Arduino will disconnect connID: " + String(connId));
      // panggil fungsi BLE untuk disconnect
      pServer->disconnect(connId);  // sesuai API ESP32 BLE
    }
  }
  digitalWrite(LED_BUILTIN, LOW); 

  if (connectedClients == 0 && deviceConnected) {
    delay(500);
    pServer->startAdvertising();
    // Serial.println("No clients connected, restarting advertising");
    deviceConnected = false;
  }

  if (connectedClients > 0 && !deviceConnected) {
    deviceConnected = true;
  }

  // if (millis() - lastTime >= 1000) {
  //   // Serial.println(json);
  //   serializeJson(doc, Serial);
  //   Serial.println();
  //   lastTime = millis();
  // }
}
