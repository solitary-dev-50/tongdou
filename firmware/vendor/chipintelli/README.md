# ChipIntelli voice model assets

This directory keeps voice model assets exported from the ChipIntelli AI platform.

Current package:

- `tongdou_ci13162p_20260619`

Source:

- Project: Tong Dou
- Chip: CI13162P
- Acoustic model: `V01050_中文_ASR_通用_Pro4_1.4M`
- Export date: 2026-06-19

Package contents:

- `GfstCmd/[0]lm_chinese_CI13162P_V01050.dat`
- `GfstWake/[1]lm_chinese_CI13162P_V01050.dat`
- `CmdWordStructure/[60000]{cmd_info}.xlsx`
- `CmdWordStructure/[60000]{cmd_info_old}.xlsx`

Current command ID mapping:

| commandID | Meaning |
| --- | --- |
| `1` | Wake word |
| `2` | Quiet mode |
| `3` | Snooze reminder |
| `4` | Confirm reminder |
| `5` | Reminder due |
| `6` | Guard word, ignored by ESP32 |

The ESP32 firmware currently treats the second byte after `0xAA` as this platform `commandID`.
If hardware testing shows that CI13162P sends a different UART frame, update only the voice parser layer first.

Verify this package after every platform re-export:

```powershell
python firmware/tools/verify_voice_model.py
```

The verifier reads `CmdWordStructure/[60000]{cmd_info}.xlsx` and checks that command IDs still match the firmware mapping.
