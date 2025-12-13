#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

// --- WIFI ---
extern String WIFI_SSID;
extern String WIFI_PASS;

// Station & Display Settings
extern String STATION_NAME;
extern int FETCH_LIMIT;
extern long REFRESH_MS;

extern const int MAX_DEST_LEN;

// --- FUNCTIONS ---
void loadSettings();
void saveSettings(String new_ssid, String new_pass, String new_station, int new_refresh_min);


// --- PINS (ESP32-S3 SuperMini Right-Side Cluster) ---
const int PIN_RGB_LED = 48; // Built-in RGB LED (WS2812) on SuperMini
const int PIN_TOUCH = 10;

const int PIN_EINK_BUSY = 4;
const int PIN_EINK_CS = 5;
const int PIN_EINK_DIN = 7;
const int PIN_EINK_RST = 16;
const int PIN_EINK_DC = 17;
const int PIN_EINK_CLK = 18;

// --- REGION ---
extern const char *TIMEZONE_STR;

#endif