# Public Release Checklist

Use this checklist before copying `board_test_firmware/` into a public GitHub
repository or release archive.

## Source Boundary

- [ ] The package contains only `board_test_firmware/` files.
- [ ] The full firmware under `firmware/` is not included.
- [ ] The internal demo firmware under `local_demo_firmware/` is not included.
- [ ] Website files under `site/` are not included.
- [ ] Hardware PCB, schematic, Gerber, BOM, mechanical, fixture, and tooling files are not included unless the release scope explicitly says so.
- [ ] Audio files are not included.

## Firmware Boundary

- [ ] No full TongDou scenario state machine is included.
- [ ] No demo scene, filming scene, or scripted personality sequence is included.
- [ ] No personality prompt or AI conversation logic is included.
- [ ] No full expression, light, voice, and motor timeline is included.
- [ ] No formal product firmware feature is included.
- [ ] No gyro closed-loop straight-line correction is included.
- [ ] No automatic motor learning algorithm is included.
- [ ] No production calibration flow is included.

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
```

Expected notes:

- `TongDou-BoardTest` is the public test access-point name, not a password.
- `http://192.168.4.1/motor` is the local board-test page.
- Local variable names such as `token` may appear when parsing serial command words; they are not credentials.

## Build Check

Run:

```bash
cd board_test_firmware/firmware
platformio run
```

Optional short smoke test:

```bash
cd board_test_firmware/qmi8658a_test_firmware
platformio run
```

## Manual Hardware Check

- [ ] OLED shows the expected test face/status.
- [ ] SK6812-EC20 shows red, green, blue, and off.
- [ ] PDM microphone prints changing level statistics.
- [ ] NS4168 plays only the generated test tone.
- [ ] Left and right motors pass short forward/reverse/manual checks.
- [ ] QMI8658A appears on the I2C scan and prints motion data.
- [ ] LD2412 radar OUT and serial parser checks work.
- [ ] Battery, USB, charge, and standby readings make sense.
