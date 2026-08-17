# TongDou LED_DATA HIGH Test Firmware

This standalone firmware only configures `LED_DATA / GPIO9` as a normal output
and holds it HIGH.

Use it to measure whether the ESP32-S3 GPIO can drive the LED data line high.

## Build

```bash
cd hardware_test_firmware/led_data_high_test_firmware
platformio run
```

## Upload

```bash
cd hardware_test_firmware/led_data_high_test_firmware
platformio run -t upload --upload-port COM13
```

## Measurement

Power the board after flashing, then measure:

- R9 GPIO side to GND: expected about 3.3V
- R9 LED side to GND: expected about 3.3V

This firmware does not send SK6812 data timing, so the LED may not light.
