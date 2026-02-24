#include "gpio.h"
#include <Arduino.h>

// Default EN pin; override in board_config or board-specific header if needed
static const uint8_t EN_PIN = 5;

void HAL_GPIO::init() {
    pinMode(EN_PIN, OUTPUT);
    // Disable motors by default
    digitalWrite(EN_PIN, LOW);
    Serial.println("HAL_GPIO: initialized, motors disabled (EN low)");
}

void HAL_GPIO::setMotorEnable(bool enabled) {
    digitalWrite(EN_PIN, enabled ? HIGH : LOW);
}

bool HAL_GPIO::readPin(uint8_t pin) {
    return digitalRead(pin) == HIGH;
}
