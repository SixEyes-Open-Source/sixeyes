#pragma once
#include <cstdint>
#include <Arduino.h>

class TMC2209Driver {
public:
    static TMC2209Driver &instance();
    // Initialize UART and PDN pins; uses Serial2 by default.
    void init(HardwareSerial &uart = Serial2);

    // High-level configuration
    void configureMotor(uint8_t motor_index);
    bool readDiagnostics(uint8_t motor_index, uint32_t &diag);
    void setCurrent(uint8_t motor_index, uint16_t milliamps);

    // Low-level register access (placeholders for real TMC2209 protocol)
    // NOTE: These use a simple framing for initial testing. Replace with
    // the official TMC2209 UART framing (CRC, addressing) before hardware use.
    bool writeRegister(uint8_t motor_index, uint8_t reg, uint32_t value);
    bool readRegister(uint8_t motor_index, uint8_t reg, uint32_t &value, unsigned long timeout_ms = 50);

private:
    TMC2209Driver();
    HardwareSerial *uart_ = nullptr;
    void selectDriver(uint8_t motor_index);
    void deselectAll();
};

