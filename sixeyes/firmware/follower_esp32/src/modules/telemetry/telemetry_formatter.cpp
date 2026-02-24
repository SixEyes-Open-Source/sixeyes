#include "telemetry_formatter.h"
#include <Arduino.h>
#include "modules/config/board_config.h"

TelemetryFormatter &TelemetryFormatter::instance() {
    static TelemetryFormatter inst;
    return inst;
}

TelemetryFormatter::TelemetryFormatter() {}

void TelemetryFormatter::init() {
    Serial.println("TelemetryFormatter: init");
}

size_t TelemetryFormatter::formatBasicTelemetry(char *buf, size_t buf_len) {
    // Minimal JSON with uptime and control loop freq
    unsigned long ms = millis();
    int written = snprintf(buf, buf_len, "{\"uptime_ms\":%lu,\"hz\":%d}", ms, CONTROL_LOOP_HZ);
    if (written < 0) return 0;
    return (size_t)written;
}
