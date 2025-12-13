#ifndef WATCHFACE_LOGIC_H
#define WATCHFACE_LOGIC_H

#include <Arduino.h>
#include <time.h>
#include "WeAct_EInk.h"

// Fonts
#include <Fonts/FreeSansBold18pt7b.h>

// Reference the global display object
extern WeAct42_Driver display;

void drawClockFace(bool isNewMinute)
{
    struct tm t;
    if (!getLocalTime(&t))
        return;

    // Only clear background on new minute to prepare for clean refresh
    if (isNewMinute)
    {
        display.clearBuffer();
    }

    int cx = 200;
    int cy = 150;

    // --- STATIC ELEMENTS (Draw only if new minute) ---
    if (isNewMinute)
    {
        // Red Ticks
        int r_ticks = 120;
        for (int i = 0; i < 12; i++)
        {
            float a = (i * 30 * 3.14159 / 180.0) - (3.14159 / 2);
            int x1 = cx + (r_ticks - 10) * cos(a);
            int y1 = cy + (r_ticks - 10) * sin(a);
            int x2 = cx + r_ticks * cos(a);
            int y2 = cy + r_ticks * sin(a);
            display.drawLine(x1, y1, x2, y2, EINK_RED);
            display.drawLine(x1 + 1, y1 + 1, x2 + 1, y2 + 1, EINK_RED);
        }

        // Inner Hour Dots
        int r_hours = 75;

        for (int i = 0; i < 12; i++)
        {
            float a = (i * 30 * 3.14159 / 180.0) - (3.14159 / 2);
            display.drawCircle(cx + r_hours * cos(a), cy + r_hours * sin(a), 4, EINK_BLACK);
        }
        for (int i = 0; i < t.tm_hour % 12; i++)
        {
            float a = (i * 30 * 3.14159 / 180.0) - (3.14159 / 2);
            display.fillCircle(cx + r_hours * cos(a), cy + r_hours * sin(a), 4, EINK_BLACK);
        }

        // Minute Ring (Outlines + Filled)
        int r_minutes = 100;
        for (int m = 0; m < 60; m++)
        {
            float a = ((m / 60.0) * 2 * 3.14159) - (3.14159 / 2);
            display.drawCircle(cx + r_minutes * cos(a), cy + r_minutes * sin(a), 3, EINK_BLACK);
        }
        for (int m = 0; m <= t.tm_min; m++)
        {
            float a = ((m / 60.0) * 2 * 3.14159) - (3.14159 / 2);
            display.fillCircle(cx + r_minutes * cos(a), cy + r_minutes * sin(a), 3, EINK_BLACK);
        }

        // Digital Time
        display.setTextColor(EINK_BLACK);
        display.setFont(&FreeSansBold18pt7b);
        char timeStr[6];
        sprintf(timeStr, "%02d:%02d", t.tm_hour, t.tm_min);
        int16_t x, y;
        uint16_t w, h;
        display.getTextBounds(timeStr, 0, 0, &x, &y, &w, &h);
        display.fillRect(cx - w / 2 - 10, cy - h / 2 - 10, w + 20, h + 20, EINK_WHITE);
        display.setCursor(cx - w / 2 - x, cy - h / 2 - y);
        display.print(timeStr);

        // Empty Bar Container
        display.drawRect(0, 290, 400, 10, EINK_BLACK);
    }

    // --- DYNAMIC ELEMENT (The 5s Bar) ---
    // Drawn on top of existing buffer every 5 seconds
    int barWidth = (t.tm_sec / 5) * (400 / 12);
    if (barWidth > 0)
    {
        display.fillRect(0, 290, barWidth, 10, EINK_BLACK);
    }
}

#endif