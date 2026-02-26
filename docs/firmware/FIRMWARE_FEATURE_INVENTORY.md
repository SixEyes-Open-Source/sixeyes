# SixEyes Firmware Documentation Checklist

Track firmware documentation completion with explicit owner, status, and priority.

## Scope

- Follower firmware: `sixeyes/firmware/follower_esp32`
- Leader firmware: `sixeyes/firmware/leader_esp32`
- Teleoperation bridge: `sixeyes/tools/teleoperation_bridge.py`

## Status Legend

- `todo` = not documented yet
- `in-progress` = partial docs exist, needs refinement
- `done` = documented and reviewed

## Checklist Table

| ID | Feature Area | What to Document | Owner | Priority | Status | Primary Source | Notes |
|:---|:-------------|:-----------------|:------|:---------|:-------|:---------------|:------|
| FW-001 | Mode Selection | `OPERATION_MODE` usage and defaults | Vincent | P0 | in-progress | `modules/config/mode_config.h` | Ensure examples for mode 1/2 are consistent in all docs |
| FW-002 | Mode Router | MessageRouter init + route behavior | Vincent | P1 | in-progress | `modules/comms/message_router.*` | Add command-flow diagram per mode |
| FW-003 | JSON Parser Core | Non-blocking parse model and buffer limits | Vincent | P0 | in-progress | `modules/comms/uart_json_parser/*` | Include overflow behavior and error handling |
| FW-004 | JSON Message Types | Full command/response matrix | Vincent | P0 | in-progress | `uart_json_messages.h` | Keep docs/protocols aligned with enum names |
| FW-005 | VLA Command Path | MOTOR/SERVO/ENABLE command handling | Vincent | P1 | in-progress | `modes/vla_inference/*` | Add sequence diagram for VLA path |
| FW-006 | Teleop JOINT_STATE | Joint mapping + valid_mask semantics | Vincent | P0 | in-progress | `modes/teleoperation/teleoperation_handler.cpp` | Document 6-joint mapping assumptions |
| FW-007 | TELEMETRY_STATE Payload | Output fields and update rate | Vincent | P0 | in-progress | `teleoperation_handler.cpp` | Current output is minimal; mark planned extensions |
| FW-008 | SafetyTask Logic | Enable/disable gate and fault interactions | Vincent | P0 | in-progress | `modules/safety/safety_task.*` | Include truth table for safe-to-operate |
| FW-009 | Heartbeat RX | `HB:source,seq` parser details | Vincent | P1 | in-progress | `modules/safety/heartbeat_receiver.*` | Add packet examples and edge cases |
| FW-010 | Heartbeat TX | `SB:fault,motors_en,ros2_alive` timing | Vincent | P1 | in-progress | `modules/safety/heartbeat_transmitter.*` | Mention packet drop behavior when busy |
| FW-011 | Fault Manager | Latch/clear fault lifecycle | Vincent | P1 | todo | `modules/safety/fault_manager.*` | Add fault-code mapping table |
| FW-012 | Motor Controller | PID/interpolation/diagnostics interfaces | Vincent | P0 | in-progress | `modules/motor_control/motor_controller.*` | Add tuning defaults and constraints |
| FW-013 | Servo Manager | Degrees/PWM API and watchdog behavior | Vincent | P0 | in-progress | `modules/servo_control/servo_manager.*` | Include clamp ranges and startup posture |
| FW-014 | Control Scheduler | 400 Hz loop order and timing stats | Vincent | P0 | in-progress | `modules/motor_control/motor_control_scheduler.*` | Add deterministic timing assumptions |
| FW-015 | TMC2209 Driver | Driver init and communication model | Vincent | P1 | todo | `modules/drivers/tmc2209/*` | Add practical wiring + config notes |
| FW-016 | HAL Layer | GPIO/ADC abstraction contracts | Vincent | P2 | todo | `modules/hal/*` | Keep concise unless APIs expand |
| FW-017 | Telemetry Formatter | Basic/joint payload formatter usage | Vincent | P2 | todo | `modules/telemetry/telemetry_formatter.*` | Add sample payloads |
| FW-018 | Logging Utility | log levels and formatting helpers | Vincent | P2 | todo | `modules/util/logging.h` | Document expected serial prefixes |
| FW-019 | Leader Streamer | ADC sampling and JOINT_STATE generation | Vincent | P0 | in-progress | `leader_esp32/src/main.cpp` | Add calibration guidance for sensors |
| FW-020 | Bridge Forwarding | Leader->Follower forwarding pipeline | Vincent | P0 | in-progress | `tools/teleoperation_bridge.py` | Include startup order and failure modes |
| FW-021 | Bridge Telemetry Intake | Follower telemetry handling/logging | Vincent | P1 | in-progress | `tools/teleoperation_bridge.py` | Clarify direction labels in JSONL |
| FW-022 | Bridge Stream Metrics | Gaps, out-of-order, latency, jitter definitions | Vincent | P0 | in-progress | `tools/teleoperation_bridge.py` | Add interpretation thresholds in docs |
| FW-023 | ROS2 Teleop Integration | End-to-end ROS2 runtime path for teleop | Vincent | P0 | todo | `sixeyes/ros2_ws/src/*` | Major outstanding documentation + implementation area |
| FW-024 | E2E Teleop Testing | Test plan for leader->bridge->follower | Vincent | P0 | todo | `docs/testing`, `tools` | Add automated regression script |

## Immediate Documentation Sprint (Suggested)

1. Close all `P0` rows to `done`
2. Convert `in-progress` sections into protocol + architecture diagrams
3. Add one reproducible test case per `P0` feature
4. Review all references to mode selection (`OPERATION_MODE=1/2`) for consistency
