#ifndef BLEHANDLER_H
#define BLEHANDLER_H

#include <Arduino.h>

class BleHandler {
public:
    void begin();
    void update(); // Call in loop to handle save/reboot
    void saveAndReboot(); // Explicitly save and restart
    void stop();

private:
    static void onConnect(bool success);
};

#endif
