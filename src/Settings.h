#ifndef SETTINGS_H
#define SETTINGS_H

// --- WIFI ---
const char *WIFI_SSID = "Andy";
const char *WIFI_PASS = "f001#f001";

// Station & Display Settings
const char *STATION_NAME = "Brugg AG";
const int FETCH_LIMIT = 7;             // Rows to display (7 fits 4.2" screen)
const long REFRESH_MS = 5 * 60 * 1000; // Refresh every 5 minutes (300,000 ms)
const int MAX_DEST_LEN = 20;           // Increased from 16 to 20 chars

// --- PINS (ESP32-S3 SuperMini Right-Side Cluster) ---
const int PIN_TOUCH = 10;

const int PIN_EINK_BUSY = 4;
const int PIN_EINK_CS = 5;
const int PIN_EINK_DIN = 7;
const int PIN_EINK_RST = 16;
const int PIN_EINK_DC = 17;
const int PIN_EINK_CLK = 18;

// --- REGION ---
const char *TIMEZONE_STR = "CET-1CEST,M3.5.0/2,M10.5.0/3"; // Zurich

#endif