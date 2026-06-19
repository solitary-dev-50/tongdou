#!/usr/bin/env python3
"""Verify ChipIntelli exported command IDs against Tong Dou firmware mapping."""

from __future__ import annotations

import argparse
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PACKAGE = (
    REPO_ROOT
    / "firmware"
    / "vendor"
    / "chipintelli"
    / "tongdou_ci13162p_20260619"
)


@dataclass(frozen=True)
class ExpectedCommand:
    command_id: int
    semantic_id: str
    meaning: str
    keywords: tuple[str, ...]


EXPECTED_COMMANDS = (
    ExpectedCommand(1, "0x0001", "wake word", ("你好铜豆", "小铜豆", "铜豆", "嗨铜豆")),
    ExpectedCommand(2, "0x0002", "quiet mode", ("闭嘴", "别说话", "安静模式")),
    ExpectedCommand(3, "0x0003", "snooze reminder", ("稍后提醒", "等会儿再提醒")),
    ExpectedCommand(4, "0x0004", "confirm reminder", ("知道了", "我知道了")),
    ExpectedCommand(5, "0x0005", "reminder due", ("开始提醒", "提醒我一下")),
    ExpectedCommand(6, "0x0006", "guard word", ("防误",)),
)


def cmd_info_path(package_dir: Path) -> Path:
    return package_dir / "CmdWordStructure" / "[60000]{cmd_info}.xlsx"


def xml_text(element: ElementTree.Element) -> str:
    return "".join(element.itertext())


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
        inline = cell.find("x:is", namespace)
        return xml_text(inline) if inline is not None else ""

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


def read_first_sheet(path: Path) -> list[list[object]]:
    namespace = {"x": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    with zipfile.ZipFile(path) as workbook:
        shared_strings = load_shared_strings(workbook)
        root = ElementTree.fromstring(workbook.read("xl/worksheets/sheet1.xml"))
        rows: list[list[object]] = []

        for row in root.findall(".//x:sheetData/x:row", namespace):
            values: list[object] = []
            for cell in row.findall("x:c", namespace):
                ref = cell.attrib.get("r", "")
                index = column_index(ref)
                while len(values) <= index:
                    values.append(None)
                values[index] = parse_cell(cell, shared_strings)
            rows.append(values)

        return rows


def load_rows(path: Path) -> list[dict[str, object]]:
    rows = read_first_sheet(path)
    header = tuple(str(value) for value in rows[0])

    parsed: list[dict[str, object]] = []
    for row in rows[1:]:
        if not any(value is not None for value in row):
            continue
        normalized = row + [None] * (len(header) - len(row))
        parsed.append({header[index]: normalized[index] for index in range(len(header))})
    return parsed


def command_rows(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    return [
        row
        for row in rows
        if isinstance(row.get("commandID"), int)
        and row.get("semanticID") not in (None, "0x00", "0xAA55")
        and isinstance(row.get("commandWord"), str)
    ]


def words_by_command(rows: list[dict[str, object]]) -> dict[int, set[str]]:
    grouped: dict[int, set[str]] = {}
    for row in rows:
        command_id = int(row["commandID"])
        word = str(row["commandWord"])
        grouped.setdefault(command_id, set()).add(word)
    return grouped


def semantic_by_command(rows: list[dict[str, object]]) -> dict[int, set[str]]:
    grouped: dict[int, set[str]] = {}
    for row in rows:
        command_id = int(row["commandID"])
        semantic_id = str(row["semanticID"])
        grouped.setdefault(command_id, set()).add(semantic_id)
    return grouped


def verify(package_dir: Path) -> int:
    path = cmd_info_path(package_dir)
    if not path.exists():
        print(f"missing cmd_info file: {path}", file=sys.stderr)
        return 2

    rows = command_rows(load_rows(path))
    actual_words = words_by_command(rows)
    actual_semantics = semantic_by_command(rows)
    expected_ids = {item.command_id for item in EXPECTED_COMMANDS}
    actual_ids = set(actual_words)

    errors: list[str] = []
    for expected in EXPECTED_COMMANDS:
        words = actual_words.get(expected.command_id, set())
        semantics = actual_semantics.get(expected.command_id, set())

        missing_words = set(expected.keywords) - words
        if missing_words:
            errors.append(
                f"commandID {expected.command_id} ({expected.meaning}) missing words: "
                + ", ".join(sorted(missing_words))
            )

        if semantics != {expected.semantic_id}:
            errors.append(
                f"commandID {expected.command_id} ({expected.meaning}) semanticID mismatch: "
                f"expected {expected.semantic_id}, got {sorted(semantics)}"
            )

    extra_ids = actual_ids - expected_ids
    if extra_ids:
        errors.append(f"unexpected commandID values: {sorted(extra_ids)}")

    print(f"voice model: {package_dir}")
    print(f"cmd_info: {path}")
    print()
    for expected in EXPECTED_COMMANDS:
        words = sorted(actual_words.get(expected.command_id, set()))
        print(
            f"{expected.command_id}: {expected.meaning} "
            f"{sorted(actual_semantics.get(expected.command_id, set()))} -> "
            + " / ".join(words)
        )

    if errors:
        print()
        print("FAILED")
        for error in errors:
            print(f"- {error}")
        return 1

    print()
    print("OK: voice model command IDs match firmware mapping.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify ChipIntelli voice model command IDs.")
    parser.add_argument(
        "package_dir",
        nargs="?",
        type=Path,
        default=DEFAULT_PACKAGE,
        help="ChipIntelli exported package directory.",
    )
    args = parser.parse_args()
    return verify(args.package_dir.resolve())


if __name__ == "__main__":
    raise SystemExit(main())
