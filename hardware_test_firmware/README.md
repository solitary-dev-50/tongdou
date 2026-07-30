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
- The built-in board-test web page

Not included:

- Demo scenes
- Personality prompts or AI conversation logic
- Local voice clips or audio assets
- Full expression, light, voice, and motor timelines
- The full TongDou product firmware
- Gyro closed-loop straight-line correction or auto-learning algorithms
- Production calibration flow
- Wi-Fi passwords, tokens, API keys, certificates, or private keys

## Package Layout

```text
hardware_test_firmware/
├── README.md
├── docs/
│   ├── HARDWARE_TESTS.md
│   ├── LD2412_RADAR_TEST.md
│   └── PUBLIC_RELEASE_CHECKLIST.md
├── firmware/
│   ├── platformio.ini
│   ├── include/
│   ├── src/
│   └── tools/
└── qmi8658a_test_firmware/
    ├── README.md
    ├── platformio.ini
    └── src/
```

`firmware/` is the main board-test firmware. `qmi8658a_test_firmware/` is a
small standalone smoke test for early board bring-up.

## Build

```bash
cd hardware_test_firmware/firmware
platformio run
```

## Upload

The board-test firmware uses the 4 MB flash layout and DIO flash mode.

If normal upload drops the USB serial port near the end of the application
write, use the chunked uploader:

```bash
cd hardware_test_firmware/firmware
platformio run -t upload_chunked --upload-port <PORT>
```

## Test

1. Flash this firmware to the V9 board.
2. Power on TongDou.
3. Connect phone or computer to `TongDou-BoardTest`.
4. Open `http://192.168.4.1/motor`.
5. Use the page or serial commands such as `selftest`, `battery`, `mic`,
   `speaker`, `imu`, `i2c scan`, `radar`, `motor forward`,
   `motor reverse`, and `motor manual forward`.

See [docs/HARDWARE_TESTS.md](docs/HARDWARE_TESTS.md) for the current test list.
See [docs/LD2412_RADAR_TEST.md](docs/LD2412_RADAR_TEST.md) for the LD2412 radar
diagnostic result and empty-scene background calibration notes.
See [docs/PUBLIC_RELEASE_CHECKLIST.md](docs/PUBLIC_RELEASE_CHECKLIST.md) before
publishing this package elsewhere.

## Notes

- This firmware always starts the board-test access point.
- It does not include demo scenes, local voice clips, ASR, LLM, TTS, MCP, or cloud voice code.
- Speaker testing uses a generated test tone only.
- This is not the internal filming firmware.
