# TongDou v0.1.2 quick flashing

1. Extract the whole package. Connect a USB data cable and close serial monitors.
2. Run `FLASH_TONGDOU.bat` and confirm the fresh installation.
3. Select the correct USB serial port; identify it by unplugging/reconnecting TongDou if needed.
4. Wait for `SUCCESS`. If needed, restart using the power switch.
5. Start the matching TongDou Server v0.1.2 and note its device Server address (normally port 8102).
6. Join `TongDou-Setup` and open `http://192.168.4.1/`.
7. Set Wi-Fi and the server address, bind the activation code, and configure your own speech/model providers.
8. Recheck language, volume, reminders, and left/right motor settings.

This full 4 MB installation resets device settings including motor calibration. The hardware MAC does not change. Moving from the v0.1.0 partition layout requires the complete USB image.

There is no RESET button. If an accessible BOOT input is provided and identified on your board, hold it low while switching the power on, then release it. Otherwise obtain board-specific recovery instructions; do not short unidentified pads.

Do not flash the application-only file in `OTA/` at address 0x0. See Features & Usage for capabilities and test limits.
