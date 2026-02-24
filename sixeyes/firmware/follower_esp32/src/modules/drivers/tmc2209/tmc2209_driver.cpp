#include "tmc2209_driver.h"
#include "tmc2209_config.h"
#include <Arduino.h>
#include <cstring>
#include <TMCStepper.h>

TMC2209Driver &TMC2209Driver::instance() {
    static TMC2209Driver inst;
    return inst;
}

TMC2209Driver::TMC2209Driver() {}

void TMC2209Driver::init(HardwareSerial &uart) {
    uart_ = &uart;
    // initialize PDN pins and set them HIGH (disabled)
    for (size_t i = 0; i < TMC2209_NUM_DRIVERS; ++i) {
        pinMode(TMC2209_PDN_PINS[i], OUTPUT);
        digitalWrite(TMC2209_PDN_PINS[i], HIGH);
    }
    // Start UART for TMC communication. Default safe baud.
    uart_->begin(115200);

    // Create TMCStepper wrappers for each motor (they will use the same UART,
    // PDN selection makes only one active at a time).
    for (uint8_t i = 0; i < TMC2209_NUM_DRIVERS; ++i) {
        drivers[i] = new TMC2209Stepper(uart_, TMC2209_R_SENSE, 0);
        if (drivers[i]) {
            drivers[i]->begin();
            // Use sensible defaults; driver configuration can be tuned later
            drivers[i]->toff(3);
            drivers[i]->rms_current(600); // conservative default (mA)
        }
    }

    Serial.println("TMC2209Driver: init complete (using TMCStepper)");
}

void TMC2209Driver::selectDriver(uint8_t motor_index) {
    // Deselect all first
    deselectAll();
    if (motor_index < TMC2209_NUM_DRIVERS) {
        // Active low: pull PDN low to enable UART on that driver
        digitalWrite(TMC2209_PDN_PINS[motor_index], LOW);
        // small delay to allow device to wake/respond over UART
        delayMicroseconds(200);
    }
}

void TMC2209Driver::deselectAll() {
    for (size_t i = 0; i < TMC2209_NUM_DRIVERS; ++i) {
        digitalWrite(TMC2209_PDN_PINS[i], HIGH);
    }
}

void TMC2209Driver::configureMotor(uint8_t motor_index) {
    Serial.print("TMC2209Driver: configure motor ");
    Serial.println(motor_index);
    // Example: set motor current via writeRegister
    // TODO: replace register addresses and payloads with official TMC2209 values
    setCurrent(motor_index, 800); // 800 mA as example
}

bool TMC2209Driver::readDiagnostics(uint8_t motor_index, uint32_t &diag) {
    // Placeholder: read a hypothetical DIAG register 0x6F
    uint32_t v = 0;
    bool ok = readRegister(motor_index, 0x6F, v, 50);
    if (ok) diag = v;
    return ok;
}

void TMC2209Driver::setCurrent(uint8_t motor_index, uint16_t milliamps) {
    if (motor_index >= TMC2209_NUM_DRIVERS) return;
    if (!drivers[motor_index]) return;
    // Use TMCStepper API to set RMS current (library handles internal scaling)
    drivers[motor_index]->rms_current(milliamps);
    Serial.print("TMC2209Driver: setCurrent ");
    Serial.print(motor_index);
    Serial.print(" -> ");
    Serial.print(milliamps);
    Serial.println(" mA (requested)");
}

// Simple framing for initial testing only.
// Packet format used here (NOT TMC2209 spec):
// [0x55][CMD(1=write,2=read)][REG(1)][DATA(4 little-endian)][CHK(1 sum)]
static uint8_t simple_checksum(const uint8_t *buf, size_t len) {
    uint8_t s = 0;
    for (size_t i = 0; i < len; ++i) s += buf[i];
    return s;
}

bool TMC2209Driver::writeRegister(uint8_t motor_index, uint8_t reg, uint32_t value) {
    // Generic register write wrapper not implemented for all addresses.
    // Use higher-level APIs (e.g. setCurrent) where possible. Return false
    // to indicate not supported for arbitrary registers.
    (void)motor_index; (void)reg; (void)value;
    return false;
}

bool TMC2209Driver::readRegister(uint8_t motor_index, uint8_t reg, uint32_t &value, unsigned long timeout_ms) {
    // Generic register read wrapper not implemented. Provide accessors
    // via specific methods (e.g. DRV_STATUS(), IOIN()) when needed.
    (void)motor_index; (void)reg; (void)timeout_ms;
    value = 0;
    return false;
}

void TMC2209Driver::configureAllMotors() {
    for (uint8_t i = 0; i < TMC2209_NUM_DRIVERS; ++i) {
        configureMotor(i);
    }
}

void TMC2209Driver::setBaud(unsigned long baud) {
    if (uart_) uart_->updateBaudRate(baud);
}
