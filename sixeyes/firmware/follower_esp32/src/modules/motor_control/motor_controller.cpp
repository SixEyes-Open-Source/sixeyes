#include "motor_controller.h"
#include <Arduino.h>
#include <algorithm>

MotorController &MotorController::instance() {
    static MotorController inst;
    return inst;
}

MotorController::MotorController() {
    // Initialize arrays
    current_positions_.fill(0.0f);
    current_velocities_.fill(0.0f);
    target_positions_.fill(0.0f);
    interpolated_targets_.fill(0.0f);
    last_target_positions_.fill(0.0f);
    position_errors_.fill(0.0f);
    error_integral_.fill(0.0f);
    last_errors_.fill(0.0f);
}

void MotorController::init() {
    Serial.println("MotorController: init with closed-loop control and interpolation");
    Serial.println("  - PID gains: Kp=2.0, Ki=0.05, Kd=0.1");
    Serial.println("  - Max velocity: 45.0 deg/sec");
    Serial.println("  - Interpolation: 1.0 sec S-curve");
    Serial.println("  - Update frequency: controlled by MotorControlScheduler");
}

void MotorController::setAbsoluteTargets(const std::array<float, NUM_STEPPERS> &targets) {
    // Check if targets have changed significantly
    bool targets_changed = false;
    for (int i = 0; i < NUM_STEPPERS; ++i) {
        if (std::abs(targets[i] - target_positions_[i]) > POSITION_TOLERANCE) {
            targets_changed = true;
            break;
        }
    }
    
    if (targets_changed) {
        target_positions_ = targets;
        last_target_positions_ = current_positions_;  // Start interpolation from current position
        interpolation_timer_ = 0.0f;
        is_interpolating_ = true;
        
        Serial.println("MotorController: new targets received, starting interpolation");
    }
}

void MotorController::updateInterpolation(float dt) {
    if (!is_interpolating_) {
        interpolated_targets_ = target_positions_;
        return;
    }
    
    interpolation_timer_ += dt;
    float progress = std::min(1.0f, interpolation_timer_ / INTERPOLATION_TIME_S);
    
    // S-curve interpolation for smooth acceleration/deceleration
    // Using ease-in-out cubic: t^2 * (3 - 2*t)
    float t = progress;
    float smoothed = t * t * (3.0f - 2.0f * t);
    
    for (int i = 0; i < NUM_STEPPERS; ++i) {
        interpolated_targets_[i] = 
            last_target_positions_[i] + 
            (target_positions_[i] - last_target_positions_[i]) * smoothed;
    }
    
    // Check if interpolation is complete
    if (progress >= 1.0f) {
        is_interpolating_ = false;
        interpolated_targets_ = target_positions_;
        Serial.println("MotorController: interpolation complete");
    }
}

void MotorController::updateVelocities(float dt) {
    if (dt <= 0.0f) return;
    
    for (int i = 0; i < NUM_STEPPERS; ++i) {
        float delta_pos = current_positions_[i] - last_errors_[i];  // Use last position for derivative
        current_velocities_[i] = delta_pos / dt;
        
        // Clamp velocity to max
        current_velocities_[i] = std::max(-MAX_VELOCITY, 
                                          std::min(MAX_VELOCITY, current_velocities_[i]));
    }
}

float MotorController::calculatePIDOutput(uint8_t motor_index, float error) {
    if (motor_index >= NUM_STEPPERS) return 0.0f;
    
    // Proportional term
    float p_term = KP * error;
    
    // Integral term (with anti-windup)
    error_integral_[motor_index] += error * (1.0f / CONTROL_LOOP_HZ);
    error_integral_[motor_index] = std::max(-10.0f, std::min(10.0f, error_integral_[motor_index]));
    float i_term = KI * error_integral_[motor_index];
    
    // Derivative term
    float d_error = error - last_errors_[motor_index];
    float d_term = KD * d_error * CONTROL_LOOP_HZ;  // Scale by loop rate
    
    last_errors_[motor_index] = error;
    
    // Combine and saturate output
    float output = p_term + i_term + d_term;
    output = std::max(-100.0f, std::min(100.0f, output));  // Output limits (0-100% PWM equivalent)
    
    return output;
}

void MotorController::update() {
    if (!enabled_) {
        current_velocities_.fill(0.0f);
        return;
    }
    
    // Calculate time delta (assuming called at CONTROL_LOOP_HZ)
    static unsigned long last_update_ms = 0;
    unsigned long now_ms = millis();
    float dt = (now_ms - last_update_ms) / 1000.0f;
    last_update_ms = now_ms;
    
    // Clamp dt to reasonable range (prevent overflow on first call)
    if (dt > 0.1f) dt = 0.01f;  // Max 10ms expected at 100Hz, floor at 1ms
    if (dt < 0.001f) dt = 0.001f;
    
    // Update interpolation trajectory
    updateInterpolation(dt);
    
    // Closed-loop control: compute position error and PID output
    for (int i = 0; i < NUM_STEPPERS; ++i) {
        // Error is difference between interpolated target and current position
        position_errors_[i] = interpolated_targets_[i] - current_positions_[i];
        
        // Calculate PID control output
        float pid_output = calculatePIDOutput(i, position_errors_[i]);
        
        // Update position based on control signal
        // For stepper motors, pid_output represents desired velocity in degrees/sec
        float velocity_cmd = pid_output * (MAX_VELOCITY / 100.0f);  // Scale from 0-100 to velocity range
        
        // Integrate velocity to position
        current_positions_[i] += velocity_cmd * dt;
    }
    
    // Update velocity tracking
    updateVelocities(dt);
}

void MotorController::enableMotors() {
    enabled_ = true;
    // Reset integrator state when enabling
    error_integral_.fill(0.0f);
    last_errors_.fill(0.0f);
    Serial.println("MotorController: motors enabled");
}

void MotorController::disableMotors() {
    enabled_ = false;
    current_velocities_.fill(0.0f);
    error_integral_.fill(0.0f);
    Serial.println("MotorController: motors disabled");
}

bool MotorController::motorsEnabled() const { 
    return enabled_; 
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
