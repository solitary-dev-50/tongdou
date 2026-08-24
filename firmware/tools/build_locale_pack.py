#!/usr/bin/env python3

import argparse
import hashlib
import struct
from pathlib import Path


MAGIC = b"TDLPACK1"
FORMAT_VERSION = 1
HEADER_FORMAT = "<8sH8sHIIIII32s"
ENTRY_FORMAT = "<32sII"
MAX_FILES = 32
PACK_SIZE = 0x20000
REQUIRED_FILES = {
    "welcome.ogg",
    "wifi_config.ogg",
    "upgrade.ogg",
    "activation_code.ogg",
    "activation_error.ogg",
    "pin_error.ogg",
    *(f"digit_{digit}.ogg" for digit in range(10)),
}


def parse_args():
    parser = argparse.ArgumentParser(
        description="Build one Tong Dou locale resource pack."
    )
    parser.add_argument("--locale", required=True)
    parser.add_argument("--version", type=int, default=1)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def resource_name(path: Path, locale: str) -> str:
    prefix = locale.replace("-", "_") + "_"
    if not path.name.startswith(prefix):
        raise ValueError(f"unexpected locale filename: {path.name}")
    return path.name[len(prefix) :]


def main():
    args = parse_args()
    tools_dir = Path(__file__).resolve().parent
    firmware_dir = tools_dir.parent
    source = args.source or firmware_dir / "assets" / "audio_cues" / "locales" / args.locale
    files = sorted(path for path in source.glob("*.ogg") if path.is_file())
    if not files:
        raise SystemExit(f"no Ogg Opus files found in {source}")
    if len(files) > MAX_FILES:
        raise SystemExit(f"too many files: {len(files)} > {MAX_FILES}")

    names = {resource_name(path, args.locale) for path in files}
    missing = sorted(REQUIRED_FILES - names)
    if missing:
        raise SystemExit(f"locale pack is missing required files: {', '.join(missing)}")

    locale_bytes = args.locale.encode("ascii")
    if len(locale_bytes) > 7:
        raise SystemExit("locale name is too long")
    locale_field = locale_bytes + b"\0" * (8 - len(locale_bytes))

    header_size = struct.calcsize(HEADER_FORMAT)
    entry_size = struct.calcsize(ENTRY_FORMAT)
    directory_offset = header_size
    payload_offset = header_size + len(files) * entry_size

    payload = bytearray()
    entries = bytearray()
    next_offset = payload_offset
    for path in files:
        name = resource_name(path, args.locale).encode("ascii")
        if len(name) > 31:
            raise SystemExit(f"resource name is too long: {name.decode()}")
        data = path.read_bytes()
        if not data.startswith(b"OggS") or b"OpusHead" not in data[:512]:
            raise SystemExit(f"resource is not Ogg Opus: {path}")
        name_field = name + b"\0" * (32 - len(name))
        entries.extend(struct.pack(ENTRY_FORMAT, name_field, next_offset, len(data)))
        payload.extend(data)
        next_offset += len(data)

    content_digest = hashlib.sha256(entries + payload).digest()
    total_size = payload_offset + len(payload)
    if total_size > PACK_SIZE:
        raise SystemExit(f"pack is too large: {total_size} > {PACK_SIZE}")

    header = struct.pack(
        HEADER_FORMAT,
        MAGIC,
        FORMAT_VERSION,
        locale_field,
        len(files),
        args.version,
        directory_offset,
        payload_offset,
        len(payload),
        total_size,
        content_digest,
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(header + entries + payload)
    print(
        f"locale pack built locale={args.locale} files={len(files)} "
        f"payload={len(payload)} total={total_size} output={args.output}"
    )


if __name__ == "__main__":
    main()
