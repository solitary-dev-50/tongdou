# TongDou

> ⚠️ **Work in Progress**
>
> TongDou is being actively documented and open-sourced step by step.  
> Hardware design files, original fixture files, manufacturing documents,
> and build guides will be released progressively.  
> Please see the [Roadmap](#roadmap) below for the current release plan.

🌐 **Official website:** https://gettongdou.com

TongDou is a small desktop companion that turns ordinary reminders into
expressions, light, movement, voice, and character.

It is not a timer with a speaker. TongDou is designed to make everyday
reminders feel more human: a little expressive, a little playful, and hard to
ignore in the best possible way.

**TongDou has been physically built and is running.** It is now being
open-sourced step by step by one independent maker.

## Official Website

Follow TongDou’s development, testing program, and future crowdfunding
updates:

https://gettongdou.com

## What TongDou Demonstrates

- OLED expressions and status display
- WS2812 light effects
- Dual-motor movement
- Voice playback and scripted interactions
- Logo touch interaction
- Motion, tilt, and tap sensing
- Millimeter-wave presence detection
- A compact brass-and-PCB mechanical form

The current demonstration firmware is internal development and performance
firmware. It is shown in videos, but it is not included in this repository and
is not a promised future release.

## Hardware Platform

- ESP32-S3-MINI-1-N4R2
- IM72D128V01XTMA1 PDM microphone
- NS4168 I2S audio amplifier
- 0.96-inch OLED display
- WS2812B-2020 RGB LED
- AT8833CT dual-motor driver, with nFAULT connected to IO37
- QMI8658A 6-axis IMU
- TongDou logo touch input on IO4
- HLK-LD2412 millimeter-wave radar module
- TP4057 battery charging and battery monitoring

The V9 PCB is the current bring-up target. The public PCB package currently
available in this repository is still the verified v8 package; V9 manufacturing
files will be published after bring-up checks are complete.

For the currently published design files, electrical details, and PCB assembly
notes, see the [TongDou PCB v8 package](hardware/pcb/tongdou-pcb-v8/).

## Open-Source Status

TongDou is a phased open-source hardware project. It does not provide a
complete manufacturing package at this stage.

### Available now

- Project overview and hardware direction
- Verified prototype status
- [Hardware release package](hardware/)
- [TongDou PCB v8 design package](hardware/pcb/tongdou-pcb-v8/)
- [TongDou brass side-plate DXF](hardware/mechanical/TongDou_Brass_Frame_1mm_H62_RevA.dxf)
- License, attribution, and brand-use boundaries
- Hardware and documentation release plan

### Not included

- Internal demonstration firmware
- Private data, keys, backend configuration, and service credentials
- Any asset not explicitly marked as open hardware material

## Roadmap

### 1. Project entry

Project overview, real prototype media, demonstration video, verified status,
license information, and the public release route.

### 2. Additional hardware design materials

The verified TongDou PCB v8 package is available now. It includes editable PCB
source files, schematic PDFs, Gerber manufacturing files, BOM, and
pick-and-place data.

Further hardware notes and validation records will be added progressively.

### 3. Original fixture and mechanical package

TongDou’s original soldering and alignment fixture, CAD and STEP files,
mechanical drawings, CNC notes, and fixture documentation.

### 4. Build and verification records

Brass-frame soldering, assembly, power-on checks, testing records, and
troubleshooting material.

## TongDou PCB v8

The current verified PCB package is available in
[hardware/pcb/tongdou-pcb-v8](hardware/pcb/tongdou-pcb-v8/):

- Editable PCB source file
- Main-control and power schematic PDFs
- Gerber manufacturing archive
- BOM with PCB, external-component, and assembly-material categories
- Pick-and-place data

This is a PCB design package. TongDou as a whole is still being released in
phases and is not yet a complete end-to-end manufacturing kit.

V9 hardware bring-up is in progress. V9 adds the QMI8658A motion sensor, the
TongDou logo touch input on IO4, and AT8833CT nFAULT feedback on IO37. The
QMI8658A has passed board-level bring-up and is used by the local demo
firmware's `/motor` diagnostic page for gyro-assisted straight-line testing;
V9 files are not yet part of the published manufacturing package.

## License and Attribution

Hardware design materials explicitly marked as open hardware are licensed
under [CERN-OHL-P-2.0](LICENSE). The full license text is included in this
repository.

You may use, copy, modify, manufacture, and sell hardware based on those
materials, provided that the original attribution and license notice are kept.

Recommended attribution:

```text
Based on the TongDou open-source project by solitary-dev-50
https://github.com/solitary-dev-50/tongdou
```

See [NOTICE.md](NOTICE.md) for the exact license scope, attribution rules, and
brand boundaries.

### Brand Boundary

The name “TongDou”, its logo, character design, official audio, promotional
videos, and official packaging are not granted under the hardware license.

Third parties must not present a derivative as an official TongDou product or
claim official certification or endorsement.

## Repository Documents

- [Hardware Release Package](hardware/)
- [TongDou PCB v8 Package](hardware/pcb/tongdou-pcb-v8/)
- [License Scope and Attribution](NOTICE.md)

## Inspiration & Acknowledgements

TongDou's brass-and-PCB aesthetic and compact mechanical form were deeply
inspired by the incredible craftsmanship of **Huy Vector** of
**Huy Vector Lab**, particularly the desktop robot **Mo-chan**, as shown
in [this original build video](https://www.youtube.com/watch?v=3hjvpyjxPsk).

I’m a huge fan of Huy’s work and highly recommend exploring the original
projects and [Huy Vector on YouTube](https://www.youtube.com/@huyvector).

Thank you, Huy, for inspiring TongDou and for showing how beautifully brass,
PCBs, and mechanical structures can be brought to life.

TongDou’s electronics, PCB, brass frame, original soldering and alignment
fixture, firmware, character design, and interaction system were independently
developed for this project.

## Community & Contact

> **Note:** TongDou is currently a solo-maker project. To keep my focus on
> hardware development, firmware polishing, documentation, and the early
> tester program, GitHub Issues and Discussions are temporarily disabled.
>
> To follow project updates, join the waitlist, or apply for the early tester
> program, please visit the [TongDou official website](https://gettongdou.com)
> and read the [Tester Program & Agreement](https://gettongdou.com/tester-agreement.html).
>
> For essential project, media, licensing, or collaboration inquiries:
> **[hello@gettongdou.com](mailto:hello@gettongdou.com)**
>
> Thank you for your patience while I build and document TongDou step by step.
