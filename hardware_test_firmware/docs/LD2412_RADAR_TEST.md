# LD2412 Radar Diagnostic Test

This document records the public LD2412 radar diagnostic behavior in the
TongDou V9 hardware test firmware.

## Scope

This is a hardware bring-up and diagnostic test. It is not a production
calibration flow and it is not part of the complete TongDou application
firmware.

The public firmware exposes:

- `/api/radar`
- Live target / no-target status
- Motion target status
- Nearest reported target distance
- Valid frame count
- Bad frame count
- Empty-scene background calibration command and status check

## Web Test

1. Flash `hardware_test_firmware/firmware` to the V9 board.
2. Power on the board.
3. Connect to the `TongDou-BoardTest` access point.
4. Open `http://192.168.4.1/motor`.
5. Use the `Radar Recognition` panel.

The panel can read the radar once, start live polling, run the guided test, and
start empty-scene calibration.

## Serial Commands

```text
radar
radar target
radar sample
radar parser selftest
radar guided test
radar guided status
radar guided blocking
radar config
radar sensitivity
radar desk
radar near
radar resolution
radar res20
radar calibrate
radar calibrate status
radar bridge
```

## API

```text
GET /api/radar
```

Important fields:

- `received`: whether a recent LD2412 serial frame was received.
- `hasTarget`: live target / no-target status from the LD2412 serial frame.
- `movingTarget`: whether the current LD2412 state reports a moving target.
- `staticTarget`: whether the current LD2412 state reports a static target.
- `nearestDistanceCm`: nearest reported target distance in centimeters.
- `targetDistanceCm`: same distance value kept for compatibility.
- `movingDistanceCm`: moving target distance reported by the module.
- `staticDistanceCm`: static target distance reported by the module.
- `validFrameCount`: accepted LD2412 serial frame count.
- `badFrameCount`: rejected LD2412 serial frame count.

Distance is a module-reported diagnostic value, not a precision ruler.

## Real Board Result

The current V9 board was checked in a real desk environment:

- Empty scene: 25 of 25 frames reported no target.
- Hand wave: 25 of 25 frames reported a target.
- Motion recognition: 23 frames correctly reported a moving target.
- Nearest distance: about 60 cm, matching the test setup.
- Bad serial frames: 0.

## Empty-Scene Background Calibration

Run empty-scene background calibration when the radar module is replaced or the
mounting direction changes.

Before starting calibration:

- Keep the radar facing an empty scene.
- Do not move a hand or tool in front of the radar.
- Keep the board in its normal installed orientation.

Commands:

```text
radar calibrate
radar calibrate status
```

The web page exposes the same action as `Empty-scene calibration`.

This calibration is a field diagnostic step. It is intentionally documented as
part of the hardware test firmware, not as manufacturing or production
calibration.
