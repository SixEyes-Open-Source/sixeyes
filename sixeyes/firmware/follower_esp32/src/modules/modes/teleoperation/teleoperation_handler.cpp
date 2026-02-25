/**
 * @file teleoperation_handler.cpp
 * @brief Placeholder for teleoperation mode (Phase 2)
 */

#include "teleoperation_handler.h"
#include "modules/util/logging.h"

void TeleoperationHandler::init() {
  Logging::info("TeleoperationHandler: Teleoperation mode (Phase 2 - not yet implemented)");
  Logging::info("TeleoperationHandler: Awaiting Phase 2 implementation");
}

bool TeleoperationHandler::handleCommand(const char* cmd, const JsonDocument& doc) {
  Logging::warn("TeleoperationHandler: Mode not yet implemented. Command ignored");
  // Note: placeholder - will be replaced with real handler in Phase 2
  return false;
}
