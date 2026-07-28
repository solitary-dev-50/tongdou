# TongDou V9 Open Board Test Firmware

This folder is the public board bring-up firmware for TongDou V9 hardware.

It is separate from the internal filming firmware. It should stay boring, stable,
and useful for checking whether a newly assembled board is healthy.

## Build

```bash
cd board_test_firmware/firmware
platformio run
```

## Test

1. Flash this firmware to the V9 board.
2. Power on TongDou.
3. Connect phone or computer to `TongDou-BoardTest`.
4. Open `http://192.168.4.1/motor`.
5. Use the page or serial commands such as `selftest`, `battery`, `mic`,
   `speaker`, `imu`, `i2c scan`, `radar`, and `motor auto forward`.

## Notes

- This firmware always starts the board-test access point.
- It does not include demo scenes, local voice clips, ASR, LLM, TTS, MCP, or cloud voice code.
- Speaker testing uses a generated test tone only.
- Motor settings are saved in NVS through the motor driver calibration path.
- This is not the clean filming firmware.
