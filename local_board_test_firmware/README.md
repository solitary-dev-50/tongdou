# TongDou V9 Board Test Firmware

This folder is for internal V9 board bring-up and empty-load hardware tests.

It is separate from the clean demo firmware so hardware diagnostics do not leak into filming builds.

## Build

```bash
cd local_board_test_firmware/firmware
platformio run
```

## Test

1. Flash this firmware to the V9 board.
2. Power on TongDou.
3. Connect phone to `TongDou-BoardTest`.
4. Open `http://192.168.4.1/motor` for motor and gyro checks.
5. Use serial commands such as `selftest`, `battery`, `mic`, `speaker`, `imu`, and `motor auto forward`.
6. Use `/demo` only as a quick cross-check that demo scenes still run.

## Notes

- This firmware always starts the board-test access point.
- Diagnostic APIs and serial self-test commands are intentionally enabled here.
- Voice clips currently go through the existing clip logging/playback path.
- This is not the clean filming firmware.
