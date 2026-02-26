# SixEyes Firmware Feature Inventory

This document lists small, concrete firmware/runtime features for documentation and tracking.

## Scope

- Follower firmware: `sixeyes/firmware/follower_esp32`
- Leader firmware: `sixeyes/firmware/leader_esp32`
- Teleoperation bridge tool: `sixeyes/tools/teleoperation_bridge.py`

## 1) Mode and Routing

- Compile-time mode selection via `OPERATION_MODE`
  - `MODE_VLA_INFERENCE` (1)
  - `MODE_TELEOPERATION` (2)
- Mode-specific parameter macros (timeouts, rates, comms settings)
- Central message routing by mode (`MessageRouter`)
- Mode-name diagnostics (`getModeName`, `MODE_NAME`)

## 2) JSON Protocol and Parsing

- Line-delimited JSON parser (non-blocking)
- Message enum and typed payload structs
- Handler registration/unregistration by message type
- Global parse-error callback support
- Parse statistics: message count and error count
- JSON command support includes:
  - `MOTOR_TARGET`
  - `SERVO_TARGET`
  - `ENABLE_MOTORS`
  - `RESET_FAULT`
  - `TUNE_PID`
  - `CONFIG_PARAM`
  - `TELEMETRY_ENABLE`
  - `TELEMETRY_RATE`
  - `HEARTBEAT`

## 3) VLA Inference Path

- `MOTOR_TARGET` -> absolute motor target updates
- `SERVO_TARGET` -> degree or PWM-us servo updates
- `ENABLE_MOTORS` -> motor enable/disable control
- Parser-level heartbeat handling path
- Parser error logging callbacks

## 4) Teleoperation Path (Current)

- `JOINT_STATE` ingestion with `valid_mask` filtering
- Joint mapping to:
  - 4 stepper targets (motor controller)
  - 2 servo targets (servo manager)
- Servo clamping to safe degree range
- `TELEMETRY_STATE` response emission with:
  - `seq`, `ts`
  - `follower_joints`
  - `errors`
  - `faults`
  - `motors_en`
- Unsupported-command guard path in teleoperation mode

## 5) Safety and Heartbeat

- Heartbeat timeout enforcement in `SafetyTask`
- Motor enable gating from safety state
- Fault clear / safe-to-operate checks
- ROS2 connectivity state exposure
- ASCII heartbeat RX parser (`HB:source,seq`)
- ASCII status TX packet (`SB:fault,motors_en,ros2_alive`)
- RX/TX packet statistics

## 6) Deterministic Scheduling and Control

- FreeRTOS deterministic scheduler at control-loop rate
- Ordered loop execution (safety -> motor -> servo -> monitor)
- Loop timing statistics (count, average, max)
- Motor controller features:
  - PID-based closed-loop control
  - Absolute target API
  - Interpolation support
  - Current position/velocity/error diagnostics
- Servo manager features:
  - Multi-servo position API
  - Direct pulse-width API
  - Degree<->pulse mapping
  - Watchdog timeout handling

## 7) Drivers, HAL, and Utility Modules

- TMC2209 driver integration layer
- HAL GPIO abstraction
- HAL ADC abstraction
- Telemetry formatter helpers
- Logging helpers (`info`, `warn`, `error`, `debug`, formatted variants)

## 8) Leader Firmware Features

- 6-channel ADC sampling
- Degree conversion from raw ADC
- `JOINT_STATE` publish at 100 Hz
- `valid_mask` generation
- Sequence and timestamp fields

## 9) Teleoperation Bridge Tool Features

- Leader -> follower forwarding for validated `JOINT_STATE`
- Follower telemetry intake (`TELEMETRY_STATE`)
- Optional direction-tagged JSONL logging
- Basic bridge counters:
  - packets in/forwarded/dropped
  - parse errors
  - telemetry in/parse errors
- Stream quality metrics:
  - sequence gaps
  - out-of-order packets
  - source latency avg/max
  - interarrival average
  - jitter average

## 10) Not Yet Complete (Track as TODO)

- Full follower telemetry content (currently minimal/stub-oriented fields)
- Complete ROS2 teleoperation runtime integration in `ros2_ws`
- Automated end-to-end integration tests for teleoperation chain
- Production calibration/mapping profiles for leader/follower joint alignment
