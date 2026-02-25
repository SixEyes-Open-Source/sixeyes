# ROS2 Heartbeat Integration - Quick Start Guide

## Hardware Required

- **SixEyes ESP32-S3 Follower Robot**
- **Laptop with USB-CDC (serial) connection to ESP32**
- **Terminal emulator** (picocom, minicom, or PuTTY)
- **Python 3.7+** with `pyserial` library

## Step 1: Identify Serial Port

### Linux/Mac
```bash
$ ls -la /dev/tty*USB* /dev/tty.usb*
# Examples:
# /dev/ttyUSB0 (ESP32)
# /dev/ttyUSB1 (optional: separate UART for TMC2209)
```

### Windows (PowerShell)
```powershell
PS> [System.IO.Ports.SerialPort]::GetPortNames()
# Examples:
# COM3 (ESP32)
# COM4 (optional: separate UART)
```

## Step 2: Open Serial Monitor

### Using picocom (Linux)
```bash
picocom /dev/ttyUSB0 -b 115200
# Press Ctrl+A then Ctrl+X to exit
```

### Using minicom (Linux)
```bash
minicom -D /dev/ttyUSB0 -b 115200
# Press Ctrl+A then Z then X to exit
```

### Using PuTTY (Windows)
1. Open PuTTY
2. Select "Serial" connection type
3. Set Port: COM3 (or whatever your port is)
4. Set Speed: 115200
5. Click "Open"

### Using Python
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200)
while True:
    if ser.in_waiting:
        print(ser.readline().decode().strip())
```

## Step 3: Verify Firmware is Running

You should see the firmware startup messages:
```
SixEyes follower_esp32 starting
[HAL_GPIO] Initializing
[TMC2209Driver] Initialized
[MotorController] Initialized
[ServoManager] Initialized
[SafetyTask] initialized
  - Heartbeat timeout: 500 ms
  - Motors disabled by default
  - Fault latch enabled (clear required)
  - ROS2 heartbeat bridge: ENABLED
[MotorControlScheduler] Initialized
[MotorControlScheduler] starting control loop task...
Initialization complete; control loop running on FreeRTOS
```

**Key signal**: You should NOT see "EN pin HIGH - motors enabled" yet (waiting for ROS2 heartbeat).

## Step 4: Test Manual Heartbeat

### From another terminal

```bash
# Send a single heartbeat (sequence #0)
echo "HB:0,0" > /dev/ttyUSB0

# Or send multiple heartbeats
for i in {0..10}; do
  echo "HB:0,$i"
  sleep 0.1
done > /dev/ttyUSB0
```

### Expected Response (in serial monitor)

After sending heartbeats, you should see:
```
[SafetyTask] EN pin HIGH - motors enabled
SB:0,1,1
SB:0,1,1
SB:0,1,1
...
```

**Explanation**:
- `SafetyTask` enables motors (EN pin goes HIGH)
- `SB:0,1,1` = no fault, motors enabled, ROS2 alive

## Step 5: Test Heartbeat Timeout

### From serial monitor, send one heartbeat:
```bash
echo "HB:0,0" > /dev/ttyUSB0
```

### Wait 500+ ms without sending more heartbeats

### Expected Response (in serial monitor):
```
[SafetyTask] EN pin HIGH - motors enabled
SB:0,1,1

[After 500 ms of silence...]

SafetyTask: HEARTBEAT_TIMEOUT raised - disabling motors
[SafetyTask] EN pin LOW - motors disabled
SB:1,0,0

[Repeat SB:1,0,0 every 100 ms...]
```

**Explanation**:
- Motors are **immediately disabled** when heartbeat times out
- Firmware sends `SB:1,0,0` (fault=1) to alert ROS2
- Motors stay disabled until heartbeat resumes

## Step 6: Run Python Heartbeat Node

### Install dependencies
```bash
pip install pyserial
```

### Run the heartbeat bridge
```bash
python ros2_heartbeat_node.py --port /dev/ttyUSB0 --hz 50
```

### Expected Output
```
[INFO] Connected to /dev/ttyUSB0 @ 115200 baud
[INFO] Heartbeat bridge started (50 Hz)

[FW] SixEyes follower_esp32 starting
[FW] [HAL_GPIO] Initializing
[FW] [TMC2209Driver] Initialized
...
[FW] [MotorControlScheduler] control loop task created successfully
[FW] Initialization complete; control loop running on FreeRTOS

[After 2-3 seconds, heartbeats kick in...]

[STATS] SixEyes Heartbeat Bridge
  Packets sent:     42
  Packets received: 41
  Sequence:         42
  Last Status:
    Fault:          False
    Motors enabled: True
    ROS2 alive:     True
    Age:            0.1s
