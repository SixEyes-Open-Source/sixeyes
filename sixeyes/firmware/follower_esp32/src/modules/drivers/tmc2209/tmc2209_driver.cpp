#include "tmc2209_driver.h"
#include "tmc2209_config.h"
#include <Arduino.h>
#include <cstring>

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
    uart_->begin(115200);
    Serial.println("TMC2209Driver: init complete (PDN_UART placeholder)");
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
    // Placeholder mapping: convert mA to register value (fake mapping)
    uint32_t regval = (uint32_t)milliamps; // TODO: convert to driver current register units
    bool ok = writeRegister(motor_index, 0x10, regval);
    Serial.print("TMC2209Driver: setCurrent ");
    Serial.print(motor_index);
    Serial.print(" -> ");
    Serial.print(milliamps);
    Serial.print(" mA : ");
    Serial.println(ok ? "OK" : "ERR");
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
    if (!uart_) return false;
    const int MAX_TRIES = 3;
    bool ok = false;
    for (int attempt = 0; attempt < MAX_TRIES; ++attempt) {
        selectDriver(motor_index);
        uint8_t pkt[1 + 1 + 1 + 4 + 1];
        size_t p = 0;
        pkt[p++] = 0x55; // start
        pkt[p++] = 1; // cmd=write
        pkt[p++] = reg;
        // little-endian
        pkt[p++] = (uint8_t)(value & 0xFF);
        pkt[p++] = (uint8_t)((value >> 8) & 0xFF);
        pkt[p++] = (uint8_t)((value >> 16) & 0xFF);
        pkt[p++] = (uint8_t)((value >> 24) & 0xFF);
        pkt[p++] = simple_checksum(pkt, p);
        uart_->write(pkt, p+1);
        uart_->flush();
        // wait for ack (not real TMC behavior) - read one byte
        unsigned long start = millis();
        bool heard = false;
        while (millis() - start < 75) {
            if (uart_->available()) {
                int b = uart_->read();
                if (b == 0x06) { heard = true; break; } // ACK
            }
        }
        deselectAll();
        if (heard) { ok = true; break; }
        // retry after short delay
        delay(10);
    }
    return ok;
}

bool TMC2209Driver::readRegister(uint8_t motor_index, uint8_t reg, uint32_t &value, unsigned long timeout_ms) {
    if (!uart_) return false;
    selectDriver(motor_index);
    uint8_t pkt[1 + 1 + 1 + 1];
    size_t p = 0;
    pkt[p++] = 0x55;
    pkt[p++] = 2; // cmd=read
    pkt[p++] = reg;
    pkt[p++] = simple_checksum(pkt, p);
    uart_->write(pkt, p);
    uart_->flush();
    unsigned long start = millis();
    uint8_t resp[6];
    size_t got = 0;
    while (millis() - start < timeout_ms && got < sizeof(resp)) {
        while (uart_->available() && got < sizeof(resp)) {
            resp[got++] = (uint8_t)uart_->read();
        }
    }
    deselectAll();
    if (got >= 6 && resp[0] == 0x55 && resp[1] == 0x02 && resp[2] == reg) {
        // resp layout: [0x55][0x02][reg][4 data][chk]
        if (got < 8) return false;
        uint32_t v = (uint32_t)resp[3] | ((uint32_t)resp[4] << 8) | ((uint32_t)resp[5] << 16) | ((uint32_t)resp[6] << 24);
        // basic checksum validation
        uint8_t chk = resp[7];
        uint8_t sum = simple_checksum(resp, 7);
        if (chk != sum) return false;
        value = v;
        return true;
    }
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
