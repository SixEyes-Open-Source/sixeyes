/**
 * @file teleoperation_handler.cpp
 * @brief Teleoperation mode command router
 */

#include <Arduino.h>
#include <array>
#include "teleoperation_handler.h"
#include "modules/util/logging.h"
#include "modules/motor_control/motor_controller.h"
#include "modules/servo_control/servo_manager.h"

namespace {
constexpr size_t TELEOP_JOINT_COUNT = 6;

float clampDegrees(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 180.0f) return 180.0f;
  return value;
}

bool isJointValid(const JsonArrayConst& valid_mask, size_t index) {
  if (index >= valid_mask.size()) {
    return true;
  }

  const JsonVariantConst value = valid_mask[index];
  if (value.is<uint8_t>()) {
    return value.as<uint8_t>() != 0;
  }
  if (value.is<bool>()) {
    return value.as<bool>();
  }
  return true;
}

void applyJointState(const JsonArrayConst& in_joints, const JsonArrayConst& valid_mask) {
  std::array<float, NUM_STEPPERS> motor_targets = MotorController::instance().getCurrentPositions();
  std::array<float, NUM_SERVOS> servo_targets = ServoManager::instance().getPositions();

  for (size_t index = 0; index < NUM_STEPPERS && index < in_joints.size(); ++index) {
    if (!isJointValid(valid_mask, index)) {
      continue;
    }
    if (in_joints[index].is<float>()) {
      motor_targets[index] = in_joints[index].as<float>();
    }
  }

  for (size_t servo_index = 0; servo_index < NUM_SERVOS; ++servo_index) {
    const size_t source_index = NUM_STEPPERS + servo_index;
    if (source_index >= in_joints.size()) {
      continue;
    }
    if (!isJointValid(valid_mask, source_index)) {
      continue;
    }
    if (in_joints[source_index].is<float>()) {
      const float requested = in_joints[source_index].as<float>();
      servo_targets[servo_index] = clampDegrees(requested);
    }
  }

  MotorController::instance().setAbsoluteTargets(motor_targets);
  ServoManager::instance().setPositions(servo_targets);
}

void sendTelemetryState(const JsonDocument& source_doc) {
  JsonDocument telemetry;
  telemetry["cmd"] = "TELEMETRY_STATE";
  telemetry["seq"] = source_doc["seq"].is<uint32_t>() ? source_doc["seq"].as<uint32_t>() : 0;
  telemetry["ts"] = millis();

  JsonArray follower_joints = telemetry["follower_joints"].to<JsonArray>();
  JsonArray errors = telemetry["errors"].to<JsonArray>();
  JsonArray faults = telemetry["faults"].to<JsonArray>();

  const std::array<float, NUM_STEPPERS> current_motors = MotorController::instance().getCurrentPositions();
  const std::array<float, NUM_STEPPERS> motor_errors = MotorController::instance().getErrors();
  const std::array<float, NUM_SERVOS> current_servos = ServoManager::instance().getPositions();

  for (size_t index = 0; index < NUM_STEPPERS && index < TELEOP_JOINT_COUNT; ++index) {
    follower_joints.add(current_motors[index]);
    errors.add(motor_errors[index]);
    faults.add(0);
  }

  for (size_t servo_index = 0; servo_index < NUM_SERVOS; ++servo_index) {
    if ((NUM_STEPPERS + servo_index) >= TELEOP_JOINT_COUNT) {
      break;
    }
    follower_joints.add(current_servos[servo_index]);
    errors.add(0.0f);
    faults.add(0);
  }

  telemetry["motors_en"] = MotorController::instance().motorsEnabled() ? 1 : 0;

  serializeJson(telemetry, Serial);
  Serial.println();
}

bool handleJointState(const JsonDocument& doc) {
  if (!doc["leader_joints"].is<JsonArray>()) {
    Logging::warn("TeleoperationHandler: JOINT_STATE missing leader_joints array");
    return false;
  }

  const JsonArrayConst in_joints = doc["leader_joints"].as<JsonArrayConst>();
  const JsonArrayConst valid_mask = doc["valid_mask"].is<JsonArray>()
      ? doc["valid_mask"].as<JsonArrayConst>()
      : JsonArrayConst();

  applyJointState(in_joints, valid_mask);
  sendTelemetryState(doc);
  return true;
}
} // namespace

void TeleoperationHandler::init() {
  Logging::info("TeleoperationHandler: Teleoperation mode initialized");
  Logging::info("TeleoperationHandler: JOINT_STATE mapping + TELEMETRY_STATE enabled");
}

bool TeleoperationHandler::handleCommand(const char* cmd, const JsonDocument& doc) {
  if (!cmd) {
    Logging::warn("TeleoperationHandler: Null command ignored");
    return false;
  }

  if (strcmp(cmd, "JOINT_STATE") == 0) {
    return handleJointState(doc);
  }

  Logging::warnf("TeleoperationHandler: Unsupported command in teleoperation mode: %s", cmd);
  return false;
}
