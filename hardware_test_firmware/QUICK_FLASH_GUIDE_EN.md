# TongDou V9 Hardware Test Firmware Quick Flash Guide

This guide is for users who only want to flash the prebuilt firmware. You do
not need to install PlatformIO, ESP-IDF, Python, or any compiler.

## Recommended Method

Use the release ZIP package and run the one-click flasher.

1. Extract the ZIP package.
2. Connect TongDou V9 to the Windows computer with a USB data cable.
3. Double-click:

```text
FLASH_TONGDOU.bat
```

The script will:

- Use the included official Espressif `esptool.exe`
- Search for an Espressif serial device automatically
- Flash `TongDou_V9_Hardware_Test_v1.0.bin` at `0x0`
- Reset TongDou after flashing
- Keep the window open and show `SUCCESS` or `ERROR`

## How To Confirm It Worked

After reboot, TongDou should start the test access point:

```text
TongDou-BoardTest
```

Connect to it and open:

```text
http://192.168.4.1/motor
```

You should see the TongDou board-test web page.

## Backup Steps If Auto Flash Fails

Try these steps, then run `FLASH_TONGDOU.bat` again:

1. Use a USB data cable, not a charge-only cable.
2. Close PlatformIO, serial monitor, and any serial terminal.
3. Open Windows Device Manager and check that a COM port appears.
4. Enter ESP32-S3 Download Mode if needed:
   - Hold BOOT.
   - Tap RESET.
   - Release BOOT.
5. Try another USB port.

## What Is Inside The ZIP

```text
TongDou_V9_Hardware_Test_v1.0.bin
FLASH_TONGDOU.bat
Flash_Tool/esptool.exe
Flash_Tool/LICENSE
Flash_Tool/SOURCE_AND_LICENSE.txt
QUICK_FLASH_GUIDE_EN.md
QUICK_FLASH_GUIDE_CN.md
README.md
```

The flash tool is the official Espressif esptool standalone Windows release.
License and source information are included in `Flash_Tool/`.
