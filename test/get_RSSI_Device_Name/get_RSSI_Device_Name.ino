#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

int scanTime = 10;  // detik
BLEScan *pBLEScan;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Address: ");
    Serial.print(advertisedDevice.getAddress().toString().c_str());
    Serial.print(" | Name: ");
    if (advertisedDevice.haveName()) {
      Serial.print(advertisedDevice.getName().c_str());
    } else {
      Serial.print("(No name)");
    }
    Serial.print(" | RSSI: ");
    Serial.println(advertisedDevice.getRSSI());
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // penting: agar dapat Scan Response (nama)
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // harus <= interval
}

void loop() {
  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  Serial.printf("Scan done! Devices found: %d\n", foundDevices->getCount());
  pBLEScan->clearResults(); // hapus hasil scan biar memori gak penuh
  delay(3000);
}
