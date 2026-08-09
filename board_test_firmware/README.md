# TongDou V9 Board Test Firmware

This folder is the standalone board bring-up firmware package for TongDou V9
hardware.

It is separate from the full TongDou firmware and from the internal filming
firmware. It should stay boring, stable, and useful for checking whether a newly
assembled board is healthy.

## Scope

Included:

- Hardware initialization
- Board-level diagnostics
- Individual hardware checks
- Serial test commands
- The built-in board-test web page at `http://192.168.4.1/motor`
- V9 pin validation for display, LED, motor, battery, microphone, speaker,
  radar, logo touch, and QMI8658A

Not included:

- Demo scenes
- Personality prompts or AI conversation logic
- Local voice clips or audio assets
- Full expression, light, voice, and motor timelines
- The full TongDou product firmware
- Production calibration flow
- Wi-Fi passwords, tokens, API keys, certificates, or private keys

## Package Layout

```text
board_test_firmware/
|-- README.md
|-- docs/
|   |-- HARDWARE_TESTS.md
|   `-- PUBLIC_RELEASE_CHECKLIST.md
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

`firmware/` is the main board-test firmware. `qmi8658a_test_firmware/` is a
small standalone smoke test for early QMI8658A and basic peripheral bring-up.

## Current V9 Hardware Checks

- OLED display on I2C `IO6` / `IO7`
- SK6812-EC20 LED on `IO9`
- AT8833CT motor driver, including `nSLEEP` and `nFAULT`
- QMI8658A accelerometer and gyroscope
- Logo touch input on `IO4`
- LD2412 radar OUT and serial report parser
- IM72D128V01 PDM microphone
- NS4168 I2S speaker output
- Battery voltage and charging status pins

## Build

```bash
cd board_test_firmware/firmware
platformio run
```

## Upload

The board-test firmware uses the 4 MB flash layout and DIO flash mode.

If normal upload drops the USB serial port near the end of the application
write, use the chunked uploader:

```bash
cd board_test_firmware/firmware
platformio run -t upload_chunked --upload-port COM13
```

Change `COM13` to the actual port shown by Windows Device Manager.

## Test Page

1. Flash this firmware to the V9 board.
2. Power on TongDou.
3. Connect a phone to `TongDou-BoardTest`.
4. Open `http://192.168.4.1/motor`.
5. Use the page for normal assembly checks.

The page includes:

- Quick board checks
- Microphone recording with waveform and WAV download
- Speaker short test tone
- LED color checks
- Motor direction and power checks
- Gyroscope-assisted straight-line check
- Radar live recognition panel
- Log copy button for sharing test results

## Serial Tests

Use the serial console for raw sensor data, parser inspection, and other
advanced diagnostics. Common commands include:

- `status`
- `selftest`
- `battery`
- `mic`
- `audio record`
- `audio record status`
- `speaker`
- `led red`, `led green`, `led blue`, `led off`
- `motor forward`, `motor reverse`, `motor stop`
- `motor auto forward`
- `i2c scan`
- `imu`
- `imu raw test`
- `radar target`
- `radar samples`
- `radar calibrate`
- `radar calibrate status`

See [docs/HARDWARE_TESTS.md](docs/HARDWARE_TESTS.md) for the full current test
list. See [docs/PUBLIC_RELEASE_CHECKLIST.md](docs/PUBLIC_RELEASE_CHECKLIST.md)
before publishing this package elsewhere.

## Audio Notes

- Microphone recording captures a short local sample and provides a WAV download.
- Recording does not automatically play through the speaker.
- Speaker testing uses a generated short test tone only.
- This keeps microphone capture and speaker output separate, so one test does
  not hide problems in the other.

## Radar Notes

- The radar page uses LD2412 serial report parsing as the main source.
- The OUT pin is shown only as a reference signal.
- Use the official HLK tool or serial commands when changing module parameters.

## Notes

- This firmware always starts the board-test access point.
- It does not include demo scenes, local voice clips, ASR, LLM, TTS, MCP, or
  cloud voice code.
- This is not the internal filming firmware.
