# TongDou Internal Demo Firmware

This folder is for internal filming only. Keep it clean for overseas demo shots.

Do not use it as the public firmware. New generated files, local audio exports,
and build outputs under this folder are ignored by git unless they are
explicitly reviewed and added.

## Build

```bash
cd local_demo_firmware/firmware
platformio run
```

## Test

1. Flash this firmware to the demo board.
2. Power on TongDou.
3. Connect phone to `TongDou-Demo`.
4. Open `http://192.168.4.1/demo`.
5. Press `1` to `8` for overseas campaign scenes.
6. Press `0` to stop and return to idle.

## Notes

- This firmware always starts the demo access point.
- Board-level test pages and diagnostic APIs live in `../board_test_firmware/firmware`.
- Voice clips use local pre-recorded audio files under `firmware/data/audio`.
- Scene `5` uses a recorded email-summary voice line for faster filming.
- Scene `8` plays `first_summon_nightmare`.
- Serial `summon` or `nightmare` triggers scene `8`.
- Serial `reset_first_summon` clears the first-start marker.
