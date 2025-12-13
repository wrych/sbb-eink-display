#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

#include "Settings.h"
#include "WeAct_EInk.h"
#include "SBB_Logic.h"
#include "Config_GUI.h"
#include "BleHandler.h"

BleHandler ble;
bool configMode = false;

// --- DRIVER ---
WeAct42_Driver display(PIN_EINK_CS, PIN_EINK_DC, PIN_EINK_RST, PIN_EINK_BUSY, PIN_EINK_CLK, PIN_EINK_DIN);

// --- GLOBALS ---
unsigned long lastUpdate = 0;
volatile unsigned long pressStartTime = 0;
volatile bool shouldUpdate = false;
volatile bool shouldConfig = false;

void IRAM_ATTR onButton()
{
    int state = digitalRead(PIN_TOUCH);
    if (state == HIGH) // Pressed (Active HIGH as per original code)
    {
        pressStartTime = millis();
    }
    else // Released (LOW)
    {
        if (pressStartTime > 0)
        {
            unsigned long duration = millis() - pressStartTime;
            if (duration > 3000)
            {
                shouldConfig = true;
            }
            else if (duration > 50)
            { // Debounce 50ms
                shouldUpdate = true;
            }
            pressStartTime = 0; // Reset
        }
    }
}

void enterConfigMode()
{
    configMode = true;
    Serial.println("-- Entering Config Mode --");

    // Stop WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    // Initial Draw
    drawConfigScreen();
    display.display();

    // Start BLE
    ble.begin();
}

// --- SETUP ---
void setup()
{
    Serial.begin(115200);

    // Load Settings from NVS
    loadSettings();

    // Init Hardware
    display.begin();
    pinMode(PIN_TOUCH, INPUT); // Reverting to original INPUT (likely External Pulldown or relying on floating being low enough?)

    // Explicitly reading to check for boot hold (Active HIGH)
    if (digitalRead(PIN_TOUCH) == HIGH)
    {
        Serial.println("Boot Button Held -> Config Mode");
        enterConfigMode();
    }

    attachInterrupt(digitalPinToInterrupt(PIN_TOUCH), onButton, CHANGE);

    delay(1000); // Short delay

    // If not in config mode, connect to WiFi
    if (!configMode)
    {
        Serial.print("Connecting to ");
        Serial.println(WIFI_SSID);
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());

        // Wait for connection with timeout/escape
        unsigned long wifiCheckStart = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");

            // Check for Timeout (30s)
            if (millis() - wifiCheckStart > 30000)
            {
                Serial.println("\nWiFi Timeout (30s) -> Entering Config Mode");
                enterConfigMode();
                break;
            }

            // Allow entering config mode while stuck connecting
            if (shouldConfig)
            {
                enterConfigMode();
                break;
            }
            if (digitalRead(PIN_TOUCH) == HIGH && pressStartTime != 0 && (millis() - pressStartTime > 5000))
            {
                enterConfigMode();
                break;
            }
        }
    }

    if (!configMode && WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi Connected!");

        // --- TIME SYNC (REQUIRED FOR TIMESTAMP) ---
        configTime(0, 0, "pool.ntp.org");
        setenv("TZ", TIMEZONE_STR, 1);
        tzset();

        // First Update
        fetchSBB();
        lastUpdate = millis();
    }
}

// --- LOOP ---
void loop()
{
    // Check for Config Mode Toggle
    if (shouldConfig)
    {
        shouldConfig = false;
        if (configMode)
        {
            Serial.println("Exiting Config Mode (Saving & Restarting...)");
            ble.saveAndReboot();
        }
        else
        {
            enterConfigMode();
        }
        return;
    }

    if (configMode)
    {
        ble.update();
        delay(100);
        return; // Skip normal loop
    }

    if (shouldUpdate)
    {
        delay(200); // UI Debounce visual
        shouldUpdate = false;

        Serial.println("Button Trigger -> Updating");
        fetchSBB();
        lastUpdate = millis();
    }

    if (millis() - lastUpdate > REFRESH_MS)
    {
        Serial.println("Timer -> Auto Refreshing");
        fetchSBB();
        lastUpdate = millis();
    }
}