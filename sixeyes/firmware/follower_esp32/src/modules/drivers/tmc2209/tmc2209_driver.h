#pragma once
#include <cstdint>
#include <Arduino.h>
#include <TMCStepper.h>
#include "tmc2209_config.h"

class TMC2209Driver {
public:
    static TMC2209Driver &instance();
    // Initialize UART and PDN pins; uses Serial2 by default.
    void init(HardwareSerial &uart = Serial2);

    // High-level configuration
    void configureMotor(uint8_t motor_index);
    bool readDiagnostics(uint8_t motor_index, uint32_t &diag);
    void setCurrent(uint8_t motor_index, uint16_t milliamps);
    // Convenience: configure all detected motors with safe defaults
    void configureAllMotors();
    // Set UART baud used for drivers
    void setBaud(unsigned long baud);

    // Low-level register access (uses TMCStepper library for UART register protocol)
    // NOTE: TMCStepper implements the official TMC2209 register protocol
    // including CRC and addressing; this wrapper selects the PDN_UART
    // target and delegates reads/writes to that library.
    bool writeRegister(uint8_t motor_index, uint8_t reg, uint32_t value);
    bool readRegister(uint8_t motor_index, uint8_t reg, uint32_t &value, unsigned long timeout_ms = 50);

private:
    TMC2209Driver();
    HardwareSerial *uart_ = nullptr;
    // TMCStepper driver instances (one per physical TMC2209)
    TMC2209Stepper *drivers[TMC2209_NUM_DRIVERS];
    void selectDriver(uint8_t motor_index);
    void deselectAll();
};

