#ifndef WEATHER_UTILS_H
#define WEATHER_UTILS_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WeAct_EInk.h"

extern WeAct42_Driver display;

struct WeatherData
{
    float temp;
    int code;
    bool valid;
};

inline void drawWeatherSymbol(int x, int y, int code)
{
    // Basic symbols drawn using primitives
    if (code == 0)
    {                                          // Clear sky
        display.fillCircle(x, y, 8, EINK_RED); // Red sun
        for (int i = 0; i < 8; i++)
        {
            float a = i * 45 * PI / 180.0;
            display.drawLine(x + 10 * cos(a), y + 10 * sin(a), x + 14 * cos(a), y + 14 * sin(a), EINK_RED);
        }
    }
    else if (code <= 3)
    {                                                  // Partly cloudy
        display.fillCircle(x - 8, y - 3, 8, EINK_RED); // Red sun
        display.drawCircle(x - 4, y + 2, 6, EINK_BLACK);
        display.drawCircle(x + 4, y + 2, 6, EINK_BLACK);
        display.drawCircle(x, y - 2, 6, EINK_BLACK);
        display.fillCircle(x - 4, y + 2, 4, EINK_WHITE);
        display.fillCircle(x, y - 2, 4, EINK_WHITE);
        display.fillCircle(x + 4, y + 2, 4, EINK_WHITE);
    }
    else if (code >= 51 && code <= 67 || code >= 80 && code <= 82)
    { // Rain
        display.fillCircle(x - 4, y + 2, 6, EINK_BLACK);
        display.fillCircle(x + 4, y + 2, 6, EINK_BLACK);
        display.fillCircle(x, y - 2, 6, EINK_BLACK);
        display.drawLine(x - 4, y + 6, x - 8, y + 14, EINK_RED);
        display.drawLine(x, y + 6, x - 4, y + 14, EINK_RED);
        display.drawLine(x + 4, y + 6, x + 0, y + 14, EINK_RED);
    }
    else if (code >= 71 && code <= 77 || code >= 85 && code <= 86)
    { // Snow
        display.drawCircle(x - 4, y + 2, 6, EINK_BLACK);
        display.drawCircle(x + 4, y + 2, 6, EINK_BLACK);
        display.drawCircle(x, y - 2, 6, EINK_BLACK);
        display.fillCircle(x - 4, y + 2, 4, EINK_WHITE);
        display.fillCircle(x, y - 2, 4, EINK_WHITE);
        display.fillCircle(x + 4, y + 2, 4, EINK_WHITE);
        int x_off = -5;
        int y_off = 8;
        for (int i = 0; i < 6; i++)
        {
            float a = (i * 60) * PI / 180.0;
            int x1 = x + x_off + (int)(12 * cos(a));
            int y1 = y + y_off + (int)(12 * sin(a));
            display.drawLine(x + x_off, y + y_off, x1, y1, EINK_RED);

            // Side branches
            int bx = x + x_off + (int)(8 * cos(a));
            int by = y + y_off + (int)(8 * sin(a));

            float ba1 = a + PI / 3;
            float ba2 = a - PI / 3;

            display.drawLine(bx, by, bx + (int)(5 * cos(ba1)), by + (int)(5 * sin(ba1)), EINK_RED);
            display.drawLine(bx, by, bx + (int)(5 * cos(ba2)), by + (int)(5 * sin(ba2)), EINK_RED);
        }
    }
    else if (code == 45 || code == 48)
    {                                                  // Fog
        display.drawCircle(x - 8, y - 3, 8, EINK_RED); // Red sun
        display.fillRect(x - 8, y - 2, 20, 2, EINK_BLACK);
        display.fillRect(x - 12, y + 4, 20, 2, EINK_BLACK);
        display.fillRect(x - 10, y + 10, 20, 2, EINK_BLACK);
    }
    else
    { // Generic cloud for everything else
        display.fillCircle(x - 5, y + 3, 5, EINK_BLACK);
        display.fillCircle(x + 5, y + 3, 5, EINK_BLACK);
        display.fillCircle(x, y - 1, 7, EINK_BLACK);
    }
}

inline WeatherData fetchWeather(double lat, double lon)
{
    WeatherData data = {0, 0, false};
    if (WiFi.status() != WL_CONNECTED)
        return data;

    HTTPClient http;
    // Using Open-Meteo as a proxy for Swiss coordinates
    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
                 "&longitude=" + String(lon, 4) + "&current_weather=true";

    Serial.print("Fetching Weather for ");
    Serial.print(lat, 4);
    Serial.print(", ");
    Serial.println(lon, 4);

    if (http.begin(url))
    {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            JsonDocument doc;
            if (!deserializeJson(doc, payload))
            {
                data.temp = doc["current_weather"]["temperature"];
                data.code = doc["current_weather"]["weathercode"];
                data.valid = true;
                Serial.print("Weather: ");
                Serial.print(data.temp);
                Serial.print("C, Code: ");
                Serial.println(data.code);
            }
        }
        else
        {
            Serial.print("Weather HTTP Error: ");
            Serial.println(httpCode);
        }
        http.end();
    }
    return data;
}

#endif
