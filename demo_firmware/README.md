# TongDou Demo Firmware

## What this is

TongDou Demo Firmware is a scene example firmware for developers to study,
modify, and experiment with.

It is not the long-term maintained TongDou application firmware. It is also not
the official daily-use TongDou firmware.

This demo firmware evolved scene by scene on real TongDou hardware, so some
timing values, motion tuning, and experimental paths reflect the actual process
of making the character work.

This is provided as a development example rather than a long-term maintained
application. Feel free to change the dialogue, expressions, timing, lights, and
movement to make your own TongDou scenes.

Software source code in this directory is licensed under the MIT License. See
[LICENSE](LICENSE).

The audio files in `firmware/data/audio/` are official TongDou character
assets. They are included only so the published demo scenes can run as intended
on TongDou hardware, and they are not covered by the MIT License.

## Target hardware

This firmware targets TongDou V9 hardware:

- ESP32-S3-MINI-1-N4R2
- 4 MB Flash
- IM72D128V01XTMA1 PDM digital microphone
- NS4168 I2S speaker amplifier
- 0.96 inch OLED
- SK6812-EC20 status LED
- AT8833CT dual motor driver, with nFAULT on IO37
- QMI8658A 6-axis IMU
- TongDou logo touch input on IO4
- LD2412 radar interface
- TP4057 charging and battery detection

This firmware is built for the 4 MB TongDou V9 route.

## Complete demo scenes

The complete runnable examples in this release are:

| Button | Scene | Audio included |
| --- | --- | --- |
| `8` | First Summon / Nightmare | Yes |
| `6-1` | Confused Accountant - clear | Yes |
| `6-2` | Confused Accountant - extort | Yes |
| `6-3` | Confused Accountant - bribe | Yes |
| `6-4` | Confused Accountant - forgetful | Yes |
| `6-5` | Confused Accountant - pi | Yes |

Scene 6 and Scene 8 include the real tuned audio, OLED expressions, motor
movement, LED effects, and timing synchronization used on TongDou V9 hardware.

Note: Scene 6 begins with an approximately 10-second visual prelude before the
accountant voice line starts. During this time, TongDou blinks, looks around,
and makes small movements. This is intentional and was originally designed as a
"listening / waiting" performance before the spoken scene begins. It is not a
delay, freeze, or audio synchronization issue.

Scene 8 also includes a short intentional opening pause before the main spoken
performance begins.

## Development scene templates

Scenes `1`, `2`, `3`, `5`, and `7` are kept in source as development
experiments and scene templates.

Their original voice clips were never recorded, so this release does not
include audio resources for those scenes. They are left in place so developers
can study the timeline structure and replace the dialogue, expressions, lights,
and movement with their own material.

This is intentional for this release. It is not a missing-file issue, and this
release does not promise that those audio files will be added later.

## Silent Scene 4

Scene `4` is a silent email notice. It is designed to show a quiet visual
notification without speaking, so it does not include audio.

In the current implementation, it uses a serious OLED expression, soft white
light, then weak breathing light.

## Build with PlatformIO

The PlatformIO project is here:

```text
demo_firmware/firmware
```

Build the application firmware:

```bash
cd demo_firmware/firmware
platformio run
```

Build the LittleFS audio image:

```bash
cd demo_firmware/firmware
platformio run -t buildfs
```

Current PlatformIO configuration:

- Environment: `esp32s3`
- Platform: `platformio/espressif32@6.10.0`
- Board: `esp32-s3-devkitc-1`
- Framework: Arduino
- Flash size used by this project: 4 MB
- Partition table: `partitions_demo_audio.csv`
- Filesystem: LittleFS
- C++ standard: GNU++17

## Flashing

For developers using PlatformIO:

```bash
cd demo_firmware/firmware
platformio run -t upload
platformio run -t uploadfs
```

For Windows users, the prebuilt release package provides a one-click flashing
option without requiring PlatformIO or Python. That package contains one merged
4 MB image:

```text
TongDou_Demo_Firmware_v0.1.0.bin
```

The one-click script flashes that merged image at:

```text
0x0
```

The merged image contains:

- Bootloader
- Partition table
- Application firmware
- LittleFS audio resources

## How to use

1. Flash the demo firmware.
2. Power on TongDou V9.
3. Connect your phone or computer to:

```text
TongDou-Demo
```

4. Open:

```text
http://192.168.4.1
```

5. Use the page buttons:

- `8` for First Summon / Nightmare
- `6-1` to `6-5` for Confused Accountant branches
- `4` for silent email notice
- `0` to stop and return to idle

## Serial commands

Useful serial commands:

```text
0
stop
idle
6
6-1
6-2
6-3
6-4
6-5
accountant
accountant 1
accountant 2
accountant 3
accountant 4
accountant 5
summon
nightmare
reset_first_summon
```

`summon` and `nightmare` trigger Scene 8.

`reset_first_summon` clears the first-start marker, which lets Scene 8 play
automatically again on the next boot.

## Known boundaries

- This is a developer scene example firmware, not a complete product firmware.
- It is not the long-term maintained TongDou application firmware.
- It is built for TongDou V9 with 4 MB Flash.
- It does not include cloud voice, ASR, LLM, TTS, or a production assistant
  workflow.
- It does not include audio for Scenes `1`, `2`, `3`, `5`, or `7`.
- Scene `4` is intentionally silent.
- Scene 6 and Scene 8 timing, motion, OLED expressions, LED effects, and audio
  synchronization should be treated as real hardware tuning.
- Build outputs, `.pio`, `.vscode`, ELF files, map files, and temporary files
  should not be committed to the source repository.
