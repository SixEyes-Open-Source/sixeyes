#pragma once
#include <array>
#include "modules/config/board_config.h"

class ServoManager {
public:
    static ServoManager &instance();
    void init();
    void setPositions(const std::array<float, NUM_SERVOS> &positions);
    void enable();
    void disable();
    bool enabled() const;
private:
    ServoManager();
    bool enabled_ = false;
    std::array<float, NUM_SERVOS> positions_{};
};
