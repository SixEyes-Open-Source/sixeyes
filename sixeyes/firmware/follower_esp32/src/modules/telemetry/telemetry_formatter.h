#pragma once
#include <Arduino.h>

class TelemetryFormatter {
public:
    static TelemetryFormatter &instance();
    void init();
    // Format a small telemetry JSON payload into the provided buffer.
    // Returns number of bytes written (not including null terminator).
    size_t formatBasicTelemetry(char *buf, size_t buf_len);
private:
    TelemetryFormatter();
};
