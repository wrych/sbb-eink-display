#include "Settings.h"
#include <Preferences.h>

Preferences preferences;

// Define the globals
String WIFI_SSID = "";
String WIFI_PASS = "";
String STATION_NAME = "Zuerich HB";
int FETCH_LIMIT = 7;
long REFRESH_MS = 7 * 60 * 1000;
bool WLAN_QR_ENABLED = false;
uint8_t WLAN_QR_BITMAP[256] = {0};
int WLAN_QR_SIZE = 0;

// Region / Pins
const int MAX_DEST_LEN = 21;
const char *TIMEZONE_STR = "CET-1CEST,M3.5.0/2,M10.5.0/3";

void loadSettings()
{
    preferences.begin("sbb_config", true); // Read-only mode = true

    // Load string with default backup
    // Note: If keys don't exist, it returns the default value passed in 2nd arg
    WIFI_SSID = preferences.getString("ssid", WIFI_SSID);
    WIFI_PASS = preferences.getString("pass", WIFI_PASS);
    STATION_NAME = preferences.getString("station", STATION_NAME);

    // Load Refresh time (stored in minutes to save space/logic complexity?)
    int refresh_min = preferences.getInt("refresh_min", 7);
    REFRESH_MS = refresh_min * 60 * 1000;

    WLAN_QR_ENABLED = preferences.getBool("qr_enabled", false);
    WLAN_QR_SIZE = preferences.getInt("qr_size", 0);
    preferences.getBytes("qr_bitmap", WLAN_QR_BITMAP, 256);

    preferences.end();

    Serial.println("--- Loaded Settings ---");
    Serial.println("SSID: " + WIFI_SSID);
    Serial.println("Station: " + STATION_NAME);
    Serial.println("Refresh: " + String(refresh_min) + " min");
    Serial.println("-----------------------");
}

void saveSettings(String new_ssid, String new_pass, String new_station, int new_refresh_min)
{
    preferences.begin("sbb_config", false); // Read-write

    if (new_ssid.length() > 0)
    {
        preferences.putString("ssid", new_ssid);
        WIFI_SSID = new_ssid;
    }
    if (new_pass.length() > 0)
    {
        preferences.putString("pass", new_pass);
        WIFI_PASS = new_pass;
    }
    if (new_station.length() > 0)
    {
        preferences.putString("station", new_station);
        STATION_NAME = new_station;
    }
    if (new_refresh_min > 0)
    {
        preferences.putInt("refresh_min", new_refresh_min);
        REFRESH_MS = new_refresh_min * 60 * 1000;
    }

    // Invalidation logic: if SSID or Pass actually changed
    if ((new_ssid.length() > 0 && new_ssid != WIFI_SSID) ||
        (new_pass.length() > 0 && new_pass != WIFI_PASS))
    {
        WLAN_QR_ENABLED = false;
        preferences.putBool("qr_enabled", false);
        Serial.println("WLAN Settings changed -> QR Code Invalidated");
    }

    preferences.end();
    Serial.println("Settings Saved to NVS");
}

void saveWLANQR()
{
    preferences.begin("sbb_config", false);
    preferences.putBool("qr_enabled", WLAN_QR_ENABLED);
    preferences.putInt("qr_size", WLAN_QR_SIZE);
    preferences.putBytes("qr_bitmap", WLAN_QR_BITMAP, 256);
    preferences.end();
    Serial.println("WLAN QR Saved to NVS");
}
