#pragma once
#include <cstdint>

/**
 * SafetyTask: Monitor heartbeat, control EN pin, manage faults
 * 
 * Runs at CONTROL_LOOP_HZ and enforces:
 * - Motors disabled if heartbeat timeout (~500 ms)
 * - EN pin directly controlled by fault status
 * - Fault latch policy (fault persists until explicit clear)
 * - Safe state on initialization
 */
class SafetyTask {
public:
    static SafetyTask &instance();
    void init();
    void update();  // Call at CONTROL_LOOP_HZ
    
    // Public fault management interface
    void clearFault();
    bool isSafeToOperate() const;
    
private:
    SafetyTask();
    
    static constexpr unsigned long HEARTBEAT_TIMEOUT_MS = 500;
    static constexpr unsigned long FAULT_CHECK_INTERVAL_MS = 10;  // Check every 10ms
    
    unsigned long last_check_ms_ = 0;
    bool motors_enabled_ = false;
    bool user_requested_enable_ = false;
};
