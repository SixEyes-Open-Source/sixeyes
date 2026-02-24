#include "servo_manager.h"
#include <Arduino.h>

ServoManager &ServoManager::instance() {
    static ServoManager inst;
    return inst;
}

ServoManager::ServoManager() {}

void ServoManager::init() {
    Serial.println("ServoManager: init");
}

void ServoManager::setPositions(const std::array<float, NUM_SERVOS> &positions) {
    positions_ = positions;
    if (enabled_) {
        Serial.println("ServoManager: apply positions");
    }
}

void ServoManager::enable() {
    enabled_ = true;
    Serial.println("ServoManager: enabled");
}

void ServoManager::disable() {
    enabled_ = false;
    Serial.println("ServoManager: disabled");
}

bool ServoManager::enabled() const { return enabled_; }
