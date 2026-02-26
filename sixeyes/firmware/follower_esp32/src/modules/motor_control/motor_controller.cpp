#include "motor_controller.h"
#include <Arduino.h>
#include <algorithm>
#include "modules/drivers/tmc2209/tmc2209_driver.h"

namespace {
constexpr float kDegreesPerStep =
    360.0f / (TMC2209_FULL_STEPS_PER_REV * TMC2209_MICROSTEPS);
constexpr int8_t kHomingDirectionSign[NUM_STEPPERS] = {
    -1, // Base
    -1, // Shoulder A
     1, // Shoulder B (mirrored)
    -1  // Elbow
};
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
    homing_latched_zero_steps_.fill(0);
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
    if (homing_phase_ != HomingPhase::IDLE && homing_phase_ != HomingPhase::COMPLETE) {
        return;
    }
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

bool MotorController::isMotorSelectedForHoming(uint8_t motor_index) const {
    return ((homing_mask_ >> motor_index) & 0x01) != 0;
}

int32_t MotorController::maxStepsForSpeed(float speed_deg_per_s, float dt) const {
    const float max_steps_f = (speed_deg_per_s / kDegreesPerStep) * dt;
    return std::max<int32_t>(1, static_cast<int32_t>(floorf(max_steps_f)));
}

int32_t MotorController::doSingleMotorStep(uint8_t motor_index, bool direction_positive, int32_t max_steps_this_cycle) {
    if (max_steps_this_cycle <= 0) return 0;

    digitalWrite(TMC2209_DIR_PINS[motor_index], direction_positive ? HIGH : LOW);
    for (int32_t step = 0; step < max_steps_this_cycle; ++step) {
        pulseStepPin(motor_index);
    }

    const int32_t signed_move = direction_positive ? max_steps_this_cycle : -max_steps_this_cycle;
    current_steps_[motor_index] += signed_move;
    return signed_move;
}

bool MotorController::advanceToNextHomingMotor() {
    for (uint8_t index = homing_motor_index_; index < NUM_STEPPERS; ++index) {
        if (!isMotorSelectedForHoming(index)) {
            continue;
        }

        homing_motor_index_ = index;
        homing_seek_steps_done_ = 0;
        homing_phase_steps_done_ = 0;
        homing_stall_debounce_count_ = 0;
        homing_phase_ = HomingPhase::SEEK_STALL;

        TMC2209Driver::instance().enableStallGuard(index, homing_sensitivity_);
        Serial.print("MotorController: homing motor ");
        Serial.println(index);
        return true;
    }

    completeHomingSuccess();
    return false;
}

void MotorController::completeHomingSuccess() {
    for (size_t i = 0; i < NUM_STEPPERS; ++i) {
        if (isMotorSelectedForHoming(static_cast<uint8_t>(i))) {
            TMC2209Driver::instance().disableStallGuard(static_cast<uint8_t>(i));
            zero_offsets_steps_[i] = homing_latched_zero_steps_[i];
            target_steps_[i] = current_steps_[i];
        }
    }

    homing_phase_ = HomingPhase::COMPLETE;
    last_homing_success_ = true;
    Serial.println("MotorController: StallGuard homing completed");
}

void MotorController::failHoming(const char* reason) {
    if (homing_motor_index_ < NUM_STEPPERS) {
        TMC2209Driver::instance().disableStallGuard(homing_motor_index_);
    }
    homing_phase_ = HomingPhase::FAILED;
    last_homing_success_ = false;
    Serial.print("MotorController: StallGuard homing failed: ");
    Serial.println(reason);
}

void MotorController::runHomingStep(float dt) {
    if (homing_phase_ == HomingPhase::COMPLETE || homing_phase_ == HomingPhase::FAILED) {
        homing_phase_ = HomingPhase::IDLE;
        return;
    }

    if (homing_motor_index_ >= NUM_STEPPERS) {
        failHoming("invalid motor index");
        return;
    }

    const int8_t sign = kHomingDirectionSign[homing_motor_index_];
    const bool seek_positive = sign > 0;

    if (homing_phase_ == HomingPhase::SEEK_STALL) {
        const int32_t steps = maxStepsForSpeed(HOMING_SEEK_SPEED_DEG_PER_S, dt);
        homing_seek_steps_done_ += std::abs(doSingleMotorStep(homing_motor_index_, seek_positive, steps));

        if (TMC2209Driver::instance().isStalled(homing_motor_index_)) {
            homing_stall_debounce_count_++;
        } else {
            homing_stall_debounce_count_ = 0;
        }

        if (homing_stall_debounce_count_ >= HOMING_STALL_DEBOUNCE_CYCLES) {
            homing_phase_ = HomingPhase::BACKOFF;
            homing_phase_steps_done_ = 0;
            homing_stall_debounce_count_ = 0;
            Serial.print("MotorController: stall detected, backoff motor ");
            Serial.println(homing_motor_index_);
            return;
        }

        if (homing_seek_steps_done_ >= HOMING_SEEK_MAX_STEPS) {
            failHoming("seek max steps exceeded");
            return;
        }
    }

    if (homing_phase_ == HomingPhase::BACKOFF) {
        const int32_t steps = maxStepsForSpeed(HOMING_SEEK_SPEED_DEG_PER_S, dt);
        const int32_t remaining = HOMING_BACKOFF_STEPS - homing_phase_steps_done_;
        const int32_t to_move = std::min(steps, remaining);
        homing_phase_steps_done_ += std::abs(doSingleMotorStep(homing_motor_index_, !seek_positive, to_move));

        if (homing_phase_steps_done_ >= HOMING_BACKOFF_STEPS) {
            homing_phase_ = HomingPhase::APPROACH;
            homing_phase_steps_done_ = 0;
            Serial.print("MotorController: re-approach motor ");
            Serial.println(homing_motor_index_);
            return;
        }
    }

    if (homing_phase_ == HomingPhase::APPROACH) {
        const int32_t steps = maxStepsForSpeed(HOMING_APPROACH_SPEED_DEG_PER_S, dt);
        homing_phase_steps_done_ += std::abs(doSingleMotorStep(homing_motor_index_, seek_positive, steps));

        if (TMC2209Driver::instance().isStalled(homing_motor_index_)) {
            homing_stall_debounce_count_++;
        } else {
            homing_stall_debounce_count_ = 0;
        }

        if (homing_stall_debounce_count_ >= HOMING_STALL_DEBOUNCE_CYCLES) {
            homing_latched_zero_steps_[homing_motor_index_] = current_steps_[homing_motor_index_];
            TMC2209Driver::instance().disableStallGuard(homing_motor_index_);

            homing_motor_index_++;
            advanceToNextHomingMotor();
            return;
        }

        if (homing_phase_steps_done_ >= HOMING_APPROACH_MAX_STEPS) {
            failHoming("approach max steps exceeded");
            return;
        }
    }
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

    if (homing_phase_ != HomingPhase::IDLE) {
        runHomingStep(dt);
        for (size_t i = 0; i < NUM_STEPPERS; ++i) {
            current_positions_[i] = stepsToDegrees(current_steps_[i] - zero_offsets_steps_[i]);
            target_positions_[i] = stepsToDegrees(target_steps_[i] - zero_offsets_steps_[i]);
            position_errors_[i] = target_positions_[i] - current_positions_[i];
        }
        current_velocities_.fill(0.0f);
        return;
    }

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

bool MotorController::startStallGuardHoming(uint8_t motor_mask, uint8_t sensitivity) {
    if (isHoming()) {
        Serial.println("MotorController: homing already active");
        return false;
    }

    if (motor_mask == 0) {
        Serial.println("MotorController: homing rejected (empty motor mask)");
        return false;
    }

    if (!enabled_) {
        enableMotors();
    }

    homing_mask_ = static_cast<uint8_t>(motor_mask & 0x0F);
    homing_sensitivity_ = sensitivity;
    homing_motor_index_ = 0;
    homing_seek_steps_done_ = 0;
    homing_phase_steps_done_ = 0;
    homing_stall_debounce_count_ = 0;
    last_homing_success_ = false;

    Serial.print("MotorController: starting StallGuard homing, mask=0x");
    Serial.print(homing_mask_, HEX);
    Serial.print(" sensitivity=");
    Serial.println(homing_sensitivity_);

    return advanceToNextHomingMotor();
}

bool MotorController::isHoming() const {
    return homing_phase_ != HomingPhase::IDLE;
}

bool MotorController::lastHomingSucceeded() const {
    return last_homing_success_;
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
