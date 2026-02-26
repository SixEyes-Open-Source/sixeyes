/**
 * @file teleoperation_handler.cpp
 * @brief Teleoperation mode command router
 */

#include "teleoperation_handler.h"
#include "modules/motor_control/motor_controller.h"
#include "modules/safety/fault_manager.h"
#include "modules/servo_control/servo_manager.h"
#include "modules/util/logging.h"
#include <Arduino.h>
#include <array>

namespace {
constexpr size_t TELEOP_INPUT_COUNT = 6;

constexpr size_t JOINT_BASE = 0;
constexpr size_t JOINT_SHOULDER = 1;
constexpr size_t JOINT_ELBOW = 2;
constexpr size_t JOINT_WRIST_PITCH = 3;
constexpr size_t JOINT_WRIST_YAW = 4;
constexpr size_t JOINT_GRIPPER = 5;

constexpr size_t MOTOR_BASE = 0;
constexpr size_t MOTOR_SHOULDER_A = 1;
constexpr size_t MOTOR_SHOULDER_B = 2;
constexpr size_t MOTOR_ELBOW = 3;

constexpr float STEPPER_GEAR_RATIO = 25.0f;

float clampDegrees(float value) {
  if (value < 0.0f)
    return 0.0f;
  if (value > 180.0f)
    return 180.0f;
  return value;
}

bool isJointValid(const JsonArrayConst &valid_mask, size_t index) {
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

void applyJointState(const JsonArrayConst &in_joints,
                     const JsonArrayConst &valid_mask) {
  std::array<float, NUM_STEPPERS> motor_targets =
      MotorController::instance().getCurrentPositions();
  std::array<float, NUM_SERVOS> servo_targets =
      ServoManager::instance().getPositions();

  // --- Stepper mapping (6 teleop joints -> 4 motors) ---
  // Joint angles are absolute follower-joint angles in degrees.
  // NEMA17 motors drive 1:25 cycloidal stages, so convert to motor-shaft
  // degrees.
  if (JOINT_BASE < in_joints.size() && isJointValid(valid_mask, JOINT_BASE) &&
      in_joints[JOINT_BASE].is<float>()) {
    motor_targets[MOTOR_BASE] =
        in_joints[JOINT_BASE].as<float>() * STEPPER_GEAR_RATIO;
  }

  if (JOINT_SHOULDER < in_joints.size() &&
      isJointValid(valid_mask, JOINT_SHOULDER) &&
      in_joints[JOINT_SHOULDER].is<float>()) {
    const float shoulder_motor_deg =
        in_joints[JOINT_SHOULDER].as<float>() * STEPPER_GEAR_RATIO;
    // Dual shoulder motors are mirrored mechanically (face-to-face): equal
    // magnitude, opposite sign.
    motor_targets[MOTOR_SHOULDER_A] = shoulder_motor_deg;
    motor_targets[MOTOR_SHOULDER_B] = -shoulder_motor_deg;
  }

  if (JOINT_ELBOW < in_joints.size() && isJointValid(valid_mask, JOINT_ELBOW) &&
      in_joints[JOINT_ELBOW].is<float>()) {
    motor_targets[MOTOR_ELBOW] =
        in_joints[JOINT_ELBOW].as<float>() * STEPPER_GEAR_RATIO;
  }

  // --- Servo mapping (wrist pitch/yaw + gripper) ---
  if (JOINT_WRIST_PITCH < in_joints.size() &&
      isJointValid(valid_mask, JOINT_WRIST_PITCH) &&
      in_joints[JOINT_WRIST_PITCH].is<float>()) {
    servo_targets[0] = clampDegrees(in_joints[JOINT_WRIST_PITCH].as<float>());
  }
  if (JOINT_WRIST_YAW < in_joints.size() &&
      isJointValid(valid_mask, JOINT_WRIST_YAW) &&
      in_joints[JOINT_WRIST_YAW].is<float>()) {
    servo_targets[1] = clampDegrees(in_joints[JOINT_WRIST_YAW].as<float>());
  }
  if (JOINT_GRIPPER < in_joints.size() &&
      isJointValid(valid_mask, JOINT_GRIPPER) &&
      in_joints[JOINT_GRIPPER].is<float>()) {
    servo_targets[2] = clampDegrees(in_joints[JOINT_GRIPPER].as<float>());
  }

  MotorController::instance().setAbsoluteTargets(motor_targets);
  ServoManager::instance().setPositions(servo_targets);
}

void sendTelemetryState(const JsonDocument &source_doc) {
  JsonDocument telemetry;
  telemetry["cmd"] = "TELEMETRY_STATE";
  telemetry["seq"] =
      source_doc["seq"].is<uint32_t>() ? source_doc["seq"].as<uint32_t>() : 0;
  telemetry["ts"] = millis();

  JsonArray follower_joints = telemetry["follower_joints"].to<JsonArray>();
  JsonArray errors = telemetry["errors"].to<JsonArray>();
  JsonArray faults = telemetry["faults"].to<JsonArray>();

  const std::array<float, NUM_STEPPERS> current_motors =
      MotorController::instance().getCurrentPositions();
  const std::array<float, NUM_STEPPERS> motor_errors =
      MotorController::instance().getErrors();
  const std::array<float, NUM_SERVOS> current_servos =
      ServoManager::instance().getPositions();

  // Report follower joints in teleop-joint space (6 values), not raw 7-actuator
  // space.
  const float base_joint_deg = current_motors[MOTOR_BASE] / STEPPER_GEAR_RATIO;
  const float shoulder_joint_deg =
      ((current_motors[MOTOR_SHOULDER_A] - current_motors[MOTOR_SHOULDER_B]) *
       0.5f) /
      STEPPER_GEAR_RATIO;
  const float elbow_joint_deg =
      current_motors[MOTOR_ELBOW] / STEPPER_GEAR_RATIO;

  follower_joints.add(base_joint_deg);
  errors.add(motor_errors[MOTOR_BASE] / STEPPER_GEAR_RATIO);
  faults.add(0);

  follower_joints.add(shoulder_joint_deg);
  errors.add((motor_errors[MOTOR_SHOULDER_A] - motor_errors[MOTOR_SHOULDER_B]) *
             0.5f / STEPPER_GEAR_RATIO);
  faults.add(0);

  follower_joints.add(elbow_joint_deg);
  errors.add(motor_errors[MOTOR_ELBOW] / STEPPER_GEAR_RATIO);
  faults.add(0);

  follower_joints.add(current_servos[0]);
  errors.add(0.0f);
  faults.add(0);

  follower_joints.add(current_servos[1]);
  errors.add(0.0f);
  faults.add(0);

  follower_joints.add(current_servos[2]);
  errors.add(0.0f);
  faults.add(0);

  telemetry["motors_en"] = MotorController::instance().motorsEnabled() ? 1 : 0;

  serializeJson(telemetry, Serial);
  Serial.println();
}

bool handleJointState(const JsonDocument &doc) {
  if (!doc["leader_joints"].is<JsonArray>()) {
    Logging::warn(
        "TeleoperationHandler: JOINT_STATE missing leader_joints array");
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
  Logging::info(
      "TeleoperationHandler: JOINT_STATE mapping + TELEMETRY_STATE enabled");
}

bool TeleoperationHandler::handleCommand(const char *cmd,
                                         const JsonDocument &doc) {
  (void)doc;

  if (!cmd) {
    Logging::warn("TeleoperationHandler: Null command ignored");
    return false;
  }

  if (strcmp(cmd, "JOINT_STATE") == 0) {
    return handleJointState(doc);
  }

  if (strcmp(cmd, "HOME_ZERO") == 0) {
    MotorController::instance().setCurrentPositionAsZero();
    Logging::info("TeleoperationHandler: HOME_ZERO applied");
    return true;
  }

  if (strcmp(cmd, "HOME_STALLGUARD") == 0) {
    const uint8_t motor_mask = doc["motor_mask"].is<uint8_t>()
                                   ? doc["motor_mask"].as<uint8_t>()
                                   : 0x0F;
    const uint8_t sensitivity = doc["sensitivity"].is<uint8_t>()
                                    ? doc["sensitivity"].as<uint8_t>()
                                    : TMC2209_SGTHRS_DEFAULT;
    if (!MotorController::instance().startStallGuardHoming(motor_mask,
                                                           sensitivity)) {
      Logging::warn("TeleoperationHandler: HOME_STALLGUARD failed to start");
      return false;
    }
    Logging::infof("TeleoperationHandler: HOME_STALLGUARD started mask=0x%02X "
                   "sensitivity=%u",
                   motor_mask, sensitivity);
    return true;
  }

  if (strcmp(cmd, "ENABLE_MOTORS") == 0) {
    const bool enable =
        doc["enable"].is<bool>() ? doc["enable"].as<bool>() : false;
    if (enable) {
      MotorController::instance().enableMotors();
      Logging::info("TeleoperationHandler: ENABLE_MOTORS=true");
    } else {
      MotorController::instance().disableMotors();
      Logging::info("TeleoperationHandler: ENABLE_MOTORS=false");
    }
    return true;
  }

  if (strcmp(cmd, "RESET_FAULT") == 0) {
    FaultManager::instance().clear();
    Logging::info("TeleoperationHandler: RESET_FAULT applied");
    return true;
  }

  Logging::warnf(
      "TeleoperationHandler: Unsupported command in teleoperation mode: %s",
      cmd);
  return false;
}
