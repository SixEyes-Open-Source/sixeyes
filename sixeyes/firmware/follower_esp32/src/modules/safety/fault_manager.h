#pragma once
#include <cstdint>

class FaultManager {
public:
    enum class Fault : uint32_t { NONE = 0, HEARTBEAT_TIMEOUT = 1, OVERTEMP = 2, OVERCURRENT = 4 };
    static FaultManager &instance();
    void init();
    void raise(Fault f);
    void clear();
    bool hasFault() const;
    Fault current() const;
private:
    FaultManager();
    Fault fault_ = Fault::NONE;
};
