#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <vector>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
int connectedClients = 0;
bool deviceConnected = false;
uint32_t value = 0;

struct ClientInfo {
    String mac;
    int connectedID;
    uint16_t connInterval;
    uint16_t latency;
    uint16_t timeout;
};

std::vector<ClientInfo> clients;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, ble_gap_conn_desc* desc) override {
        Serial.println("=== Device Connected ===");

        ClientInfo ci;
        ci.mac = BLEAddress(desc->peer_ota_addr).toString().c_str();
        ci.connectedID = pServer->getConnId();
        ci.connInterval = desc->conn_itvl;
        ci.latency = desc->conn_latency;
        ci.timeout = desc->supervision_timeout;

        clients.push_back(ci);
        connectedClients++;

        // Kirim JSON semua client
        String json = "[";
        for (size_t i = 0; i < clients.size(); i++) {
            json += "{";
            json += "\"mac\":\"" + clients[i].mac + "\",";
            json += "\"interval\":" + String(clients[i].connInterval) + ",";
            json += "\"latency\":" + String(clients[i].latency) + ",";
            json += "\"timeout\":" + String(clients[i].timeout);
            json += "}";
            if (i < clients.size() - 1) json += ",";
        }
        json += "]";
        Serial.println(json);

        BLEDevice::startAdvertising();
    }

    void onDisconnect(BLEServer* pServer) override {
        Serial.println("=== Device Disconnected ===");
        if (!clients.empty()) clients.pop_back();
        connectedClients--;
    }
};

void setup() {
    Serial.begin(115200);

    BLEDevice::init("ESP32");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();

    Serial.println("Waiting for client connections to notify...");
    Serial.println("Type 'disconnect <connId>' to disconnect a client.");
}

void loop() {
    // Notify value to all connected clients
    if (connectedClients > 0) {
        // Serial.print("Notifying value: ");
        // Serial.print(value);
        // Serial.print(" to ");
        // Serial.print(connectedClients);
        // Serial.println(" client(s)");
        pCharacteristic->setValue((uint8_t *)&value, 4);
        pCharacteristic->notify();
        value++;
        delay(100);
    }

    // Interaktif: disconnect client via serial
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.startsWith("disconnect ")) {
            int id = cmd.substring(11).toInt();
            Serial.print("Trying to disconnect connId: ");
            Serial.println(id);
            pServer->disconnect(id);
        }
    }

    // Restart advertising jika tidak ada client
    if (connectedClients == 0 && deviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("No clients connected, restarting advertising");
        deviceConnected = false;
    }

    if (connectedClients > 0 && !deviceConnected) {
        deviceConnected = true;
    }
}
