#pragma once
#include <Arduino.h>

namespace Logging {
    inline void info(const char *msg) { Serial.println(msg); }
    inline void infof(const char *fmt, unsigned long v) { char buf[128]; snprintf(buf, sizeof(buf), fmt, v); Serial.println(buf); }
}
