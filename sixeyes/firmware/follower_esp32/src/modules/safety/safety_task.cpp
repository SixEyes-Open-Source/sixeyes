#include "safety_task.h"
#include "heartbeat_monitor.h"
#include "fault_manager.h"
#include "modules/hal/gpio.h"
#include "modules/config/board_config.h"
#include <Arduino.h>

SafetyTask &SafetyTask::instance() {
    static SafetyTask inst;
    return inst;
}

SafetyTask::SafetyTask() {}

void SafetyTask::init() {
    Serial.println("SafetyTask: initialized");
    Serial.println("  - Heartbeat timeout: 500 ms");
    Serial.println("  - Motors disabled by default");
    Serial.println("  - Fault latch enabled (clear required)");
    
    // Initialize subsystems
    HAL_GPIO::init();
    FaultManager::instance().init();
    HeartbeatMonitor::instance().init(HEARTBEAT_TIMEOUT_MS);
    
    // Motors start disabled
    motors_enabled_ = false;
    user_requested_enable_ = false;
    last_check_ms_ = millis();
}

void SafetyTask::update() {
    unsigned long now_ms = millis();
    
    // Check heartbeat at regular intervals (not every call, to save cycles)
    if ((now_ms - last_check_ms_) < FAULT_CHECK_INTERVAL_MS) {
        return;
    }
    last_check_ms_ = now_ms;
    
    // Run heartbeat monitor check
    HeartbeatMonitor::instance().check();
    
    // Determine if heartbeat is healthy
    bool heartbeat_ok = HeartbeatMonitor::instance().isHealthy();
    
    // Fault logic: raise HEARTBEAT_TIMEOUT if heartbeat lost
    if (!heartbeat_ok && !FaultManager::instance().hasFault()) {
        FaultManager::instance().raise(FaultManager::Fault::HEARTBEAT_TIMEOUT);
        Serial.println("SafetyTask: HEARTBEAT_TIMEOUT raised - disabling motors");
    }
    
    // EN pin control: motors can only run if:
    // 1. No fault is present
    // 2. User has requested enable (via setAbsoluteTargets from motor_controller)
    bool safe_to_operate = !FaultManager::instance().hasFault() && heartbeat_ok;
    bool should_enable = safe_to_operate && user_requested_enable_;
    
    // Update EN pin state
    if (should_enable && !motors_enabled_) {
        HAL_GPIO::setMotorEnable(true);
        motors_enabled_ = true;
        Serial.println("SafetyTask: EN pin HIGH - motors enabled");
    } else if (!should_enable && motors_enabled_) {
        HAL_GPIO::setMotorEnable(false);
        motors_enabled_ = false;
        Serial.println("SafetyTask: EN pin LOW - motors disabled");
    }
}

void SafetyTask::clearFault() {
    FaultManager::instance().clear();
    Serial.println("SafetyTask: fault cleared - ready to enable motors on next heartbeat");
}

bool SafetyTask::isSafeToOperate() const {
    return !FaultManager::instance().hasFault() && 
           HeartbeatMonitor::instance().isHealthy();
}
