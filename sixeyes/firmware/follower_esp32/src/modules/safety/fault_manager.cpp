#include "fault_manager.h"
#include <Arduino.h>

FaultManager &FaultManager::instance() {
    static FaultManager inst;
    return inst;
}

FaultManager::FaultManager() {}

void FaultManager::init() {
    fault_ = Fault::NONE;
}

void FaultManager::raise(Fault f) {
    fault_ = f;
    Serial.print("FaultManager: fault raised ");
    Serial.println(static_cast<uint32_t>(f));
}

void FaultManager::clear() {
    fault_ = Fault::NONE;
    Serial.println("FaultManager: cleared");
}

bool FaultManager::hasFault() const { return fault_ != Fault::NONE; }

FaultManager::Fault FaultManager::current() const { return fault_; }
