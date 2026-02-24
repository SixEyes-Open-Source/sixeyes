#pragma once
#include <Arduino.h>

class UsbCDC {
public:
    static UsbCDC &instance();
    void init();
    void sendTelemetry(const char *msg);
private:
    UsbCDC();
};
