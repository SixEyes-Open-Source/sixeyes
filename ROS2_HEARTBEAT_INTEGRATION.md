# ROS2 Heartbeat Integration Guide

## Overview

The SixEyes follower ESP32 firmware implements a symmetrical heartbeat protocol for safety-critical communication with the ROS2 safety_node running on the laptop. This ensures:

- **Motor Safety**: Automatic motor shutdown if ROS2 heartbeat is lost for >500ms
- **Real-time Feedback**: Status updates at 10 Hz for fault monitoring
- **Deterministic Control**: 400 Hz control loop with ROS2 integration hooks
- **Non-blocking Operation**: All ROS2 communication is asynchronous to preserve timing

## Architecture

### Data Flow

```
ROS2 Safety Node (Laptop)
  ↓ (UART/USB-CDC)
HeartbeatReceiver ← Parses "HB:<source_id>,<seq>\n"
  ↓
HeartbeatMonitor ← Timeout detection (500 ms)
  ↓
SafetyTask ← Enables/disables motors based on heartbeat
  ↓
HeartbeatTransmitter → Sends "SB:<fault>,<motors_en>,<ros2_alive>\n"
  ↓ (USB-CDC back to laptop)
ROS2 Safety Node (visualize, log, trigger E-stop)
```

### Module Breakdown

#### 1. **HeartbeatReceiver** (`heartbeat_receiver.h/.cpp`)
Parses incoming heartbeat packets from ROS2 safety_node.

**Packet Format (ASCII)**:
```
HB:<source_id>,<seq>\n
Example: HB:0,42\n
```

**Features**:
- Non-blocking UART read (compatible with ISR)
- Robust packet parsing with timeout detection
- Packet loss tracking for diagnostics

**Key Methods**:
```cpp
void init(HardwareSerial &uart);              // Initialize UART reference
void update();                                 // Call from main control loop
bool parseHeartbeatPacket(const char *line,   // Parse ASCII format
                          uint32_t &source_id, uint32_t &seq);
uint32_t getPacketsReceived() const;          // Diagnostics
uint32_t getPacketsDropped() const;           // Diagnostics
```

#### 2. **ROS2SafetyBridge** (in `heartbeat_receiver.h/.cpp`)
Monitors ROS2 connectivity status.

**Features**:
- Tracks packet sequence for anomaly detection
- 1-second connection timeout
- Thread-safe connection state

**Key Methods**:
```cpp
bool isROS2Connected() const;     // ROS2 heartbeat OK?
uint32_t getLastSequence() const; // Sequence tracking
void checkConnection();           // Internal state machine
```

#### 3. **HeartbeatTransmitter** (`heartbeat_transmitter.h/.cpp`)
Sends firmware status back to ROS2 safety_node.

**Packet Format (ASCII)**:
```
SB:<fault>,<motors_en>,<ros2_alive>\n
Example: SB:0,1,1\n
```

**Features**:
- 10 Hz transmission rate (100 ms interval)
- Non-blocking USB-CDC writes (drops packets if buffer full)
- Packet statistics for diagnostics

**Key Methods**:
```cpp
void init();                                  // Initialize USB-CDC
void update(bool fault, bool motors_enabled, // Call from SafetyTask
            bool ros2_alive);
void sendStatus(bool fault, bool motors_en,  // Manual transmit
                bool ros2_alive);
uint32_t getPacketsSent() const;             // Diagnostics
uint32_t getPacketsDropped() const;          // Diagnostics
```

#### 4. **SafetyNodeBridge** (in `heartbeat_transmitter.h/.cpp`)
High-level bridge combining receiver + transmitter.

**Features**:
- Bidirectional heartbeat protocol
- Single `update()` call handles RX and TX
- Query method for ROS2 connectivity state

**Key Methods**:
```cpp
void init(HardwareSerial &uart);         // Pass UART for RX
void update(bool fault, bool motors_en); // RX + TX in one call
bool isROS2Alive() const;                // For SafetyTask
```

#### 5. **SafetyTask** Integration (`safety_task.h/.cpp`)
Controls motor enable/disable based on heartbeat status.

**Updated Constructor**:
```cpp
void init(HardwareSerial *uart = nullptr);  // Optional UART for ROS2
```

**New Methods**:
```cpp
bool isROS2Connected() const;  // Query ROS2 status
```

