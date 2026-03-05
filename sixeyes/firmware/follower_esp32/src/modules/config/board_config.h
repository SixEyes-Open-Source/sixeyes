// Board and system configuration constants
#pragma once

// Safety defaults
static constexpr unsigned long SAFETY_HEARTBEAT_TIMEOUT_MS =
    500; // default ~500 ms

// Control loop frequency (Hz) - choose between 200 and 500
static constexpr int CONTROL_LOOP_HZ = 400;

// Motor/driver counts
static constexpr int NUM_STEPPERS = 4;
static constexpr int NUM_SERVOS = 3;

// Leader-Follower dedicated UART pins (Follower side)
// Per "SixEyes Technical Reference 2":
//   RX (Leader TX) -> GPIO38
//   TX (Leader RX) -> GPIO39
static constexpr int LEADER_UART_RX_PIN = 38;
static constexpr int LEADER_UART_TX_PIN = 39;
