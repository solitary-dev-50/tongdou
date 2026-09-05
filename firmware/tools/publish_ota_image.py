"""Validate and publish the current formal firmware image to a server bin directory."""

from __future__ import annotations

import argparse
import configparser
import hashlib
import re
import shutil
import tempfile
from pathlib import Path


MODEL = "tongdou-esp32s3"
MAIN_PARTITION_SIZE = 0x200000


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(64 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def configured_version(firmware_directory: Path) -> str:
    parser = configparser.ConfigParser(interpolation=None)
    parser.read(firmware_directory / "platformio.ini", encoding="utf-8")
    version = parser["env:esp32s3"]["custom_firmware_version"].strip()
    if not re.fullmatch(r"\d+(?:\.\d+)+", version):
        raise SystemExit("custom_firmware_version must contain numeric dot segments")
    return version


def main() -> None:
    firmware_directory = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--server-bin-dir", type=Path)
    parser.add_argument("--check-only", action="store_true")
    parser.add_argument("--replace", action="store_true")
    args = parser.parse_args()

    image = firmware_directory / ".pio" / "build" / "esp32s3" / "firmware.bin"
    if not image.is_file():
        raise SystemExit("firmware image missing; run platformio run -e esp32s3 first")
    size = image.stat().st_size
    if size <= 0 or size > MAIN_PARTITION_SIZE:
        raise SystemExit(
            f"firmware image size {size} exceeds app0 limit {MAIN_PARTITION_SIZE}"
        )

    version = configured_version(firmware_directory)
    digest = sha256(image)
    filename = f"{MODEL}_{version}.bin"
    print(f"version={version}")
    print(f"filename={filename}")
    print(f"size={size}")
    print(f"sha256={digest}")

    if args.check_only:
        return
    if args.server_bin_dir is None:
        raise SystemExit("--server-bin-dir is required unless --check-only is used")

    destination_directory = args.server_bin_dir.resolve()
    destination_directory.mkdir(parents=True, exist_ok=True)
    destination = destination_directory / filename
    if destination.exists() and not args.replace:
        raise SystemExit(f"destination already exists: {destination}")
    temporary_path = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=destination_directory,
            prefix=f".{filename}.",
            suffix=".part",
            delete=False,
        ) as temporary_file:
            temporary_path = Path(temporary_file.name)
        shutil.copy2(image, temporary_path)
        temporary_path.replace(destination)
    finally:
        if temporary_path is not None and temporary_path.exists():
            temporary_path.unlink()
    print(f"published={destination}")


if __name__ == "__main__":
    main()