**Heartbeat Logic**:
1. Receives heartbeat from ROS2 via HeartbeatReceiver
2. Checks internal HeartbeatMonitor timeout (500 ms)
3. Combines both checks: `heartbeat_ok = internal_ok AND ros2_alive`
4. Disables motors if **either** internal or ROS2 heartbeat fails
5. Transmits status via HeartbeatTransmitter (10 Hz)

## Protocol Specification

### ROS2 → Firmware (Heartbeat)

**Direction**: Laptop safety_node sends periodically (e.g., 50 Hz)

**Format**:
```
HB:<source_id>,<sequence>\n
```

**Fields**:
- `source_id`: 0 = safety_node, 1+ reserved for multi-source scenarios
- `sequence`: Monotonically increasing counter for gap detection

**Example**:
```
HB:0,0\n
HB:0,1\n
HB:0,2\n
HB:0,3\n
...
HB:0,1000\n
```

**Timeout**: 500 ms (configured in `board_config.h` as `SAFETY_HEARTBEAT_TIMEOUT_MS`)

### Firmware → ROS2 (Status Feedback)

**Direction**: ESP32 sends periodically at 10 Hz (100 ms interval)

**Format**:
```
SB:<fault>,<motors_enabled>,<ros2_alive>\n
```

**Fields**:
- `fault`: 0 = no fault, 1 = fault active (triggers ROS2 E-stop)
- `motors_enabled`: 0 = motors disabled, 1 = motors enabled
- `ros2_alive`: 0 = ROS2 timeout, 1 = ROS2 connected

**Example**:
```
SB:0,0,0\n  (no fault, motors off, waiting for ROS2)
SB:0,0,1\n  (no fault, motors off, ROS2 connected but not enabled)
SB:0,1,1\n  (no fault, motors enabled, ROS2 connected)
SB:1,0,1\n  (fault detected, motors emergency shutdown, ROS2 sees it)
```

## Integration with Control Loop

The heartbeat integration runs entirely within the existing **400 Hz FreeRTOS control loop**:

```
MotorControlScheduler::controlLoop() [400 Hz on core 0]
  ↓
SafetyTask::update()
  ├─ HeartbeatReceiver::update()      [Parse "HB:..." packets]
  ├─ ROS2SafetyBridge::update()       [Check ROS2 connectivity]
  ├─ HeartbeatMonitor::check()        [Internal timeout check]
  ├─ Safety logic                      [Enable/disable motors]
  └─ HeartbeatTransmitter::update()   [Send "SB:..." status]
  ↓
MotorController::update()
  ↓
ServoManager::update()
  ↓
[Back to SafetyTask next cycle]
```

## Timing Guarantees

| Component              | Frequency | Timing  | Blocking? |
|:----------------------|:---------:|:-------:|:---------:|
| SafetyTask check       | 100 Hz    | 10 ms   | No (skip if not ready) |
| Heartbeat RX parse     | Variable  | <1 ms   | No (UART read) |
| Heartbeat TX send      | 10 Hz     | 100 ms  | No (USB buffer check) |
| Control loop period    | 400 Hz    | 2.5 ms  | Strict (busy-wait) |

**Key Property**: ROS2 communication never blocks the 400 Hz control loop.

## ROS2 Safety Node Protocol (Laptop Side)

Example pseudocode for ROS2 safety_node:

```python
#!/usr/bin/env python3
import serial
import threading
import time

class ROSHeartbeatSender:
    def __init__(self, port="/dev/ttyUSB0", baudrate=115200):
        self.ser = serial.Serial(port, baudrate)
        self.sequence = 0
        self.send_thread = threading.Thread(target=self._sender_loop)
        self.send_thread.daemon = True
        self.send_thread.start()
        
        self.recv_thread = threading.Thread(target=self._receiver_loop)
        self.recv_thread.daemon = True
        self.recv_thread.start()
        
    def _sender_loop(self):
        """Send heartbeat at 50 Hz"""
        while True:
            msg = f"HB:0,{self.sequence}\n"
            self.ser.write(msg.encode())
            self.sequence += 1
            time.sleep(0.02)  # 50 Hz
            
    def _receiver_loop(self):
        """Listen for status updates"""
        while True:
            if self.ser.in_waiting > 0:
                line = self.ser.readline().decode().strip()
                if line.startswith("SB:"):
                    fault, motors_en, ros2_alive = map(int, line[3:].split(','))
                    print(f"Status: fault={fault}, motors={motors_en}, ros2={ros2_alive}")
                    
                    if fault:
                        print("[ALERT] Firmware detected fault! Triggering E-stop")
                        # TODO: ROS2 publish /emergency_stop
                        
if __name__ == "__main__":
    hb = ROSHeartbeatSender()
    input("Heartbeat running. Press Ctrl+C to exit...\n")
```

