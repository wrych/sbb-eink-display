#ifndef CONFIG_GUI_H
#define CONFIG_GUI_H

#include <Arduino.h>
#include "WeAct_EInk.h"
#include "Settings.h"
#include "DisplayUtils.h"

// Fonts
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

// Reference the global display object defined in main.cpp
extern WeAct42_Driver display;

void drawConfigLine(const char *Label, const char *hint, const String &Value, int yPos)
{
    display.setFont(&FreeMono9pt7b);
    display.setTextColor(EINK_RED);
    display.setCursor(2, yPos);
    display.print(hint);
    display.setTextColor(EINK_BLACK);
    display.print(Label);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(150, yPos);
    display.println(Value);
}

void drawConfigScreen()
{
    display.clearBuffer();

    // Header
    display.fillRect(0, 0, 400, 45, EINK_RED);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(EINK_WHITE);
    display.setCursor(10, 35);
    display.println("CONFIG MODE (BLE)");

    display.setTextColor(EINK_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    int y = 80;
    int step = 25;

    drawConfigLine("WIFI SSID", "7671", WIFI_SSID, y);
    y += step;
    drawConfigLine("PASS", "7672", "***", y); // Masked
    y += step;
    drawConfigLine("STATION", "7673", utf8ToAscii(STATION_NAME), y);
    y += step;
    drawConfigLine("REFRESH", "7674", String(REFRESH_MS / 60000) + " min(s)", y);
    y += step;
    drawConfigLine("GUEST QR", "7676", WLAN_QR_ENABLED ? "ENABLED" : "DISABLED", y);
    y += step * 1;
    display.setCursor(2, y);
    display.setTextColor(EINK_BLACK);
    display.setFont(&FreeMono9pt7b);
    display.print("Bluetooth '");
    display.setTextColor(EINK_RED);
    display.setFont(&FreeMonoBold9pt7b);
    display.print("SBB_Display_Config");
    display.setTextColor(EINK_BLACK);
    display.setFont(&FreeMono9pt7b);
    display.print("'");
    y += step;
    display.setCursor(2, y);
    display.setTextColor(EINK_BLACK);
    display.setFont(&FreeMono9pt7b);
    display.print("'");
    display.setTextColor(EINK_RED);
    display.setFont(&FreeMonoBold9pt7b);
    display.print("SAVE");
    display.setTextColor(EINK_BLACK);
    display.setFont(&FreeMono9pt7b);
    display.print("' to ...");
    display.setTextColor(EINK_RED);
    display.setFont(&FreeMonoBold9pt7b);
    display.print("7675");
    y += step * 2;
    display.setCursor(2, y);
    display.setTextColor(EINK_BLACK);
    display.setFont(&FreeMono9pt7b);
    display.print("Thanks for using ");
    display.setTextColor(EINK_RED);
    display.setFont(&FreeMonoBold9pt7b);
    display.print("SBB E-Ink Display!");
}

#endif
