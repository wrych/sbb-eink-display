#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Settings.h"

enum LedState {
    LED_OFF,
    LED_RUNNING,    // Green Solid
    LED_UPDATING,   // Blue/Green (Cyan) Breathing
    LED_CONFIG      // Orange Breathing
};

class LedManager {
public:
    LedManager(int pin, int numPixels = 1);
    void begin();
    void loop();
    void setState(LedState state);

private:
    Adafruit_NeoPixel _pixels;
    LedState _currentState;
    
    // Animation helpers
    uint32_t _targetColor;
    TaskHandle_t _taskHandle;
    
    static void taskFunction(void* parameter);
    void setBrightnessColor(uint8_t r, uint8_t g, uint8_t b, float brightness);
};

extern LedManager statusLed;

#endif
