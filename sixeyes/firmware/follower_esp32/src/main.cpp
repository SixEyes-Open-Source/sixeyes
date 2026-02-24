// SixEyes follower ESP32 - main entry (stub)
// Responsibilities: initialize RTOS tasks and hardware modules.

#include <Arduino.h>

void setup() {
  // TODO: initialize clocks, pins, USB-CDC, UART, TMC drivers
  // Motors must remain disabled by default until safety task enables them.
  Serial.begin(115200);
  delay(100);
  Serial.println("SixEyes follower_esp32 starting (stub)");
}

void loop() {
  // Main loop left empty: real work runs in RTOS tasks.
  delay(1000);
}
