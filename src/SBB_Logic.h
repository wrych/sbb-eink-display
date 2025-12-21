#ifndef SBB_LOGIC_H
#define SBB_LOGIC_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "Settings.h"
#include "WeAct_EInk.h"
#include "LedManager.h"

// Fonts
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

#include "WeatherUtils.h"
#include "DisplayUtils.h"

const char *SBB_URL_BASE = "https://transport.opendata.ch/v1/stationboard";

void drawDepartures(JsonDocument &doc)
{
    display.clearBuffer();

    // Header
    display.fillRect(0, 0, 300, 45, EINK_RED);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(EINK_WHITE);
    display.setCursor(5, 35);
    display.println(utf8ToAscii(STATION_NAME));

    // Weather Fetch & Header Info
    double lat = doc["station"]["coordinate"]["x"]; // SBB API x is lat
    double lon = doc["station"]["coordinate"]["y"]; // SBB API y is lon
    WeatherData weather = fetchWeather(lat, lon);

    if (weather.valid)
    {
        drawWeatherSymbol(325, 22, weather.code);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(EINK_BLACK);
        display.setCursor(345, 30);
        display.print(String(weather.temp, 1));
        display.print("C");
    }

    // Connections
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(EINK_BLACK);
    JsonArray board = doc["stationboard"];
    int yPos = 75;        // Slightly higher start
    int lineSpacing = 34; // Reduced from 35

    if (board.size() == 0)
    {
        display.setCursor(5, yPos);
        display.println("No Data / API Error");
    }
    else
    {
        for (JsonObject conn : board)
        {
            String timeStr = String((const char *)conn["stop"]["departure"]).substring(11, 16);
            int delay = conn["stop"]["delay"];
            const char *cat = conn["category"];
            const char *num = conn["number"];
            const char *to = conn["to"];

            display.setCursor(5, yPos);
            display.setTextColor(EINK_BLACK);
            display.print(timeStr);

            if (delay > 0)
            {
                display.setTextColor(EINK_RED);
                display.print("+");
                display.print(delay);
                display.print("'");
            }

            display.setTextColor(EINK_BLACK);
            display.setCursor(100, yPos);
            display.print(cat);
            display.print(num);

            String dest = utf8ToAscii(String(to));
            if (dest.length() > MAX_DEST_LEN)
                dest = dest.substring(0, MAX_DEST_LEN);
            display.setCursor(160, yPos);
            display.print(dest);

            yPos += lineSpacing;
        }
    }

    // Timestamp (Bottom Right)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        display.setFont(NULL); // Smallest font
        display.setTextColor(EINK_BLACK);
        char updateStr[30];
        sprintf(updateStr, "Last Update: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(updateStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(400 - w - 5, 300 - h - 2);
        display.print(updateStr);
    }
}

void fetchSBB()
{
    Serial.println("Fetching SBB...");
    if (WiFi.status() != WL_CONNECTED)
        return;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String q = STATION_NAME;
    q.replace(" ", "%20");
    String url = String(SBB_URL_BASE) + "?station=" + q + "&limit=" + String(FETCH_LIMIT);

    if (http.begin(client, url))
    {
        statusLed.setState(LED_UPDATING);
        if (http.GET() == HTTP_CODE_OK)
        {
            String payload = http.getString();
            JsonDocument doc;
            if (!deserializeJson(doc, payload))
            {
                drawDepartures(doc);
                display.display(); // Always clean refresh for Timetable
                Serial.println("Timetable Updated");
            }
        }
        http.end();
        statusLed.setState(LED_OFF);
    }
}

void drawQRCodePage()
{
    display.clearBuffer();

    // Header
    display.fillRect(0, 0, 400, 45, EINK_RED);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(EINK_WHITE);
    display.setCursor(10, 35);
    display.println("GUEST WLAN");

    if (!WLAN_QR_ENABLED || WLAN_QR_SIZE <= 0)
    {
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(EINK_BLACK);
        display.setCursor(10, 100);
        display.println("QR Code not configured.");
        return;
    }

    // Centering logic
    int scale = 6; // Adjust scale for visibility
    if (WLAN_QR_SIZE > 33)
        scale = 4;

    int qrTotalSize = WLAN_QR_SIZE * scale;
    int startX = (400 - qrTotalSize) / 2;
    int startY = 60 + (240 - qrTotalSize) / 2;

    for (int y = 0; y < WLAN_QR_SIZE; y++)
    {
        for (int x = 0; x < WLAN_QR_SIZE; x++)
        {
            int bitIdx = y * WLAN_QR_SIZE + x;
            int byteIdx = bitIdx / 8;
            int bitPos = 7 - (bitIdx % 8);

            if (byteIdx < 256 && (WLAN_QR_BITMAP[byteIdx] & (1 << bitPos)))
            {
                display.fillRect(startX + x * scale, startY + y * scale, scale, scale, EINK_BLACK);
            }
        }
    }

    // Info footer
    display.setFont(&FreeMono9pt7b);
    display.setTextColor(EINK_BLACK);
    display.setCursor(startX, startY + qrTotalSize + 25);
    display.print("SSID: ");
    display.println(WIFI_SSID);
}

#endif