#!/usr/bin/env python3

import argparse
import asyncio
import json
import shutil
import subprocess
import tempfile
from pathlib import Path

import edge_tts


def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate Tong Dou offline notice Ogg Opus assets."
    )
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--output-root", type=Path)
    return parser.parse_args()


async def synthesize(locale: str, voice: str, text: str, output: Path, audio: dict):
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as temporary_directory:
        mp3_path = Path(temporary_directory) / f"{locale}.mp3"
        await edge_tts.Communicate(text=text, voice=voice).save(str(mp3_path))
        subprocess.run(
            [
                "ffmpeg",
                "-hide_banner",
                "-loglevel",
                "error",
                "-y",
                "-i",
                str(mp3_path),
                "-ar",
                str(audio["sampleRate"]),
                "-ac",
                str(audio["channels"]),
                "-c:a",
                "libopus",
                "-b:a",
                audio["bitrate"],
                "-application",
                "voip",
                "-frame_duration",
                "20",
                str(output),
            ],
            check=True,
        )


async def main():
    args = parse_args()
    tools_directory = Path(__file__).resolve().parent
    firmware_directory = tools_directory.parent
    manifest_path = args.manifest or (
        firmware_directory / "assets" / "audio_cues" / "offline_notice_manifest.json"
    )
    output_root = args.output_root or (
        firmware_directory / "assets" / "audio_cues" / "locales"
    )

    if shutil.which("ffmpeg") is None:
        raise SystemExit("ffmpeg is required")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    audio = manifest["audio"]
    for locale, entry in manifest["locales"].items():
        prefix = locale.replace("-", "_")
        output = output_root / locale / f"{prefix}_offline_notice.ogg"
        await synthesize(locale, entry["voice"], entry["text"], output, audio)
        print(f"offline notice generated locale={locale} output={output}")


if __name__ == "__main__":
    asyncio.run(main())