```

## Step 7: Test Fault Detection

### Trigger a firmware fault (simulate motor stall)

If you have a way to trigger a fault from your code:
```cpp
// In SafetyTask::update(), for testing:
if (some_error_condition) {
    FaultManager::instance().raise(FaultManager::Fault::MOTOR_STALL);
}
```

### Expected Python Output
```
[ALERT] Firmware fault detected! Motors: False, ROS2: True
```

### Expected Serial Monitor Output
```
SafetyTask: EN pin LOW - motors disabled
SB:1,0,1
```

## Diagnostics

### Check Heartbeat Stats

In the firmware, you can call (from SafetyTask every 1-2 seconds):
```cpp
SafetyNodeBridge::instance().printBridgeStatus();
```

### Expected Output
```
[SafetyNodeBridge] Status:
[ROS2SafetyBridge] Connected: YES, Last Seq: 1234
[HeartbeatReceiver] Received: 1234, Dropped: 0
[HeartbeatTransmitter] Sent: 2468, Dropped: 0
```

### Interpret Results

| Metric | Healthy | Problem |
|:-------|:-------:|:--------:|
| `Received > 0` | ✓ | ✗ No heartbeats from ROS2 |
| `Dropped == 0` | ✓ | ✗ Packet loss in UART |
| `Sent > Received*2` | ✓ | ✗ Very few responses (buffer full?) |
| `Connected: YES` | ✓ | ✗ ROS2 timeout detected |
| `Last Seq` increasing | ✓ | ✗ Stalled (same sequence) |

## Common Issues & Fixes

### Issue: "Port not found" / "Permission denied"

**Linux**:
```bash
# Check permissions
ls -la /dev/ttyUSB0
# If not readable, add current user to dialout group:
sudo usermod -a -G dialout $USER
# Log out and back in for changes to take effect
```

**Windows**:
- Try a different USB port
- Update CH340 driver (common ESP32 USB chip)
- Run terminal as Administrator

### Issue: No output on serial monitor

**Checks**:
1. Verify baud rate = 115200
2. Try unplugging USB and replugging
3. Check with different terminal software
4. Try `pio run -v` to verify firmware is up to date

### Issue: Motors don't enable despite heartbeats

**Root causes**:
1. **UART not initialized** - Check `SafetyTask::init(&Serial)` was called with correct UART
2. **Heartbeat not being parsed** - Check if `HB:0,X\n` format is correct
3. **Another fault is active** - Run `printBridgeStatus()` to check fault status
4. **EN pin not wired** - Check GPIO7 (EN pin) is connected to motor driver

### Issue: Heartbeat timeout triggers immediately

**Root causes**:
1. **Baud rate mismatch** - Verify 115200 on both sides
2. **Junk data in UART** - Check for EMI or loose connections
3. **Timing issue** - 500 ms timeout might be too short for slow ROS2 node

**Solution**: Send heartbeats >= 50 Hz to leave margin

## Integration with ROS2

### Example ROS2 Callback

```python
import rclpy
from rclpy.node import Node

class SixEyesSafetyNode(Node):
    def __init__(self):
        super().__init__('sixeyes_safety')
        
        from ros2_heartbeat_node import SixEyesHeartbeatBridge
        self.bridge = SixEyesHeartbeatBridge('/dev/ttyUSB0')
        self.bridge.connect()
        self.bridge.start()
        
        # Publisher for status
        self.status_pub = self.create_publisher(...)
        
        # Callback
        self.bridge.on_fault = self._on_firmware_fault
        
    def _on_firmware_fault(self, msg: str):
        """Trigger ROS2 emergency stop"""
        self.get_logger().error(f"Firmware fault: {msg}")
        self.publish_emergency_stop()

if __name__ == '__main__':
    rclpy.init()
    node = SixEyesSafetyNode()
    rclpy.spin(node)
```

## Performance Testing

### Measure Heartbeat Latency

```bash
# Send heartbeat and measure response time
TIME_START=$(date +%s%N)
echo "HB:0,0" > /dev/ttyUSB0
# Watch for "SB:0,1,1" in serial monitor
TIME_END=$(date +%s%N)
LATENCY_NS=$((TIME_END - TIME_START))
echo "Heartbeat latency: ${LATENCY_NS}ns"
```

### Expected Latency

| Component | Time |
|:----------|:----:|
| UART RX   | <1 ms |
| SafetyTask check | <1 ms |
| UART TX | <1 ms |
| **Total** | **<3 ms** |

### Measure Packet Loss Rate

```bash
# Send 100 heartbeats and check received count
python ros2_heartbeat_node.py --port /dev/ttyUSB0
# Let it run for 10 seconds (500 packets)
# Expected: ~499 received (99.8% delivery)
```

## Success Criteria ✓

You've successfully integrated ROS2 heartbeat when:

- [ ] Serial monitor shows startup messages
- [ ] Manual heartbeat enables motors
- [ ] No heartbeat for 500 ms disables motors
- [ ] Python node connects and shows status updates
- [ ] `SB:` packets appear at ~10 Hz
- [ ] No packet loss (Dropped = 0)
- [ ] Motors respond to heartbeat within 10 ms
- [ ] Fault handling works (motors disable on error)

## Next Steps

1. **Hardware bring-up**: Connect actual motor and verify EN pin controls motor enable
2. **Stress testing**: Send 1000+ heartbeats, check for timeouts or glitches
3. **EMI testing**: Run heartbeat with motor spinning, verify no false alarms
4. **ROS2 integration**: Connect to actual ROS2 safety_node and test E-stop
5. **Documentation**: Run tests with multiple configurations and record results

---

**Need Help?**
- Check firmware serial output for error messages
- Review `SafetyTask::update()` logic in source code
- Verify USB cable is high-quality (some cheap cables have bad shielding)
- Test with different heartbeat frequencies (if timeout keeps triggering, try 100+ Hz)
