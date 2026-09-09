# TongDou Firmware v0.1.3 Early Tester

September 9, 2026 — Improves wake-word recognition stability.

This is a firmware-only patch for TongDou V9 with ESP32-S3-MINI-1-N4R2 and
4 MB flash. Continue using **TongDou Server v0.1.2**. There is no new server
package, and an existing v0.1.2 server does not need to be reinstalled.

## Change

The local recognition timeout is extended from 3 to 10 seconds to reduce
frequent recognition resets. Recognising “你好，铜豆” still starts a response
immediately; the device does not wait for the timeout.

The model, voice gain, recognition threshold, server protocol, flash layout,
and other firmware behaviour are unchanged from v0.1.2.

## Validation

The project owner reported a noticeable improvement after testing the
10-second adjustment. The supplied log contains nine successful local wake
events; all nine started a conversation and later completed normal server-led
conversation exit. The total number of spoken attempts was not recorded, so
this is not a measured 100% first-attempt success rate.

The versioned v0.1.3 application and factory builds passed compilation.
Package checks cover embedded application version, component checksums,
partition placement, the complete 4 MB image, and ZIP integrity. A fresh
full-image flash of the final versioned package has not been separately
validated on hardware. Long standby, different speakers/distances, and
background-sound false activations remain follow-up tests.

## Downloads

- [Firmware v0.1.3 ZIP](https://github.com/solitary-dev-50/tongdou/releases/download/v0.1.3-early-tester/TongDou_Application_Firmware_v0.1.3_Early_Tester.zip)
- [Firmware v0.1.3 SHA-256 checksum](https://github.com/solitary-dev-50/tongdou/releases/download/v0.1.3-early-tester/TongDou_Application_Firmware_v0.1.3_Early_Tester.zip.sha256.txt)
- [Unchanged server v0.1.2 ZIP](https://github.com/solitary-dev-50/tongdou/releases/download/v0.1.2-early-tester/TongDou_Server_v0.1.2_Early_Tester.zip)
- [Previous firmware and server v0.1.2 release](https://github.com/solitary-dev-50/tongdou/releases/tag/v0.1.2-early-tester)

Download the named firmware ZIP. GitHub's automatic Source code archives are
repository snapshots, not firmware installers.

## Install or roll back

Extract the entire firmware package, connect a USB data cable, close serial
monitors, and run `FLASH_TONGDOU.bat`. Follow the included English or Chinese
quick flashing guide.

The supplied script writes a complete 4 MB image at `0x0`. It clears device
Wi-Fi, server address, language, volume, reminders, and motor settings. Set
these up again after installation. The v0.1.2 and v0.1.3 partition layouts are
the same, but this installer still performs a full installation. Migration
from v0.1.0 also requires the complete image because its layout differs.

The `OTA/` directory contains only the application image. Never flash it at
`0x0`. Remote updates through the paired full server dashboard remain
unverified; use the documented USB installation.

Previous releases remain available. To roll back, download the older firmware
package and follow its flashing instructions. Its full-image installer also
clears device settings. Keep the existing server v0.1.2 installation and data.
The board has no RESET button; follow the included power-switch and BOOT
recovery guidance if required.
