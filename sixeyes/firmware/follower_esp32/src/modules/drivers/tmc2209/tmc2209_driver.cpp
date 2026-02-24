#include "tmc2209_driver.h"
#include <Arduino.h>

TMC2209Driver &TMC2209Driver::instance() {
    static TMC2209Driver inst;
    return inst;
}

TMC2209Driver::TMC2209Driver() {}

void TMC2209Driver::init() {
    Serial.println("TMC2209Driver: init (PDN_UART mode)");
}

void TMC2209Driver::configureMotor(uint8_t motor_index) {
    Serial.print("TMC2209Driver: configure motor ");
    Serial.println(motor_index);
}

bool TMC2209Driver::readDiagnostics(uint8_t motor_index, uint32_t &diag) {
    diag = 0;
    // stub: no real diagnostics
    return true;
}

void TMC2209Driver::setCurrent(uint8_t motor_index, uint16_t milliamps) {
    Serial.print("TMC2209Driver: setCurrent ");
    Serial.print(motor_index);
    Serial.print(" = ");
    Serial.print(milliamps);
    Serial.println(" mA");
}
