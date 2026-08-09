# TongDou V9 Hardware Test Firmware

This folder contains the public hardware bring-up and diagnostic firmware for
TongDou V9 prototype boards.

It is intended for checking whether a newly assembled board is healthy. It is
not the complete TongDou application firmware.

## V9 Diagnostic Update

This version updates the public hardware test firmware for the current TongDou
V9 prototype board.

Main updates:

- Updated V9 pin mapping for OLED, SK6812-EC20, motors, radar, battery,
  capacitive touch Logo, PDM microphone, and I2S speaker output.
- Updated the RGB LED test for the SK6812-EC20 LED used on the V9 board.
- Reworked the microphone test around a normal local recording flow.
- Added microphone waveform display on the board-test web page.
- Added WAV download for recorded microphone samples.
- Kept speaker testing separate from microphone recording, using only a
  generated NS4168 I2S test tone.
- Kept radar diagnostics available through the web page and serial commands.

## What This Firmware Is For

Use this package to test the main V9 board peripherals:

- OLED display on the shared I2C bus
- SK6812-EC20 RGB LED
- IM72D128V01 PDM microphone
- Microphone recording with waveform display and WAV download
- NS4168 I2S speaker test tone
- QMI8658A accelerometer and gyroscope
- LD2412 radar OUT pin and UART report parsing
- Capacitive touch Logo input
- AT8833CT motor direction, PWM, sleep, and fault checks
- Battery voltage, USB presence, charging, and standby status

The firmware starts an open test access point named `TongDou-BoardTest`. After
connecting to it, open:

```text
http://192.168.4.1/motor
```

The web page provides the common assembly checks. Advanced parser, raw sensor,
and module-configuration diagnostics remain available from the serial console.

## What This Firmware Is Not

This package does not include:

- The complete TongDou application firmware
- Demo scenes
- Personality prompts or AI conversation logic
- Official voice or audio assets
- Full expression, light, voice, and motor timelines
- Full behavior choreography
- Production calibration or manufacturing flows
- Wi-Fi passwords, tokens, API keys, certificates, or private keys

## Package Layout

```text
hardware_test_firmware/
|-- README.md
|-- docs/
|   |-- HARDWARE_TESTS.md
|   |-- LD2412_RADAR_TEST.md
|   |-- PUBLIC_RELEASE_CHECKLIST.md
|   `-- SYNC_FROM_BOARD_TEST.md
|-- firmware/
|   |-- platformio.ini
|   |-- include/
|   |-- src/
|   `-- tools/
`-- qmi8658a_test_firmware/
    |-- README.md
    |-- platformio.ini
    `-- src/
```

`firmware/` is the main web-enabled board-test firmware.

`qmi8658a_test_firmware/` is a smaller standalone smoke-test program for early
QMI8658A and basic peripheral bring-up.

## Build

Build the main test firmware:

```bash
cd hardware_test_firmware/firmware
platformio run
```

Build the standalone QMI8658A smoke test:

```bash
cd hardware_test_firmware/qmi8658a_test_firmware
platformio run
```

## Upload

The main test firmware uses the 4 MB flash layout and DIO flash mode.

If the normal upload drops the USB serial port near the end of the application
write, use the chunked uploader:

```bash
cd hardware_test_firmware/firmware
platformio run -t upload_chunked --upload-port COM13
```

Change `COM13` to the actual serial port shown by your system.

## Basic Test Flow

1. Flash the firmware to a TongDou V9 board.
2. Power on the board.
3. Connect a phone or computer to the `TongDou-BoardTest` access point.
4. Open `http://192.168.4.1/motor`.
5. Run the common web checks for display, LED, microphone, speaker, radar,
   motors, IMU, touch Logo, and battery status.
6. Use the serial console for raw sensor data, radar parser checks, and other
   advanced diagnostics.

Common serial commands include:

- `selftest`
- `status`
- `battery`
- `mic`
- `audio record`
- `audio record status`
- `speaker`
- `led red`, `led green`, `led blue`, `led off`
- `motor forward`, `motor reverse`, `motor stop`
- `i2c scan`
- `imu`
- `radar target`
- `radar sample`
- `radar calibrate`
- `radar calibrate status`

See [docs/HARDWARE_TESTS.md](docs/HARDWARE_TESTS.md) for the current detailed
test list.

See [docs/LD2412_RADAR_TEST.md](docs/LD2412_RADAR_TEST.md) for radar diagnostic
notes.

See [docs/PUBLIC_RELEASE_CHECKLIST.md](docs/PUBLIC_RELEASE_CHECKLIST.md) before
publishing this package elsewhere.

## Audio Test Notes

The microphone recording test captures a short local sample and provides a WAV
download from the board-test web page.

Recording does not automatically play through the speaker. Speaker testing uses
a generated test tone only. Keeping capture and playback separate makes it
easier to identify whether a problem is on the microphone side or the speaker
side.

## Radar Test Notes

The radar panel uses the LD2412 UART report as the main diagnostic source. The
OUT pin is shown only as a wiring and module-output reference.

When replacing the radar module or changing its mounting direction, run
empty-scene background calibration again before judging target detection.

## Notes

- This firmware always starts the board-test access point.
- It does not contain local audio files.
- It does not contain cloud voice, ASR, LLM, TTS, or MCP application code.
- It is not the internal filming firmware.
