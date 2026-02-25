#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include "modules/config/board_config.h"

class MotorController {
public:
    static MotorController &instance();
    void init();
    void setAbsoluteTargets(const std::array<float, NUM_STEPPERS> &targets);
    void update(); // called at CONTROL_LOOP_HZ
    void enableMotors();
    void disableMotors();
    bool motorsEnabled() const;
    
    // Accessors for diagnostics
    std::array<float, NUM_STEPPERS> getCurrentPositions() const;
    std::array<float, NUM_STEPPERS> getCurrentVelocities() const;
    std::array<float, NUM_STEPPERS> getErrors() const;

private:
    MotorController();
    
    // PID control constants (tunable per motor if needed)
    static constexpr float KP = 2.0f;      // Proportional gain
    static constexpr float KI = 0.05f;     // Integral gain
    static constexpr float KD = 0.1f;      // Derivative gain
    static constexpr float MAX_VELOCITY = 45.0f;  // degrees/sec max
    static constexpr float MAX_ACCELERATION = 90.0f; // degrees/sec^2 max
    static constexpr float INTERPOLATION_TIME_S = 1.0f;  // time to reach target (seconds)
    static constexpr float POSITION_TOLERANCE = 0.5f; // degrees
    
    // State tracking
    std::array<float, NUM_STEPPERS> current_positions_{};
    std::array<float, NUM_STEPPERS> current_velocities_{};
    std::array<float, NUM_STEPPERS> target_positions_{};
    std::array<float, NUM_STEPPERS> interpolated_targets_{};
    std::array<float, NUM_STEPPERS> last_target_positions_{};
    std::array<float, NUM_STEPPERS> position_errors_{};
    std::array<float, NUM_STEPPERS> error_integral_{};
    std::array<float, NUM_STEPPERS> last_errors_{};
    
    // Interpolation state
    float interpolation_timer_ = 0.0f;
    bool is_interpolating_ = false;
    bool enabled_ = false;
    
    // Helper methods
    float calculatePIDOutput(uint8_t motor_index, float error);
    void updateInterpolation(float dt);
    void updateVelocities(float dt);
};
