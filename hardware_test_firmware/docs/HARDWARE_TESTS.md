# TongDou V9 Hardware Tests

This document lists the tests that already exist in the board-test firmware.
It is not a wish list. Do not add entries here before code exists.

## Main Board-Test Firmware

Path: `hardware_test_firmware/firmware`

Build system: PlatformIO, Arduino framework, ESP32-S3 target.

### Serial Commands

- `selftest`: prints the basic board health report.
- `status`: prints ready/failed status for display, LED, microphone, speaker, and web diagnostics.
- `battery`: reads USB presence, charging, standby, and raw battery ADC state.
- `radar`: reads the LD2412 OUT pin state.
- `radar target`: prints parsed LD2412 serial target data.
- `radar sample`: prints live radar sample counters.
- `radar parser selftest`: checks the LD2412 frame parser with known sample frames.
- `radar guided test`: starts a guided empty/target radar check.
- `radar guided status`: prints guided radar check progress.
- `radar guided blocking`: runs the guided radar check as a blocking serial test.
- `radar config`: prints current LD2412 configuration.
- `radar desk`: applies the current desk-mode LD2412 configuration.
- `radar near`: applies the current near-field LD2412 configuration.
- `radar resolution`: reads LD2412 distance resolution.
- `radar res20`: sets LD2412 distance resolution to 20 cm.
- `radar calibrate`: starts LD2412 empty-scene background calibration.
- `radar calibrate status`: reads LD2412 empty-scene calibration status.
- `radar bridge`: bridges LD2412 serial traffic for external inspection.
- `mic`: samples the PDM microphone and prints level statistics.
- `i2c scan`: scans the OLED and QMI8658A shared I2C bus.
- `imu`: prints QMI8658A acceleration and gyroscope data.
- `imu raw test`: prints a short QMI8658A raw-data motion report.
- `speaker`: plays a generated I2S test tone through NS4168.
- `audio volume [0-100]`: sets the generated test-tone volume.
- `led red`: turns the WS2812 LED red.
- `led green`: turns the WS2812 LED green.
- `led blue`: turns the WS2812 LED blue.
- `led off`: turns the WS2812 LED off.
- `motor forward`: runs a short forward pulse on both motors.
- `motor reverse`: runs a short reverse pulse on both motors.
- `motor stop`: stops both motors.
- `motor manual forward [leftPwm rightPwm]`: keeps both motors moving forward briefly.
- `motor manual reverse [leftPwm rightPwm]`: keeps both motors moving reverse briefly.

### Web Page

The firmware starts an open board-test access point named `TongDou-BoardTest`.
After connecting to it, open:

```text
http://192.168.4.1/motor
```

The page exposes the same kind of checks as the serial commands: status,
battery, microphone, speaker, I2C, IMU, radar, LED, and motor tests.

The radar panel exposes `/api/radar`, live target / no-target status, motion
target status, nearest reported distance, bad frame count, guided testing, and
empty-scene background calibration.

See [LD2412_RADAR_TEST.md](LD2412_RADAR_TEST.md) for the real-board radar test
record and calibration notes.

### Logo Touch

Logo touch input on IO4 is checked at runtime:

- Single tap: OLED and LED feedback.
- Double tap: OLED and LED feedback.
- Long press: toggles a simple sleep/wake visual state.

The public board-test firmware does not attach Logo touch to scenes,
personality behavior, voice playback, or full motor choreography.

## Standalone QMI8658A Smoke Test

Path: `hardware_test_firmware/qmi8658a_test_firmware`

Build system: PlatformIO, Arduino framework, ESP32-S3 target.

This is a short single-file bring-up program. It checks:

- QMI8658A on I2C IO5/IO6.
- WS2812 LED on IO9.
- PDM microphone on IO1/IO2.
- NS4168 amplifier control and I2S beep on IO15/IO12/IO13/IO14.
- AT8833CT motor wake and short output pulses on IO42/IO38/IO39/IO40/IO41.
- Battery voltage, USB detect, charge, and standby signals on IO8/IO17/IO35/IO48.

## Explicitly Out Of Scope

The board-test package must not include:

- Full TongDou scenes.
- Personality prompts or AI conversation logic.
- Local audio files.
- Full expression, light, voice, and motor timelines.
- Formal product firmware features.
- Gyro closed-loop straight-line correction.
- Automatic motor learning algorithms.
- Production calibration procedures.
