# TongDou V9 Hardware Tests

This document lists the tests that already exist in the board-test firmware.
It is not a wish list. Do not add entries here before code exists.

## Main Board-Test Firmware

Path: `hardware_test_firmware/firmware`

Build system: PlatformIO, Arduino framework, ESP32-S3 target.

### Serial Commands

- `selftest`: prints the basic board health report.
- `status`: prints ready/failed status for display, LED, microphone, speaker, and web diagnostics.
- `battery`: reads USB presence, charging, standby, raw battery ADC, estimated
  battery voltage, and estimated battery percent.
- `radar`: reads the LD2412 OUT pin state.
- `radar target`: prints parsed LD2412 serial target data.
- `radar sample`: collects three seconds without printing during capture, then
  prints target-state, distance, energy, valid-frame, and invalid-frame totals.
- `radar parser selftest`: checks the LD2412 frame parser with known sample frames.
- `radar guided test`: starts a guided empty/target radar check.
- `radar guided status`: prints guided radar check progress.
- `radar guided blocking`: runs the guided radar check as a blocking serial test.
- `radar config`: prints current LD2412 configuration.
- `radar sensitivity`: prints all 14 moving and still gate thresholds.
- `radar engineering on|off`: enables or disables per-gate engineering data.
- `radar desk`: applies the current desk-mode LD2412 configuration.
- `radar near`: applies the current near-field LD2412 configuration.
- `radar resolution`: reads LD2412 distance resolution.
- `radar res20`: sets LD2412 distance resolution to 20 cm.
- `radar calibrate`: starts LD2412 module calibration.
- `radar calibrate status`: prints LD2412 calibration status.
- `radar bridge`: bridges LD2412 serial traffic for external inspection.
- `mic`: samples the PDM microphone and prints level statistics.
- `i2c scan`: scans the OLED and QMI8658A shared I2C bus.
- `imu`: prints QMI8658A acceleration and gyroscope data.
- `imu raw test`: prints a short QMI8658A raw-data motion report.
- `speaker`: plays a generated I2S test tone through NS4168.
- `audio volume [0-100]`: sets the generated test-tone volume.
- `led red`: turns the SK6812-EC20 LED red.
- `led green`: turns the SK6812-EC20 LED green.
- `led blue`: turns the SK6812-EC20 LED blue.
- `led off`: turns the SK6812-EC20 LED off.
- `motor forward`: runs a short forward pulse on both motors.
- `motor reverse`: runs a short reverse pulse on both motors.
- `motor stop`: stops both motors.
- `motor manual forward [leftPwm rightPwm]`: keeps both motors moving forward briefly.
- `motor manual reverse [leftPwm rightPwm]`: keeps both motors moving reverse briefly.
- `shipping discharge start`: with wheels removed and USB unplugged, runs the
  motors as a local load until estimated battery charge reaches 30%.
- `shipping discharge status`: prints the current discharge state.
- `shipping discharge stop`: stops shipping discharge immediately.

### Web Page

The firmware starts an open board-test access point named `TongDou-BoardTest`.
After connecting to it, open:

```text
http://192.168.4.1/motor
```

The page is the normal board-test workbench. It keeps the repeatable checks
needed during assembly: board status, battery, microphone, speaker, I2C, IMU,
radar presence, LED, motor direction, motor power, gyro straight-line test,
and speaker volume.

The shipping discharge panel is for shipment preparation. It requires wheels to
be removed, refuses to start while USB is present, stops at 30% estimated charge,
and plays a three-tone speaker alarm when the target is reached.

The status area shows estimated battery percent and voltage. The OLED also
refreshes a battery status screen so a board can be checked without opening the
web page. The percent is estimated from the `100k/100k` battery divider and is
not a calibrated production fuel gauge.

Advanced and temporary diagnostics remain available from the serial console
or maintenance areas when needed. These include the IMU raw test, radar parser
inspection, guided radar experiments, radar parameter presets, motor
calibration, gyro straight-line testing, audio loopback, touch watch, and
low-level LED GPIO checks.

The radar panel uses the LD2412 serial report as its primary source. The OUT
pin is shown only as a wiring and module-output reference; it is not used as
the accuracy result. Presence follows the module target-state byte exactly:
zero means clear and a non-zero target state means occupied. Per-gate energy
is diagnostic data only and never creates a host-side presence result.

The web radar panel shows only `Target detected`, `No target`, or
`No serial frame`. Live polling is intentionally silent on the USB serial
console; use `radar sample` when a detailed serial report is needed.

The V1.26 module firmware observed during board testing keeps the `0x55`
marker before the final report byte, but that final byte is not always
`0x00` in moving-target engineering frames. The parser validates the report
header, declared length, `0x55` marker, and report footer without requiring a
fixed final report byte.

For a meaningful test, install the radar in its final fixed direction, keep
the sensing area clear, and run `radar calibrate`. Do not rotate the radar
between the empty and occupied phases; move the person into and out of the
fixed sensing area instead. After calibration completes, allow the module to
settle for 10 seconds, then use `radar sample` for one empty capture and one
30-to-100-cm moving-person capture. A passing capture has complete frames,
zero invalid-frame growth, a clear target state when empty, and moving or
combined target states when occupied.

Do not run the Hi-Link Bluetooth application and UART configuration commands
at the same time. Use one controller at a time when changing parameters or
running background calibration.

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

- QMI8658A on I2C IO6/IO7.
- SK6812-EC20 LED on IO9.
- PDM microphone on IO1/IO2.
- NS4168 amplifier control and I2S beep on IO15/IO12/IO13/IO14.
- AT8833CT motor wake and short output pulses on IO42/IO38/IO39/IO40/IO41.
- AT8833CT nFAULT input on IO37.
- LD2412 radar OUT/TX/RX on IO5/IO10/IO11.
- Battery voltage, USB detect, charge, and standby signals on IO8/IO17/IO35/IO48.

## Public Boundary

This package is the real TongDou V9 engineering board-test firmware. It may
include risky maintenance and calibration functions when they are useful for
bring-up, diagnostics, repair, or shipment preparation.

The board-test package must still not include:

- Full TongDou scenes.
- Personality prompts or AI conversation logic.
- Local audio files.
- Full expression, light, voice, and motor timelines.
- Formal product firmware features.

High-risk maintenance functions must be documented clearly instead of being
hidden. These include persistent motor configuration, motor calibration,
gyro-assisted straight-line tests, radar parameter changes, radar factory
reset, and shipping discharge.
