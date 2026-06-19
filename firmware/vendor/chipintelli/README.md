# ChipIntelli voice assets

This directory keeps voice assets exported from the ChipIntelli AI platform.

Current ASR package:

- `tongdou_ci13162p_20260619`

Current TTS package:

- `tongdou_tts_20260619`

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

TTS package contents:

- `src/*.wav`
- `mp3/*.mp3`
- `opus/*.opus`
- `speex/*.speex`
- `template_chinese_prompts_*.xlsx`

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

Verify TTS prompt assets after every platform re-export:

```powershell
python firmware/tools/verify_voice_prompts.py
```

The verifier checks prompt IDs, prompt names, prompt text, generated audio files, and firmware voice-line text.

Run all offline checks before committing firmware changes:

```powershell
powershell -ExecutionPolicy Bypass -File .\firmware\tools\check_all.ps1
```
