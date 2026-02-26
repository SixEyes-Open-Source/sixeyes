#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include "modules/config/board_config.h"
#include "modules/drivers/tmc2209/tmc2209_config.h"

class MotorController {
public:
    static MotorController &instance();
    void init();
    void setAbsoluteTargets(const std::array<float, NUM_STEPPERS> &targets);
    void update(); // called at CONTROL_LOOP_HZ
    void enableMotors();
    void disableMotors();
    bool motorsEnabled() const;
    void setCurrentPositionAsZero();
    
    // Accessors for diagnostics
    std::array<float, NUM_STEPPERS> getCurrentPositions() const;
    std::array<float, NUM_STEPPERS> getCurrentVelocities() const;
    std::array<float, NUM_STEPPERS> getErrors() const;

private:
    MotorController();
    
    // Open-loop motion constants (motor shaft degrees).
    static constexpr float MAX_MOTOR_SPEED_DEG_PER_S = 720.0f;
    static constexpr float POSITION_TOLERANCE_DEG = 0.05f;
    static constexpr uint32_t STEP_PULSE_HIGH_US = 2;
    static constexpr uint32_t STEP_PULSE_LOW_US = 2;
    
    // State tracking
    std::array<float, NUM_STEPPERS> current_positions_{};    // motor-shaft deg
    std::array<float, NUM_STEPPERS> current_velocities_{};   // motor-shaft deg/s
    std::array<float, NUM_STEPPERS> target_positions_{};     // motor-shaft deg
    std::array<float, NUM_STEPPERS> position_errors_{};      // target-current deg
    std::array<int32_t, NUM_STEPPERS> current_steps_{};
    std::array<int32_t, NUM_STEPPERS> target_steps_{};
    std::array<int32_t, NUM_STEPPERS> zero_offsets_steps_{};

    bool is_interpolating_ = false;
    bool enabled_ = false;
    
    // Helper methods
    static float stepsToDegrees(int32_t steps);
    static int32_t degreesToSteps(float degrees);
    void pulseStepPin(uint8_t motor_index);
    void setupStepPins();
    void updateVelocitiesFromStepDelta(const std::array<int32_t, NUM_STEPPERS>& step_delta, float dt);
};
