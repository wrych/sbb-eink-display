#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h> // Required for time functions

#include "Settings.h"
#include "WeAct_EInk.h"

// --- DRIVER ---
WeAct42_Driver display(PIN_EINK_CS, PIN_EINK_DC, PIN_EINK_RST, PIN_EINK_BUSY, PIN_EINK_CLK, PIN_EINK_DIN);

// --- GLOBALS ---
const char *SBB_URL_BASE = "https://transport.opendata.ch/v1/stationboard";
unsigned long lastUpdate = 0;
volatile bool buttonPressed = false;

// --- FONTS ---
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

void IRAM_ATTR onButton()
{
    buttonPressed = true;
}

// --- HELPER: Fix Umlauts (UTF-8 -> ASCII) ---
String utf8ToAscii(String input)
{
    String output = "";
    for (int i = 0; i < input.length(); i++)
    {
        char c = input[i];
        if (c == 0xC3)
        {        // UTF-8 Start Byte
            i++; // Check next byte
            switch (input[i])
            {
            case 0xA4:
                output += "ae";
                break; // ä
            case 0xB6:
                output += "oe";
                break; // ö
            case 0xBC:
                output += "ue";
                break; // ü
            case 0x84:
                output += "Ae";
                break; // Ä
            case 0x96:
                output += "Oe";
                break; // Ö
            case 0xDC:
                output += "Ue";
                break; // Ü
            case 0xA9:
                output += "e";
                break; // é
            case 0xA8:
                output += "e";
                break; // è
            case 0xA0:
                output += "a";
                break; // à
            default:
                output += "?";
                break;
            }
        }
        else
        {
            output += c;
        }
    }
    return output;
}

// --- DRAWING LOGIC ---
void drawDepartures(JsonDocument &doc)
{
    display.clearBuffer();

    // 1. Draw Header (Red Bar)
    display.fillRect(0, 0, 300, 45, EINK_RED); // Leaves 100px empty on right
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(EINK_WHITE);
    display.setCursor(5, 35);
    display.println(utf8ToAscii(STATION_NAME));

    // 1.5 Draw "Last Updated" (Top Right White Area)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        // Use default tiny 5x7 font for the label
        display.setFont(NULL);
        display.setTextColor(EINK_BLACK);
        display.setCursor(310, 12);
        display.print("Last Update:");

        // Use Bold 9pt font for the actual time
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(310, 35);

        char timeStr[6]; // Buffer for "HH:MM"
        sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        display.print(timeStr);
        display.fillRect(300, 42, 100, 3, EINK_RED); // Leaves 100px empty on right
    }

    // 2. Draw Connections
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(EINK_BLACK);

    JsonArray board = doc["stationboard"];
    int yPos = 75;

    if (board.size() == 0)
    {
        display.setCursor(5, yPos);
        display.println("No Data / API Error");
        return;
    }

    for (JsonObject conn : board)
    {
        if (yPos > 290)
            break;

        // Extract Data
        String timeStr = String((const char *)conn["stop"]["departure"]).substring(11, 16);
        int delay = conn["stop"]["delay"];
        const char *cat = conn["category"];
        const char *num = conn["number"];
        const char *to = conn["to"];

        // A. Time
        display.setCursor(5, yPos);
        display.setTextColor(EINK_BLACK);
        display.print(timeStr);

        // B. Delay (Red if late)
        if (delay > 0)
        {
            display.setTextColor(EINK_RED);
            display.print("+");
            display.print(delay);
            display.print("'");
        }

        // C. Line (e.g. S12)
        display.setTextColor(EINK_BLACK);
        display.setCursor(100, yPos);
        display.print(cat);
        display.print(num);

        // D. Destination (Truncated & Sanitized)
        String dest = utf8ToAscii(String(to));
        if (dest.length() > MAX_DEST_LEN)
        {
            dest = dest.substring(0, MAX_DEST_LEN);
        }
        display.setCursor(160, yPos);
        display.print(dest);

        yPos += 35;
    }
}

// --- API FETCHING ---
void fetchSBB()
{
    Serial.println("Fetching SBB Data...");

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi Disconnected! Skipping.");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String q = STATION_NAME;
    q.replace(" ", "%20");
    String fullURL = String(SBB_URL_BASE) + "?station=" + q + "&limit=" + String(FETCH_LIMIT);

    if (http.begin(client, fullURL))
    {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error)
            {
                Serial.println("Data Received. Updating Screen...");
                drawDepartures(doc);
                display.display();
                Serial.println("Done.");
            }
            else
            {
                Serial.print("JSON Error: ");
                Serial.println(error.c_str());
            }
        }
        else
        {
            Serial.printf("HTTP Error: %d\n", httpCode);
        }
        http.end();
    }
    else
    {
        Serial.println("Connection Failed.");
    }
}

// --- SETUP ---
void setup()
{
    Serial.begin(115200);

    // Init Hardware
    display.begin();
    pinMode(PIN_TOUCH, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_TOUCH), onButton, RISING);

    // Connect WiFi
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");

    // --- TIME SYNC (REQUIRED FOR TIMESTAMP) ---
    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", TIMEZONE_STR, 1);
    tzset();

    // First Update
    fetchSBB();
    lastUpdate = millis();
}

// --- LOOP ---
void loop()
{
    if (buttonPressed)
    {
        delay(200);
        buttonPressed = false;
        Serial.println("Button Pressed -> Forcing Update");
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