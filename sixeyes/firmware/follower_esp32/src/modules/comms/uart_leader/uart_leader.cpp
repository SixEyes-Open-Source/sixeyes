#include "uart_leader.h"
#include <Arduino.h>

UartLeader &UartLeader::instance() {
    static UartLeader inst;
    return inst;
}

UartLeader::UartLeader() {}

void UartLeader::init(unsigned long baud) {
    Serial1.begin(baud);
    Serial.println("UartLeader: Serial1 started");
}

bool UartLeader::available() {
    return Serial1.available() > 0;
}

bool UartLeader::readAbsoluteTargets(std::array<float, NUM_STEPPERS> &out_targets, uint32_t &out_seq) {
    // Minimal stub: no real parsing. Return false unless data present.
    if (!available()) return false;
    // For now consume input and return false to indicate no valid packet.
    while (Serial1.available()) Serial1.read();
    return false;
}

void UartLeader::sendAck(uint32_t seq) {
    Serial1.print("ACK:");
    Serial1.println(seq);
}
