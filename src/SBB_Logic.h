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

#include "DisplayUtils.h"

// Reference the global display object defined in main.cpp
extern WeAct42_Driver display;

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

    // Timestamp
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        display.setFont(NULL);
        display.setTextColor(EINK_BLACK);
        display.setCursor(310, 12);
        display.print("Last Update:");
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(310, 35);
        char timeStr[6];
        sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        display.print(timeStr);
        display.fillRect(300, 42, 100, 3, EINK_RED);
    }

    // Connections
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

        yPos += 35;
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
        statusLed.setState(LED_RUNNING);
    }
}


#endif