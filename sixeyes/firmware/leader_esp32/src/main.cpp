// Leader ESP32 teleoperation streamer
// Role: sample leader-arm joint inputs and publish JOINT_STATE at 100 Hz.

#include <Arduino.h>
#include <ArduinoJson.h>

namespace {
constexpr uint32_t BAUD_RATE = 115200;
constexpr uint32_t STREAM_HZ = 100;
constexpr uint32_t STREAM_PERIOD_MS = 1000 / STREAM_HZ;
constexpr size_t NUM_JOINTS = 6;
constexpr uint16_t ADC_MAX = 4095;

// ESP32 DevKit ADC pins for 6 joint channels.
constexpr uint8_t JOINT_ADC_PINS[NUM_JOINTS] = {32, 33, 34, 35, 36, 39};

uint32_t sequence = 0;
uint32_t last_stream_ms = 0;

float adcToDegrees(uint16_t raw) {
  const float ratio = static_cast<float>(raw) / static_cast<float>(ADC_MAX);
  return ratio * 180.0f;
}

bool readJointDegrees(float out_degrees[NUM_JOINTS], uint8_t out_valid[NUM_JOINTS]) {
  bool all_valid = true;

  for (size_t i = 0; i < NUM_JOINTS; ++i) {
    const int raw = analogRead(JOINT_ADC_PINS[i]);
    if (raw < 0 || raw > ADC_MAX) {
      out_degrees[i] = 0.0f;
      out_valid[i] = 0;
      all_valid = false;
      continue;
    }

    out_degrees[i] = adcToDegrees(static_cast<uint16_t>(raw));
    out_valid[i] = 1;
  }

  return all_valid;
}

void publishJointState() {
  float joint_degrees[NUM_JOINTS] = {};
  uint8_t valid_mask[NUM_JOINTS] = {};
  readJointDegrees(joint_degrees, valid_mask);

  StaticJsonDocument<512> doc;
  doc["cmd"] = "JOINT_STATE";
  doc["seq"] = sequence++;
  doc["ts"] = millis();

  JsonArray joints = doc.createNestedArray("leader_joints");
  JsonArray valid = doc.createNestedArray("valid_mask");

  for (size_t i = 0; i < NUM_JOINTS; ++i) {
    joints.add(joint_degrees[i]);
    valid.add(valid_mask[i]);
  }

  serializeJson(doc, Serial);
  Serial.println();
}
} // namespace

void setup() {
  Serial.begin(BAUD_RATE);
  delay(100);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  for (size_t i = 0; i < NUM_JOINTS; ++i) {
    pinMode(JOINT_ADC_PINS[i], INPUT);
  }

  Serial.println("SixEyes leader_esp32 starting (teleoperation mode)");
  Serial.println("Streaming JOINT_STATE at 100 Hz");
}

void loop() {
  const uint32_t now = millis();
  if ((now - last_stream_ms) >= STREAM_PERIOD_MS) {
    last_stream_ms = now;
    publishJointState();
  }
}
