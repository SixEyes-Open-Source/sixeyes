#pragma once
#include <array>
#include <cstdint>
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

private:
    MotorController();
    std::array<float, NUM_STEPPERS> current_positions_{};
    std::array<float, NUM_STEPPERS> target_positions_{};
    bool enabled_ = false;
};
