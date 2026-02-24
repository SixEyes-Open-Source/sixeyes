#include "motor_controller.h"
#include <Arduino.h>

MotorController &MotorController::instance() {
    static MotorController inst;
    return inst;
}

MotorController::MotorController() {}

void MotorController::init() {
    Serial.println("MotorController: init");
    // TODO: initialize timers and deterministic control resources
}

void MotorController::setAbsoluteTargets(const std::array<float, NUM_STEPPERS> &targets) {
    target_positions_ = targets;
}

void MotorController::update() {
    // Minimal stub: copy target to current
    if (enabled_) {
        current_positions_ = target_positions_;
    }
}

void MotorController::enableMotors() {
    enabled_ = true;
    Serial.println("MotorController: motors enabled");
}

void MotorController::disableMotors() {
    enabled_ = false;
    Serial.println("MotorController: motors disabled");
}

bool MotorController::motorsEnabled() const { return enabled_; }
