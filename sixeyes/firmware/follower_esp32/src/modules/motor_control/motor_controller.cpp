#include "motor_controller.h"
#include <Arduino.h>
#include <algorithm>

namespace {
constexpr float kDegreesPerStep =
    360.0f / (TMC2209_FULL_STEPS_PER_REV * TMC2209_MICROSTEPS);
}

MotorController &MotorController::instance() {
    static MotorController inst;
    return inst;
}

MotorController::MotorController() {
    current_positions_.fill(0.0f);
    current_velocities_.fill(0.0f);
    target_positions_.fill(0.0f);
    position_errors_.fill(0.0f);
    current_steps_.fill(0);
    target_steps_.fill(0);
    zero_offsets_steps_.fill(0);
}

void MotorController::init() {
    setupStepPins();
    Serial.println("MotorController: init open-loop step executor (no encoder feedback)");
    Serial.print("  - Deg/step: ");
    Serial.println(kDegreesPerStep, 6);
    Serial.print("  - Max motor speed (deg/s): ");
    Serial.println(MAX_MOTOR_SPEED_DEG_PER_S, 1);
}

void MotorController::setAbsoluteTargets(const std::array<float, NUM_STEPPERS> &targets) {
    for (int i = 0; i < NUM_STEPPERS; ++i) {
        target_positions_[i] = targets[i];
        target_steps_[i] = degreesToSteps(target_positions_[i]);
        position_errors_[i] = target_positions_[i] - current_positions_[i];
    }
}

float MotorController::stepsToDegrees(int32_t steps) {
    return static_cast<float>(steps) * kDegreesPerStep;
}

int32_t MotorController::degreesToSteps(float degrees) {
    const float steps_f = degrees / kDegreesPerStep;
    return static_cast<int32_t>(lroundf(steps_f));
}

void MotorController::setupStepPins() {
    for (size_t i = 0; i < NUM_STEPPERS; ++i) {
        pinMode(TMC2209_STEP_PINS[i], OUTPUT);
        pinMode(TMC2209_DIR_PINS[i], OUTPUT);
        digitalWrite(TMC2209_STEP_PINS[i], LOW);
        digitalWrite(TMC2209_DIR_PINS[i], LOW);
    }
}

void MotorController::pulseStepPin(uint8_t motor_index) {
    digitalWrite(TMC2209_STEP_PINS[motor_index], HIGH);
    delayMicroseconds(STEP_PULSE_HIGH_US);
    digitalWrite(TMC2209_STEP_PINS[motor_index], LOW);
    delayMicroseconds(STEP_PULSE_LOW_US);
}

void MotorController::updateVelocitiesFromStepDelta(const std::array<int32_t, NUM_STEPPERS>& step_delta, float dt) {
    if (dt <= 0.0f) {
        current_velocities_.fill(0.0f);
        return;
    }
    for (size_t i = 0; i < NUM_STEPPERS; ++i) {
        current_velocities_[i] = stepsToDegrees(step_delta[i]) / dt;
    }
}

void MotorController::update() {
    static unsigned long last_update_us = 0;
    const unsigned long now_us = micros();
    float dt = 1.0f / static_cast<float>(CONTROL_LOOP_HZ);

    if (last_update_us != 0) {
        dt = static_cast<float>(now_us - last_update_us) / 1000000.0f;
        if (dt < 0.0005f) dt = 0.0005f;
        if (dt > 0.05f) dt = 1.0f / static_cast<float>(CONTROL_LOOP_HZ);
    }
    last_update_us = now_us;

    std::array<int32_t, NUM_STEPPERS> step_delta{};
    step_delta.fill(0);

    if (!enabled_) {
        current_velocities_.fill(0.0f);
        for (size_t i = 0; i < NUM_STEPPERS; ++i) {
            position_errors_[i] = target_positions_[i] - current_positions_[i];
        }
        return;
    }

    const float max_steps_f = (MAX_MOTOR_SPEED_DEG_PER_S / kDegreesPerStep) * dt;
    const int32_t max_steps_this_cycle = std::max<int32_t>(1, static_cast<int32_t>(floorf(max_steps_f)));

    for (size_t i = 0; i < NUM_STEPPERS; ++i) {
        const int32_t error_steps = target_steps_[i] - current_steps_[i];
        int32_t steps_to_move = std::min<int32_t>(std::abs(error_steps), max_steps_this_cycle);

        if (steps_to_move == 0) {
            continue;
        }

        const bool direction_positive = error_steps > 0;
        digitalWrite(TMC2209_DIR_PINS[i], direction_positive ? HIGH : LOW);

        for (int32_t s = 0; s < steps_to_move; ++s) {
            pulseStepPin(static_cast<uint8_t>(i));
        }

        const int32_t signed_step_move = direction_positive ? steps_to_move : -steps_to_move;
        current_steps_[i] += signed_step_move;
        step_delta[i] = signed_step_move;
    }

    updateVelocitiesFromStepDelta(step_delta, dt);

    for (size_t i = 0; i < NUM_STEPPERS; ++i) {
        current_positions_[i] = stepsToDegrees(current_steps_[i] - zero_offsets_steps_[i]);
        target_positions_[i] = stepsToDegrees(target_steps_[i] - zero_offsets_steps_[i]);
        position_errors_[i] = target_positions_[i] - current_positions_[i];
    }
}

void MotorController::enableMotors() {
    enabled_ = true;
    Serial.println("MotorController: motors enabled");
}

void MotorController::disableMotors() {
    enabled_ = false;
    current_velocities_.fill(0.0f);
    Serial.println("MotorController: motors disabled");
}

bool MotorController::motorsEnabled() const { 
    return enabled_; 
}

void MotorController::setCurrentPositionAsZero() {
    for (size_t i = 0; i < NUM_STEPPERS; ++i) {
        zero_offsets_steps_[i] = current_steps_[i];
        target_steps_[i] = current_steps_[i];
        current_positions_[i] = 0.0f;
        target_positions_[i] = 0.0f;
        position_errors_[i] = 0.0f;
    }
    Serial.println("MotorController: software zero reference set to current position");
}

std::array<float, NUM_STEPPERS> MotorController::getCurrentPositions() const {
    return current_positions_;
}

std::array<float, NUM_STEPPERS> MotorController::getCurrentVelocities() const {
    return current_velocities_;
}

std::array<float, NUM_STEPPERS> MotorController::getErrors() const {
    return position_errors_;
}
