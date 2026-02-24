#include "usb_cdc.h"
#include <Arduino.h>

UsbCDC &UsbCDC::instance() {
    static UsbCDC inst;
    return inst;
}

UsbCDC::UsbCDC() {}

void UsbCDC::init() {
    // On ESP32-S3 USB-CDC setup may require TinyUSB or core support.
    Serial.println("UsbCDC: init (host CDC placeholder)");
}

void UsbCDC::sendTelemetry(const char *msg) {
    Serial.println(msg);
}
