# TongDou V9 QMI8658A Standalone Test Firmware

This is a small standalone firmware for checking QMI8658A and basic V9 board peripherals.

It is useful before full assembly when you want the shortest possible test program.

## Current Checks

- QMI8658A on I2C IO6/IO7
- SK6812-EC20 LED on IO9
- PDM microphone on IO1/IO2
- NS4168 amplifier control and I2S beep on IO15/IO12/IO13/IO14
- AT8833CT motor driver wake and short output pulses on IO42/IO38/IO39/IO40/IO41
- AT8833CT nFAULT input on IO37
- LD2412 radar OUT/TX/RX on IO5/IO10/IO11
- Battery voltage, USB detect, charge, and standby signals on IO8/IO17/IO35/IO48

## Serial Commands

```txt
scan
led auto
led red
led green
led blue
led white
led off
mic on
mic off
motor wake
motor sleep
motor test
motor a+
motor a-
motor b+
motor b-
amp on
amp off
amp beep
battery on
battery off
battery once
```

## Notes

- This firmware is public board-test tooling.
- It should not replace the full board test firmware.
- It should not replace the main firmware.
- Keep each test simple and visible in the serial log.
