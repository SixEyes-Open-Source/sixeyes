#pragma once
#include <cstdint>

class TMC2209Driver {
public:
    static TMC2209Driver &instance();
    void init();
    void configureMotor(uint8_t motor_index);
    bool readDiagnostics(uint8_t motor_index, uint32_t &diag);
    void setCurrent(uint8_t motor_index, uint16_t milliamps);
private:
    TMC2209Driver();
};
