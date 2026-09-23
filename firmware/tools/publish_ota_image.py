"""Validate and publish the current formal firmware image to a server bin directory."""

from __future__ import annotations

import argparse
import configparser
import hashlib
import re
import shutil
import tempfile
from pathlib import Path


MODELS = {
    "esp32s3": "tongdou-esp32s3",
    "esp32s3_v94": "tongdou-esp32s3-v9.4",
}
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


def validate_image_identity(image: Path, model: str, version: str) -> None:
    identity = f"tongdou-image:{model}:{version};".encode("ascii") + b"\x00"
    identities = re.findall(rb"tongdou-image:[^;\x00]+;\x00", image.read_bytes())
    if not identities or set(identities) != {identity}:
        raise SystemExit("firmware identity mismatch; rebuild the selected board/version")


def main() -> None:
    firmware_directory = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--server-bin-dir", type=Path)
    parser.add_argument("--check-only", action="store_true")
    parser.add_argument("--replace", action="store_true")
    parser.add_argument("--environment", choices=MODELS, default="esp32s3")
    args = parser.parse_args()

    image = firmware_directory / ".pio" / "build" / args.environment / "firmware.bin"
    if not image.is_file():
        raise SystemExit(f"firmware image missing; run platformio run -e {args.environment} first")
    size = image.stat().st_size
    if size <= 0 or size > MAIN_PARTITION_SIZE:
        raise SystemExit(
            f"firmware image size {size} exceeds app0 limit {MAIN_PARTITION_SIZE}"
        )

    version = configured_version(firmware_directory)
    model = MODELS[args.environment]
    validate_image_identity(image, model, version)
    digest = sha256(image)
    filename = f"{model}_{version}.bin"
    print(f"model={model}")
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
