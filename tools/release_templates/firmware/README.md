# TongDou Application Firmware v0.1.2 Early Tester

Precompiled firmware for TongDou V9, ESP32-S3-MINI-1-N4R2, 4 MB flash. Use with TongDou Server v0.1.2.

## Install

Extract the whole package, connect a USB data cable, close serial monitors, and run `FLASH_TONGDOU.bat`. Confirm the fresh installation. The script prioritises USB serial ports; identify TongDou by disconnecting/reconnecting it if necessary. A single USB serial port is not proof that it is TongDou.

The complete `TongDou_Application_Firmware_v0.1.2.bin` is written at `0x0` and replaces all 4 MB. Saved Wi-Fi, server address, language, volume, reminders and motor direction/power settings are cleared. Hardware MAC/device identity is unchanged. Migration from v0.1.0 requires this full USB installation because the application partition moved to `0x1D0000`.

After flashing, join `TongDou-Setup`, open `http://192.168.4.1/`, and enter the server address printed by its launcher (normally `http://<server-PC-LAN-IP>:8102`). Bind the activation code in the server dashboard. Recheck motor settings after a fresh install.

## Contents

- Complete 4 MB USB image and `Images/` with all seven component images.
- `OTA/tongdou-esp32s3_0.1.2.bin`: exactly the packaged application for compatible recovery layouts. Never flash it at `0x0`.
- `FLASH_TONGDOU.bat`, `Flash_Tool/` with the executable, port selector and licences.
- Chinese/English quick flashing guides, Features & Usage, Quick Start, and `Early_Tester_Guide.md`.
- `FLASH_LAYOUT.txt`, `BUILD_INFO.json`, `RELEASE_NOTES_CN.md`, and `SHA256SUMS.txt`.

Factory defaults: British English resource version 2; speaker volume 60%; activity reminders enabled at 60 minutes; hydration reminders enabled at 45 minutes. Seven selectable locales: en-GB, en-CA, fr-FR, fr-CA, nl-NL, ru-RU, zh-CN. The local wake phrase remains Chinese: “你好，铜豆”. Other resource packs are downloaded from the paired server.

The project owner confirmed physical IP/console queries. Memory requires server configuration; multi-speaker and cross-session behaviour remains a follow-up test. This delivery does not claim production-level OTA power-loss stress validation.

## Recovery

This hardware has no RESET button. Use the power switch to restart. If the board has an accessible BOOT input, hold it low while switching on, then release it to enter USB download mode. Otherwise obtain board-specific instructions. Do not short unidentified pads.

No compiler or Python installation is needed for the Windows flasher. This package does not include hosted services, model credits, provider keys, or firmware source.
