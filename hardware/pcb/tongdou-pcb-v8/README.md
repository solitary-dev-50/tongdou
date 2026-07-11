# TongDou PCB v8

This directory contains the verified TongDou PCB v8 design package.

## Contents

- `source/` - Editable JLCEDA Pro project file
- `schematic/` - Main-control and power schematic PDFs
- `bom/` - Bill of materials in UTF-8 CSV format
- `manufacturing/` - Gerber archive and pick-and-place CSV

## Hardware

- ESP32-S3-MINI-1-N4R2
- IM72D128V01XTMA1 PDM microphone
- NS4168 I2S audio amplifier
- 0.96-inch SSD1306 I2C OLED module
- WS2812B-2020 RGB LED
- AT8833CT dual-motor driver
- HLK-LD2412 24 GHz human-presence radar module
- TP4057 battery charger

## External Connections

OLED, radar, motors, and speaker leads are soldered directly to the PCB pads.
No pin headers or external sockets are installed for those parts.

The battery uses the H6 wire-to-board connector:

```text
HX-PH2.0-2PZZ
```

Use insulated wire for the five LD2412 connections: 3V3, GND, OUT, TX, and RX.
Do not use bare wire for the radar module, because it can contact the grounded
brass frame.

## Mechanical File

The H62 brass side-plate drawing is available at:

[`hardware/mechanical/TongDou_Brass_Frame_1mm_H62_RevA.dxf`](../../mechanical/TongDou_Brass_Frame_1mm_H62_RevA.dxf)

## Scope

This is a PCB design package, not a complete TongDou manufacturing kit.
The original fixture, additional mechanical files, build records, and tutorial
material are released separately as they are prepared.

## License

Hardware design material in this package is licensed under
[CERN-OHL-P-2.0](../../../LICENSE). See [NOTICE.md](../../../NOTICE.md) for
attribution and brand-use boundaries.
