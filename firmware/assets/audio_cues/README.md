# Audio cues

This directory stores source audio used to build TongDou locale packs and the
small language-independent sounds embedded in the application image.

These files are temporary test assets copied from `xiaozhi-esp32` so the firmware
can verify local prompt playback before TongDou's own sound set is ready.

`offline_notice.ogg` is generated separately from
`offline_notice_manifest.json` with `edge-tts 7.2.6`, then converted to
16 kHz mono Ogg Opus by FFmpeg. It is also a temporary test asset and must be
replaced or relicensed before a public production release.

Source repository:

- `https://github.com/78/xiaozhi-esp32`

Source license:

- MIT License

Before public release, replace these temporary files with TongDou-owned audio
assets and re-check licensing.

Only the five files under `system/` are embedded in the application image.
Files under `locales/` are build inputs: the factory image writes one `en-GB`
pack to the dedicated `locale` partition, and the server provides later
language replacements. They are not linked into the application image.

## Directory layout

- `system/`: short non-verbal status sounds.
- `locales/zh-CN/`: Chinese spoken prompts and digits.
- `locales/en-US/`: English spoken prompts and digits.
- `locales/en-GB/`: United Kingdom English prompts and digits.
- `locales/en-CA/`: Canada English prompts and digits.
- `locales/fr-FR/`: France French prompts and digits.
- `locales/fr-CA/`: Canada French prompts and digits.
- `locales/nl-NL/`: Netherlands Dutch prompts and digits.
- `locales/ru-RU/`: Russian prompts and digits.

Temporary fallback notes:

- `en-GB` and `en-CA` currently reuse the `en-US` source audio.
- `fr-CA` currently reuses the `fr-FR` source audio.

Locale audio filenames include the locale prefix so every embedded file has a
unique filename. For example:

- `locales/en-GB/en_GB_wifi_config.ogg`
- `locales/fr-FR/fr_FR_digit_0.ogg`
- `locales/ru-RU/ru_RU_activation_code.ogg`

## Cue names

- `system/wake.ogg`: wake or enter conversation cue.
- `system/success.ogg`: successful operation cue.
- `system/error.ogg`: generic error cue.
- `system/low_battery.ogg`: low battery cue.
- `system/vibration.ogg`: short vibration-like cue.
- `locales/<locale>/<locale_prefix>_welcome.ogg`: normal startup prompt.
- `locales/<locale>/<locale_prefix>_wifi_config.ogg`: first boot or Wi-Fi setup prompt.
- `locales/<locale>/<locale_prefix>_upgrade.ogg`: firmware upgrade prompt.
- `locales/<locale>/<locale_prefix>_activation_code.ogg`: activation or pairing code intro.
- `locales/<locale>/<locale_prefix>_activation_error.ogg`: activation or registration error.
- `locales/<locale>/<locale_prefix>_pin_error.ogg`: pin or code error.
- `locales/<locale>/<locale_prefix>_offline_notice.ogg`: online voice unavailable notice.
- `locales/<locale>/<locale_prefix>_digit_0.ogg` to `<locale_prefix>_digit_9.ogg`:
  digit prompts for code playback.
