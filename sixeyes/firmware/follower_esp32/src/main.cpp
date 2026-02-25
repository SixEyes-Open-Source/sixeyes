// SixEyes follower ESP32 - deterministic control loop wiring modules
// Responsibilities: initialize modules and run deterministic control loop at CONTROL_LOOP_HZ
// with ROS2 heartbeat integration for safety-critical control

#include <Arduino.h>
#include "modules/hal/gpio.h"
#include "modules/motor_control/motor_controller.h"
#include "modules/motor_control/motor_control_scheduler.h"
#include "modules/drivers/tmc2209/tmc2209_driver.h"
#include "modules/servo_control/servo_manager.h"
#include "modules/safety/heartbeat_monitor.h"
#include "modules/safety/safety_task.h"
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

  // Initialize safety subsystem with ROS2 heartbeat bridge
  // Pass Serial (USB CDC) for ROS2 heartbeat communication
  SafetyTask::instance().init(&Serial);
  FaultManager::instance().init();

  // Start the FreeRTOS control loop (400 Hz, core 0)
  // This runs all control tasks deterministically:
  // SafetyTask → MotorController → ServoManager → HeartbeatMonitor
  MotorControlScheduler::instance().init();
  MotorControlScheduler::instance().start();

  Logging::info("Initialization complete; control loop running on FreeRTOS");
}

void loop() {
  // Main loop now just yields to FreeRTOS tasks
  // The MotorControlScheduler runs the real control loop at 400 Hz on core 0
  // USB CDC and other background tasks can run on core 1
  delay(100);
}

