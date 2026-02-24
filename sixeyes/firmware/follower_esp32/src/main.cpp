// SixEyes follower ESP32 - deterministic control loop wiring modules
// Responsibilities: initialize modules and run deterministic control loop at CONTROL_LOOP_HZ

#include <Arduino.h>
#include "modules/hal/gpio.h"
#include "modules/motor_control/motor_controller.h"
#include "modules/drivers/tmc2209/tmc2209_driver.h"
#include "modules/servo_control/servo_manager.h"
#include "modules/safety/heartbeat_monitor.h"
#include "modules/safety/fault_manager.h"
#include "modules/comms/uart_leader/uart_leader.h"
#include "modules/comms/usb_cdc/usb_cdc.h"
#include "modules/telemetry/telemetry_formatter.h"
#include "modules/util/logging.h"
#include "modules/config/board_config.h"
#include <array>

void setup() {
  Serial.begin(115200);
  delay(100);
  Logging::info("SixEyes follower_esp32 starting");

  HAL_GPIO::init(); // ensures motors disabled by default

  // Initialize drivers and modules
  TMC2209Driver::instance().init();
  MotorController::instance().init();
  ServoManager::instance().init();
  TelemetryFormatter::instance().init();
  UartLeader::instance().init(115200);

  HeartbeatMonitor::instance().init(SAFETY_HEARTBEAT_TIMEOUT_MS);
  FaultManager::instance().init();

  Logging::info("Initialization complete; entering control loop");
}

void loop() {
  static const unsigned long period_ms = 1000u / CONTROL_LOOP_HZ;
  static unsigned long next_tick = millis();

  unsigned long now = millis();
  if (now < next_tick) {
    // Not time yet; yield
    delay(0);
    return;
  }

  // Advance to next tick (avoid drift)
  next_tick += period_ms;

  // 1) Safety checks
  HeartbeatMonitor::instance().check();
  if (!HeartbeatMonitor::instance().isHealthy()) {
    FaultManager::instance().raise(FaultManager::Fault::HEARTBEAT_TIMEOUT);
  }

  // 2) Enforce faults / motor enable state
  if (FaultManager::instance().hasFault()) {
    MotorController::instance().disableMotors();
    HAL_GPIO::setMotorEnable(false);
  } else {
    if (!MotorController::instance().motorsEnabled()) {
      MotorController::instance().enableMotors();
      HAL_GPIO::setMotorEnable(true);
    }
  }

  // 3) Receive commands from leader and apply absolute targets
  std::array<float, NUM_STEPPERS> targets{};
  uint32_t seq = 0;
  if (UartLeader::instance().readAbsoluteTargets(targets, seq)) {
    MotorController::instance().setAbsoluteTargets(targets);
    UartLeader::instance().sendAck(seq);
  }

  // 4) Run motor controller deterministic update
  MotorController::instance().update();

  // 5) Publish telemetry over USB-CDC
  char buf[256] = {0};
  TelemetryFormatter::instance().formatBasicTelemetry(buf, sizeof(buf));
  UsbCDC::instance().sendTelemetry(buf);

  // Allow background tasks to run briefly
  delay(0);
}

