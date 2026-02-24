#pragma once
#include <cstdint>
#include <array>
#include "modules/config/board_config.h"

class UartLeader {
public:
    static UartLeader &instance();
    void init(unsigned long baud = 115200);
    bool available();
    // Parse into absolute joint array; returns true on success
    bool readAbsoluteTargets(std::array<float, NUM_STEPPERS> &out_targets, uint32_t &out_seq);
    void sendAck(uint32_t seq);
private:
    UartLeader();
};
