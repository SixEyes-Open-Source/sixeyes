# camera_esp32s3 Firmware (Bring-up)

Purpose: dedicated firmware target for the ESP32-S3-CAM module connected via USB hub on the follower PCB.

## Why this exists

Per system architecture, the camera MCU is a separate USB device from follower control firmware.
That means camera behavior should live in its own firmware project, not in `follower_esp32`.

## Current status

- Buildable PlatformIO target (`camera_esp32s3`)
- Basic USB-serial status heartbeat (`CAM_STATUS` JSON)
- Camera sensor init/streaming still TODO (requires final pin map + selected transport)

## Build

```bash
cd sixeyes/firmware/camera_esp32s3
pio run -e camera_esp32s3
```

## Next implementation steps

1. Add exact ESP32-S3-CAM board pin configuration
2. Initialize camera sensor and validate frame capture
3. Choose transport:
   - USB-CDC JPEG frames (simple)
   - USB-UVC stream (matches technical target)
4. Add timestamped frame metadata for dataset alignment
