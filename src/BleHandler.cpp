#include "BleHandler.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Settings.h"

// UUIDs
#define SERVICE_UUID        "91bad492-b950-4226-aa2b-4ed124237670"
#define CHAR_SSID_UUID      "91bad492-b950-4226-aa2b-4ed124237671"
#define CHAR_PASS_UUID      "91bad492-b950-4226-aa2b-4ed124237672"
#define CHAR_STATION_UUID   "91bad492-b950-4226-aa2b-4ed124237673"
#define CHAR_REFRESH_UUID   "91bad492-b950-4226-aa2b-4ed124237674"
#define CHAR_ACTION_UUID    "91bad492-b950-4226-aa2b-4ed124237675"

BLEServer* pServer = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool shouldSave = false;

// Buffer for Password (write-only)
String newWifiPass = "";

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Client Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Client Disconnected");
      // Restart advertising
      pServer->getAdvertising()->start();
    }
};

class ActionCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue(); // Get std::string
        String strVal = String(value.c_str());           // Convert to Arduino String
        
        if (strVal.length() > 0) {
            Serial.print("Action: ");
            Serial.println(strVal);
            if (strVal == "SAVE") {
                shouldSave = true;
            }
        }
    }
};

class SettingsCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        String strVal = String(value.c_str());
        
        BLEUUID uuid = pCharacteristic->getUUID();
        
        if (uuid.equals(BLEUUID(CHAR_SSID_UUID))) {
             WIFI_SSID = strVal; // Update Global
             Serial.println("New SSID: " + strVal);
        }
        else if (uuid.equals(BLEUUID(CHAR_PASS_UUID))) {
             newWifiPass = strVal; // Buffer password
             Serial.println("New Pass: Received");
        }
        else if (uuid.equals(BLEUUID(CHAR_STATION_UUID))) {
             STATION_NAME = strVal;
             Serial.println("New Station: " + strVal);
        }
        else if (uuid.equals(BLEUUID(CHAR_REFRESH_UUID))) {
             int val = strVal.toInt();
             if (val > 0) REFRESH_MS = val * 60 * 1000;
             Serial.println("New Refresh: " + String(val));
        }
    }
};

void BleHandler::begin() {
  BLEDevice::init("SBB_Display_Config");
  
  // Security - Enable Bonding (Just Works)
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  pSecurity->setCapability(ESP_IO_CAP_NONE);
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // SSID
  BLECharacteristic *pSsid = pService->createCharacteristic(
                                         CHAR_SSID_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pSsid->setValue(WIFI_SSID.c_str());
  pSsid->setCallbacks(new SettingsCallback());

  // Password (Write Only for security)
  BLECharacteristic *pPass = pService->createCharacteristic(
                                         CHAR_PASS_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pPass->setCallbacks(new SettingsCallback());

  // Station
  BLECharacteristic *pStation = pService->createCharacteristic(
                                         CHAR_STATION_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pStation->setValue(STATION_NAME.c_str());
  pStation->setCallbacks(new SettingsCallback());

  // Refresh (Minutes)
  BLECharacteristic *pRefresh = pService->createCharacteristic(
                                         CHAR_REFRESH_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pRefresh->setValue(String(REFRESH_MS / 60000).c_str());
  pRefresh->setCallbacks(new SettingsCallback());

  // Action (Write "SAVE" to trigger save)
  BLECharacteristic *pAction = pService->createCharacteristic(
                                         CHAR_ACTION_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pAction->setCallbacks(new ActionCallback());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Service Started. Waiting for clients...");
}

void BleHandler::saveAndReboot() {
    Serial.println("Saving settings and rebooting...");
    
    // Save current globals to NVS
    // Note: For password, check if we received a new one
    String passToSave = (newWifiPass.length() > 0) ? newWifiPass : WIFI_PASS;
    
    saveSettings(WIFI_SSID, passToSave, STATION_NAME, (int)(REFRESH_MS/60000));
    
    delay(1000);
    ESP.restart();
}

void BleHandler::update() {
    if (shouldSave) {
        shouldSave = false;
        saveAndReboot();
    }
}

void BleHandler::stop() {
    BLEDevice::deinit(true);
}
