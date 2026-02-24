// Leader ESP32 stub
// Role: validate/relay absolute joint targets from laptop to follower over UART.

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("SixEyes leader_esp32 starting (stub)");
}

void loop() {
  // Leader logic to be implemented: accept commands from laptop, forward to follower.
  delay(1000);
}
