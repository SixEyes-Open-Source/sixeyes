#include <Arduino.h>
#include <ArduinoJson.h>

namespace {
constexpr uint32_t BAUD_RATE = 115200;
constexpr uint32_t STATUS_HZ = 2;
constexpr uint32_t STATUS_PERIOD_MS = 1000 / STATUS_HZ;

uint32_t last_status_ms = 0;
uint32_t frame_seq = 0;
bool camera_ready = false;

bool initCameraHardware() {
  // TODO: Replace with full ESP32-S3-CAM pin map + sensor init.
  // Keeping this bring-up safe and deterministic until hardware pinout is finalized.
  return true;
}

void publishStatus() {
  StaticJsonDocument<256> doc;
  doc["cmd"] = "CAM_STATUS";
  doc["ts"] = millis();
  doc["camera_ready"] = camera_ready ? 1 : 0;
  doc["frame_seq"] = frame_seq;
  doc["streaming"] = 0;  // TODO: set to 1 once frame streaming path is implemented

  serializeJson(doc, Serial);
  Serial.println();
}
} // namespace

void setup() {
  Serial.begin(BAUD_RATE);
  delay(200);

  camera_ready = initCameraHardware();

  Serial.println("SixEyes camera_esp32s3 firmware starting");
  Serial.printf("Version: %s\n", CAMERA_FIRMWARE_VERSION);
  Serial.println(camera_ready ? "Camera init: OK" : "Camera init: FAILED");
}

void loop() {
  const uint32_t now = millis();
  if ((now - last_status_ms) >= STATUS_PERIOD_MS) {
    last_status_ms = now;
    publishStatus();
    frame_seq++;
  }
}
