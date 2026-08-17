# Public Release Checklist

Use this checklist before publishing `hardware_test_firmware/` source or release
artifacts.

## Source Boundary

- [ ] The source package contains only `hardware_test_firmware/` files.
- [ ] The complete application firmware under the repository-root `firmware/` directory is not included.
- [ ] The internal demo firmware under the repository-root `local_demo_firmware/` directory is not included.
- [ ] Website files under the repository-root `site/` directory are not included.
- [ ] Hardware PCB, schematic, Gerber, BOM, mechanical, fixture, and tooling files are not included unless the release scope explicitly says so.
- [ ] Audio files are not included.
- [ ] PlatformIO build caches such as `.pio/` are not included.
- [ ] Temporary release artifacts are not committed unless they are intentionally part of the release.
- [ ] `led_data_high_test_firmware/` is reviewed as a temporary diagnostic sub-firmware before inclusion.

## Firmware Boundary

- [ ] No full TongDou scenario state machine is included.
- [ ] No demo scene, filming scene, or scripted personality sequence is included.
- [ ] No personality prompt or AI conversation logic is included.
- [ ] No full expression, light, voice, and motor timeline is included.
- [ ] No formal product firmware feature is included.
- [ ] No complete product action or scene choreography is included.
- [ ] Logo double-tap gyro return or wiggle behavior has been reviewed before publication.
- [ ] Engineering maintenance features are documented as risky when they can move motors, write Flash, change radar parameters, or discharge the battery.

## Sensitive Data

- [ ] No Wi-Fi password is present.
- [ ] No token or API key is present.
- [ ] No certificate or private key is present.
- [ ] No local absolute path is present in docs or source.
- [ ] No personal email, private server address, MAC address, or admin information is present.

Suggested scan:

```bash
rg -n -S "password|passwd|ssid|token|api[_-]?key|secret|BEGIN PRIVATE|WiFi.begin|Authorization|Bearer|macAddress|http://|https://|ws://|wss://" hardware_test_firmware
rg --files hardware_test_firmware -g "*.mp3" -g "*.wav" -g "*.m4a" -g "*.aac" -g "*.flac" -g "*.pcm" -g "*.pem" -g "*.key" -g "*.crt" -g "*.p12" -g "*.env"
rg --files hardware_test_firmware | rg -i "\.pio|\.elf$|\.map$|\.zip$|\.7z$|\.rar$|\.log$|\.tmp$"
```

Expected notes:

- `TongDou-BoardTest` is the public test access-point name, not a password.
- `http://192.168.4.1/motor` is the local board-test page.
- Local variable names such as `token` may appear when parsing serial command words; they are not credentials.

## Build Check

Run:

```bash
cd hardware_test_firmware/firmware
platformio run
```

Current PlatformIO project settings should be checked from
`hardware_test_firmware/firmware/platformio.ini`, not guessed from ESP-IDF
defaults.

Expected public binary sources after build:

- `.pio/build/esp32s3/bootloader.bin`
- `.pio/build/esp32s3/partitions.bin`
- `.pio/build/esp32s3/firmware.bin`
- PlatformIO Arduino `boot_app0.bin`

Expected ESP32-S3 flash addresses:

- `0x0`: `bootloader.bin`
- `0x8000`: `partitions.bin`
- `0xe000`: `boot_app0.bin`
- `0x10000`: `firmware.bin`

If a merged image is produced, it should be generated from these PlatformIO
artifacts and flashed at `0x0`.

Optional short smoke test:

```bash
cd hardware_test_firmware/qmi8658a_test_firmware
platformio run
```

## Manual Hardware Check

- [ ] OLED shows the expected test face/status and battery status screen.
- [ ] SK6812-EC20 shows red, green, blue, and off.
- [ ] PDM microphone prints changing level statistics.
- [ ] NS4168 plays only the generated test tone.
- [ ] Left and right motors pass short forward/reverse/manual checks.
- [ ] QMI8658A appears on the I2C scan and prints motion data.
- [ ] LD2412 radar OUT and serial parser checks work.
- [ ] Battery percent estimate, voltage, USB, charge, and standby readings make sense.
- [ ] Shipping discharge refuses to start with USB connected.
- [ ] Shipping discharge stops the motors and alarms at 30% estimated charge.