## Error Detection & Recovery

### Scenario 1: ROS2 Heartbeat Loss

1. **At t=0 ms**: Last heartbeat received
2. **At t=500 ms**: SafetyTask detects timeout
3. **Action**: Disable motors immediately (EN pin = LOW)
4. **Transmission**: `SB:1,0,0\n` (fault=1, motors_en=0, ros2_alive=0)
5. **Recovery**: Any new "HB:..." packet re-enables (if no other faults)

### Scenario 2: Firmware Fault (e.g., motor stall)

1. **At t=123 ms**: Motor stall detected → FaultManager::raise()
2. **Immediate Action**: Disable motors (EN pin = LOW)
3. **Transmission**: `SB:1,0,1\n` (fault=1, motors_en=0, ros2_alive=1)
4. **ROS2 Receives**: Triggers E-stop announcement, logs event
5. **Recovery**: User calls diagnostics, clears fault via ROS2 command (if implemented)

### Scenario 3: Internal Heartbeat Loss (No ROS2)

If ROS2 bridge is **not initialized** (uart=nullptr):

- Control loop still runs autonomously
- Motors use internal HeartbeatMonitor (500 ms timeout)
- SafetyTask::isROS2Connected() returns false
- Status packets are NOT sent (no USB CDC)
- Useful for standalone testing without ROS2

## Diagnostics

### Print Status (from firmware via Serial Monitor)

```
// In SafetyTask::update(), call periodically:
if (loop_count % 400 == 0) {  // Every 1 second at 400 Hz
    SafetyNodeBridge::instance().printBridgeStatus();
}
```

**Output Example**:
```
[SafetyNodeBridge] Status:
[ROS2SafetyBridge] Connected: YES, Last Seq: 42
[HeartbeatReceiver] Received: 42, Dropped: 0
[HeartbeatTransmitter] Sent: 400, Dropped: 0
```

### Sequence Analysis

If you see:
- `Received: 100, Last Seq: 95` → **Packet loss detected** (HB:0,95 was the last)
- `Received: 0` → **No heartbeats from ROS2**
- `Dropped: N > 0` → **USB CDC buffer overflow** (add diagnostic burst)

## Configuration

### board_config.h

```cpp
#define SAFETY_HEARTBEAT_TIMEOUT_MS 500   // ROS2 heartbeat timeout
#define HEARTBEAT_RX_BUFFER_SIZE    64    // UART RX buffer for "HB:..."
#define HEARTBEAT_TX_INTERVAL_MS    100   // 10 Hz status transmission
#define ROS2_TIMEOUT_MS             1000  // ROS2 connectivity timeout
```

### platformio.ini

```ini
[env:follower_esp32]
build_flags = 
    -DCORE_MHZ=240
    -DSAFETY_HEARTBEAT_TIMEOUT_MS=500
```

## Testing Checklist

- [ ] Compile firmware with `pio run` (verify 400 Hz control loop runs)
- [ ] Monitor USB-CDC output: `picocom /dev/ttyUSB0 115200`
- [ ] Send heartbeats manually: `echo "HB:0,1" > /dev/ttyUSB0`
- [ ] Verify motor disable after 500 ms of no heartbeat
- [ ] Verify motor re-enable when heartbeats resume
- [ ] Check status packets received: `SB:...` every 100 ms
- [ ] Test with ROS2 safety_node connected
- [ ] Verify E-stop transition (fault=1) in ROS2

## Next Steps

1. **ROS2 Safety Node**: Fully implement heartbeat sender in Python
2. **Testbed**: Deploy to actual hardware with loaded motor
3. **Validation**: Measure heartbeat latency, jitter, packet loss rate
4. **Documentation**: Hardware setup guide with wiring diagrams
5. **Release**: Archive firmware.bin with git tag

## Files Modified

- `src/modules/safety/safety_task.h/cpp` - Updated init() signature
- `src/modules/safety/heartbeat_receiver.h/cpp` - New module
- `src/modules/safety/heartbeat_transmitter.h/cpp` - New module
- `src/main.cpp` - Integrated SafetyTask + MotorControlScheduler
- `platformio.ini` - No changes (all config in board_config.h)

---

**Version**: 1.0  
**Last Updated**: [Current Date]  
**Author**: SixEyes Development Team  
