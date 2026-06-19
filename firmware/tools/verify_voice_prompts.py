#!/usr/bin/env python3
"""Verify ChipIntelli TTS prompt assets against Tong Dou voice line IDs."""

from __future__ import annotations

import argparse
import re
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PACKAGE = REPO_ROOT / "firmware" / "vendor" / "chipintelli" / "tongdou_tts_20260619"


@dataclass(frozen=True)
class ExpectedPrompt:
    prompt_id: int
    name: str
    text: str


EXPECTED_PROMPTS = (
    ExpectedPrompt(101, "wake-greeting", "嗨，朋友，你的桌面上是不是缺少一个这样的小混蛋……和你一样的牛马搭子？"),
    ExpectedPrompt(102, "snooze-soft", "行吧，十分钟后再抓你。"),
    ExpectedPrompt(104, "confirm-reward", "收到。"),
    ExpectedPrompt(105, "quiet-reply", "行行行，我闭嘴。"),
    ExpectedPrompt(109, "reminder-due-nudge", "到点了，别装没听见，我都替你记着呢。"),
    ExpectedPrompt(112, "recognition-coffee-choke", "咳……刚才被咖啡呛了一下。你再说一遍，我这次肯定认真听。"),
)


def xml_text(element: ElementTree.Element | None) -> str:
    return "".join(element.itertext()) if element is not None else ""


def load_shared_strings(workbook: zipfile.ZipFile) -> list[str]:
    try:
        content = workbook.read("xl/sharedStrings.xml")
    except KeyError:
        return []

    root = ElementTree.fromstring(content)
    namespace = {"x": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    return [xml_text(item) for item in root.findall("x:si", namespace)]


def column_index(cell_ref: str) -> int:
    letters = ""
    for char in cell_ref:
        if char.isalpha():
            letters += char
        else:
            break

    index = 0
    for char in letters:
        index = index * 26 + (ord(char.upper()) - ord("A") + 1)
    return index - 1


def parse_cell(cell: ElementTree.Element, shared_strings: list[str]) -> object:
    namespace = {"x": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    cell_type = cell.attrib.get("t")

    if cell_type == "inlineStr":
        return xml_text(cell.find("x:is", namespace))

    value = cell.find("x:v", namespace)
    if value is None or value.text is None:
        return None

    if cell_type == "s":
        return shared_strings[int(value.text)]

    text = value.text
    try:
        numeric = float(text)
    except ValueError:
        return text

    if numeric.is_integer():
        return int(numeric)
    return numeric


def read_sheet(path: Path) -> list[list[object]]:
    namespace = {"x": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    with zipfile.ZipFile(path) as workbook:
        shared_strings = load_shared_strings(workbook)
        root = ElementTree.fromstring(workbook.read("xl/worksheets/sheet1.xml"))
        rows: list[list[object]] = []

        for row in root.findall(".//x:sheetData/x:row", namespace):
            values: list[object] = []
            for cell in row.findall("x:c", namespace):
                index = column_index(cell.attrib.get("r", ""))
                while len(values) <= index:
                    values.append(None)
                values[index] = parse_cell(cell, shared_strings)
            rows.append(values)

        return rows


def prompt_table_path(package_dir: Path) -> Path:
    matches = sorted(package_dir.glob("template_chinese_prompts_*.xlsx"))
    if not matches:
        raise FileNotFoundError(f"missing prompt table in {package_dir}")
    return matches[0]


def load_prompts(path: Path) -> dict[int, ExpectedPrompt]:
    rows = read_sheet(path)
    prompts: dict[int, ExpectedPrompt] = {}
    for row in rows[1:]:
        if len(row) < 3 or row[0] is None:
            continue
        prompt_id = int(row[0])
        prompts[prompt_id] = ExpectedPrompt(prompt_id, str(row[1]), str(row[2]))
    return prompts


def verify_audio_files(package_dir: Path, expected: ExpectedPrompt) -> list[str]:
    errors: list[str] = []
    for folder, suffix in (("mp3", "mp3"), ("opus", "opus"), ("speex", "speex"), ("src", "wav")):
        pattern = f"[{expected.prompt_id}]{expected.name}.{suffix}"
        if not (package_dir / folder / pattern).exists():
            errors.append(f"missing {folder}/{pattern}")
    return errors


def voice_line_pack_text() -> str:
    return (REPO_ROOT / "firmware" / "src" / "voice" / "VoiceLinePack.cpp").read_text(encoding="utf-8")


def verify_firmware_text(expected: ExpectedPrompt, source: str) -> list[str]:
    escaped_text = re.escape(expected.text)
    pattern = rf"return \{{{expected.prompt_id}, \"{escaped_text}\"\}};"
    if re.search(pattern, source) is None:
        return [f"VoiceLinePack.cpp missing exact clip {expected.prompt_id}: {expected.text}"]
    return []


def verify(package_dir: Path) -> int:
    try:
        table_path = prompt_table_path(package_dir)
    except FileNotFoundError as error:
        print(str(error), file=sys.stderr)
        return 2

    actual = load_prompts(table_path)
    firmware_source = voice_line_pack_text()
    errors: list[str] = []

    print(f"tts package: {package_dir}")
    print(f"prompt table: {table_path}")
    print()

    for expected in EXPECTED_PROMPTS:
        prompt = actual.get(expected.prompt_id)
        if prompt is None:
            errors.append(f"missing prompt ID {expected.prompt_id}")
            continue

        print(f"{prompt.prompt_id}: {prompt.name} -> {prompt.text}")

        if prompt.name != expected.name:
            errors.append(f"prompt {expected.prompt_id} name mismatch: expected {expected.name}, got {prompt.name}")
        if prompt.text != expected.text:
            errors.append(f"prompt {expected.prompt_id} text mismatch: expected {expected.text}, got {prompt.text}")
        errors.extend(verify_audio_files(package_dir, expected))
        errors.extend(verify_firmware_text(expected, firmware_source))

    extra_ids = set(actual) - {item.prompt_id for item in EXPECTED_PROMPTS}
    extra_ids.discard(65535)
    if extra_ids:
        errors.append(f"unexpected prompt IDs: {sorted(extra_ids)}")

    if errors:
        print()
        print("FAILED")
        for error in errors:
            print(f"- {error}")
        return 1

    print()
    print("OK: TTS prompt assets match firmware voice lines.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify ChipIntelli TTS prompt assets.")
    parser.add_argument(
        "package_dir",
        nargs="?",
        type=Path,
        default=DEFAULT_PACKAGE,
        help="ChipIntelli TTS exported package directory.",
    )
    args = parser.parse_args()
    return verify(args.package_dir.resolve())


if __name__ == "__main__":
    raise SystemExit(main())
