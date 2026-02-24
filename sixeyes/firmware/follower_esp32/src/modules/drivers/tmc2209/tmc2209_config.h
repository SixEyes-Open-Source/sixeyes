// TMC2209 driver config: PDN pin assignments (PDN_UART mode)
// Edit these to match your board wiring.
#pragma once
#include <Arduino.h>

// Number of stepper drivers
#ifndef TMC2209_NUM_DRIVERS
#define TMC2209_NUM_DRIVERS 4
#endif

// PDN pins for each driver (active LOW to enable PDN_UART on that device)
// Default pins are placeholders and MUST be updated for your PCB.
static const uint8_t TMC2209_PDN_PINS[TMC2209_NUM_DRIVERS] = { 12, 13, 14, 15 };
// Sense resistor value used by TMC drivers (ohms). Adjust for your board.
#ifndef TMC2209_R_SENSE
#define TMC2209_R_SENSE 0.11f
#endif
