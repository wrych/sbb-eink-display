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
volatile bool shouldShowQR = false;

// Pages
enum Page
{
    PAGE_SBB,
    PAGE_QR
};
Page currentPage = PAGE_SBB;
unsigned long qrStartTime = 0;
const unsigned long QR_TIMEOUT_MS = 5 * 60 * 1000; // 5 minutes

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
            if (duration > 8000)
            {
                shouldConfig = true;
            }
            else if (duration > 3000)
            {
                shouldShowQR = true;
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
    statusLed.setState(LED_CONFIG); // Orange Breathing

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
    // CPU Frequency scaling (Battery Optimization)
    // 80MHz is plenty for fetching and e-ink updates while saving power
    setCpuFrequencyMhz(80);

    // Load Settings from NVS
    loadSettings();

    // Init Hardware
    display.begin();
    statusLed.begin(); // Init LED
    pinMode(PIN_TOUCH, INPUT);

    // Check wakeup reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 || wakeup_reason == ESP_SLEEP_WAKEUP_EXT1 || digitalRead(PIN_TOUCH) == HIGH)
    {
        // Simple hold detection in setup
        unsigned long start = millis();
        while (digitalRead(PIN_TOUCH) == HIGH && (millis() - start < 8500))
        {
            delay(10);
        }
        unsigned long duration = millis() - start;

        if (duration > 8000)
        {
            enterConfigMode();
        }
        else if (duration > 3000)
        {
            shouldShowQR = true;
        }
        else if (duration > 50)
        {
            shouldUpdate = true;
        }
    }

    attachInterrupt(digitalPinToInterrupt(PIN_TOUCH), onButton, CHANGE);

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
        statusLed.setState(LED_OFF); // Battery Opt: LED off after connection

        // --- TIME SYNC (REQUIRED FOR TIMESTAMP) ---
        configTime(0, 0, "pool.ntp.org");
        setenv("TZ", TIMEZONE_STR, 1);
        tzset();

        // First Update
        fetchSBB();
        lastUpdate = millis();
    }
}

void goToSleep()
{
    Serial.println("Preparing for Deep Sleep...");
    delay(11000); // Increased to 5s to ensure display refresh completes

    Serial.println("Entering Deep Sleep now.");
    Serial.flush();

    // Shut down hardware
    display.powerDown();

    // Calculate sleep time
    // REFRESH_MS is already in milliseconds
    esp_sleep_enable_timer_wakeup(REFRESH_MS * 1000ULL);

    // Enable Wakeup on button (GPIO 10)
    // ESP32-S3 EXT1 wakeup
    esp_sleep_enable_ext1_wakeup(1ULL << PIN_TOUCH, ESP_EXT1_WAKEUP_ANY_HIGH);

    statusLed.setState(LED_OFF);

    // Final delay to ensure Serial finishes / Display finishes
    delay(100);

    esp_deep_sleep_start();
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

        if (currentPage == PAGE_QR)
        {
            Serial.println("Button Trigger -> Returning to SBB");
            currentPage = PAGE_SBB;
            fetchSBB();
            lastUpdate = millis();
        }
        else
        {
            Serial.println("Button Trigger -> Updating");
            fetchSBB();
            lastUpdate = millis();
        }
    }

    if (shouldShowQR)
    {
        shouldShowQR = false;
        if (WLAN_QR_ENABLED)
        {
            Serial.println("Button Trigger -> Showing QR Page");
            currentPage = PAGE_QR;
            qrStartTime = millis();
            drawQRCodePage();
            display.display();
        }
        else
        {
            Serial.println("QR Not Enabled in Settings");
        }
    }

    // QR Timeout or Button press in QR mode
    if (currentPage == PAGE_QR)
    {
        if (shouldUpdate || (millis() - qrStartTime > QR_TIMEOUT_MS))
        {
            Serial.println("Returning to SBB and Sleeping");
            shouldUpdate = false;
            currentPage = PAGE_SBB;
            fetchSBB();
            goToSleep();
        }
        delay(100);
        return;
    }

    // Default behavior for PAGE_SBB in loop:
    if (currentPage == PAGE_SBB)
    {
        goToSleep();
    }
}