# TongDou V9 Board No-Load Test Firmware

This is a local bring-up firmware for TongDou V9 PCB no-load testing.

It is used before full assembly to check board-level peripherals without running the demo firmware or the main firmware.

## Current Checks

- QMI8658A on I2C IO5/IO6
- WS2812 LED on IO9
- PDM microphone on IO1/IO2
- NS4168 amplifier control and I2S beep on IO15/IO12/IO13/IO14
- AT8833CT motor driver wake and short output pulses on IO42/IO38/IO39/IO40/IO41
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

- This firmware is local test tooling.
- It should not replace the demo firmware.
- It should not replace the main firmware.
- Keep each test simple and visible in the serial log.
