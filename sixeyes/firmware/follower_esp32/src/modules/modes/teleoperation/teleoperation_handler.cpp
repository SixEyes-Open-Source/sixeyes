/**
 * @file teleoperation_handler.cpp
 * @brief Teleoperation mode command router (Phase 3 stub)
 */

#include <Arduino.h>
#include "teleoperation_handler.h"
#include "modules/util/logging.h"

namespace {
void sendTelemetryStateStub(const JsonDocument& doc) {
  StaticJsonDocument<512> telemetry;
  telemetry["cmd"] = "TELEMETRY_STATE";
  telemetry["seq"] = doc["seq"].is<uint32_t>() ? doc["seq"].as<uint32_t>() : 0;
  telemetry["ts"] = millis();

  JsonArray follower_joints = telemetry.createNestedArray("follower_joints");
  JsonArray errors = telemetry.createNestedArray("errors");
  JsonArray faults = telemetry.createNestedArray("faults");

  if (doc["leader_joints"].is<JsonArray>()) {
    JsonArrayConst in_joints = doc["leader_joints"].as<JsonArrayConst>();
    for (JsonVariantConst joint : in_joints) {
      follower_joints.add(joint.is<float>() ? joint.as<float>() : 0.0f);
      errors.add(0.0f);
      faults.add(0);
    }
  }

  serializeJson(telemetry, Serial);
  Serial.println();
}
} // namespace

void TeleoperationHandler::init() {
  Logging::info("TeleoperationHandler: Teleoperation mode initialized (Phase 3 stub)");
  Logging::info("TeleoperationHandler: JOINT_STATE routing + TELEMETRY_STATE stub enabled");
}

bool TeleoperationHandler::handleCommand(const char* cmd, const JsonDocument& doc) {
  if (!cmd) {
    Logging::warn("TeleoperationHandler: Null command ignored");
    return false;
  }

  if (strcmp(cmd, "JOINT_STATE") == 0) {
    sendTelemetryStateStub(doc);
    return true;
  }

  Logging::warnf("TeleoperationHandler: Unsupported command in teleoperation mode: %s", cmd);
  return false;
}
