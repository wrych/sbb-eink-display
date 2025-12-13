#include "LedManager.h"

LedManager statusLed(PIN_RGB_LED, 1);

LedManager::LedManager(int pin, int numPixels) 
    : _pixels(numPixels, pin, NEO_GRB + NEO_KHZ800), _currentState(LED_OFF)
{
}

void LedManager::begin() {
    _pixels.begin();
    _pixels.clear();
    _pixels.show();
    _pixels.setBrightness(20); 

    // Create Task
    xTaskCreatePinnedToCore(
        LedManager::taskFunction,
        "LedTask",
        4096,
        this,
        1,
        &_taskHandle,
        0 // Core 0
    );
}

void LedManager::setState(LedState state) {
    _currentState = state;
}

void LedManager::taskFunction(void* parameter) {
    LedManager* lm = (LedManager*)parameter;
    
    while(true) {
        lm->loop();
        vTaskDelay(pdMS_TO_TICKS(20)); // Update every 20ms
    }
}

void LedManager::loop() {
    unsigned long t = millis();
    
    switch (_currentState) {
        case LED_OFF:
             // Only clear if not already cleared? 
             // Ideally we just keep showing clear. 
             // To avoid spamming show(), check state change or simple timer? 
             // NeoPixel.show() disables interrupts briefly. 20ms is 50Hz. acceptable.
             _pixels.clear();
             _pixels.show();
             break;

        case LED_RUNNING:
            // Solid Green
            _pixels.setPixelColor(0, _pixels.Color(0, 255, 0));
            _pixels.show();
            break;

        case LED_UPDATING:
        {
            // Alternating Blue/Green (Crossfade)
            // sin wave -1 to 1
            float val = sin(t / 1000.0); 
            // map to 0 to 1
            float pos = (val + 1.0) / 2.0; 

            // Linear crossfade
            uint8_t g = (uint8_t)(255 * (1.0 - pos));
            uint8_t b = (uint8_t)(255 * pos);
            
            _pixels.setPixelColor(0, _pixels.Color(0, g, b));
            _pixels.show();
            break;
        }

        case LED_CONFIG:
        {
            // Orange Breathing
            float b = (sin(t / 500.0) + 1.0) / 2.0; 
            b = 0.1 + (b * 0.9);

            uint8_t r = (uint8_t)(255 * b);
            uint8_t g = (uint8_t)(165 * b);
            
            _pixels.setPixelColor(0, _pixels.Color(r, g, 0));
            _pixels.show();
            break;
        }
    }
}

void LedManager::setBrightnessColor(uint8_t r, uint8_t g, uint8_t b, float brightness) {
    _pixels.setPixelColor(0, _pixels.Color((uint8_t)(r*brightness), (uint8_t)(g*brightness), (uint8_t)(b*brightness)));
    _pixels.show();
}
