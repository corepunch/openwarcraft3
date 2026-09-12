#!/usr/bin/env python3
"""Extract the Warcraft III demo's registered FOURCC -> C++ ability classes.

Uses only Python's standard library. Offsets are for the SHA-256 below; other
builds are rejected. See docs/games/warcraft-3/demo-ability-classes.md.
"""

import argparse
from dataclasses import dataclass
import hashlib
from pathlib import Path
import re
import struct
import sys


SHA256 = "286823c37a1083e91f07d040e46a9df7af4c4952e01fcbba460589bd4e297654"
IMAGE_BASE = 0x6F000000
TEXT_START, TEXT_END = 0x1000, 0x4EB0E8  # File offsets; .text RVA equals its file offset.
DATA_START, DATA_END = 0x546000, 0x59C000  # File-backed .data, including RTTI strings.
REGISTER = 0x73450  # File offset of the common class registration function.
DYNAMIC_CALLS = {0x712BF, 0x712DF}  # Agent root registrations with computed IDs, not abilities.
CALL = re.compile(rb"(?=\xe8)")
REG_ARGS = struct.Struct("<BIBIBIBIBI")  # push pool, push destroy, push create, mov edx,parent, mov ecx,id.
RTTI = re.compile(rb"\.\?AV([A-Za-z_][A-Za-z_0-9]*)@@\x00")


@dataclass(frozen=True)
class Entry:
    code: str
    name: str
    parent: str
    call: int
    create: int
    destroy: int
    pool: int
    rtti: int


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def fourcc(value):
    # The x86 immediate is little endian, but WC3's numeric ID reads high byte first.
    return value.to_bytes(4, "big").decode("ascii")


# Reject unknown builds before interpreting pointers using this demo's section layout.
def extract(data):
    digest = hashlib.sha256(data).hexdigest()
    if digest != SHA256:
        raise ValueError(f"unsupported DLL SHA-256 {digest}; expected {SHA256}")
    rows, calls, dynamic = [], set(), set()
    # Lookahead keeps overlapping E8 bytes visible, even when they occur inside an immediate.
    for match in CALL.finditer(data, TEXT_START, TEXT_END):
        off = match.start()
        if off + 5 + struct.unpack_from("<i", data, off + 1)[0] != REGISTER:
            continue
        calls.add(off)
        if off in DYNAMIC_CALLS:
            dynamic.add(off)
            continue
        args = REG_ARGS.unpack_from(data, off - REG_ARGS.size)
        if args[::2] != (0x68, 0x68, 0x68, 0xBA, 0xB9):
            raise ValueError(f"unrecognized registration at file offset 0x{off:08X}")
        pool, destroy, create, parent, code = args[1::2]
        pool, destroy, create = (va - IMAGE_BASE for va in (pool, destroy, create))
        if not all(TEXT_START <= ptr < TEXT_END for ptr in (pool, destroy, create)):
            raise ValueError(f"invalid callbacks at file offset 0x{off:08X}")
        # Pool cleanup passes the class's RTTI name to the allocator, as does its factory.
        if data[pool:pool + 3] != b"\x6a\xfe\x68":
            raise ValueError(f"unrecognized pool cleanup at file offset 0x{pool:08X}")
        rtti = u32(data, pool + 3) - IMAGE_BASE
        if not DATA_START <= rtti < DATA_END:
            raise ValueError(f"invalid RTTI pointer at file offset 0x{pool:08X}")
        name = RTTI.match(data, rtti, DATA_END)
        if not name or data[pool:pool + 7] not in data[create:create + 24]:
            raise ValueError(f"factory/pool RTTI mismatch at file offset 0x{off:08X}")
        rows.append(Entry(fourcc(code), name[1].decode("ascii"), fourcc(parent), off, create, destroy, pool, rtti))
    if len(calls) != 528 or len(rows) != 526 or dynamic != DYNAMIC_CALLS:
        raise ValueError(f"incomplete registry: {len(calls)} calls, {len(rows)} static registrations")
    if len({row.code for row in rows}) != len(rows):
        raise ValueError("duplicate registered FOURCC")
    if sum(row.name.startswith("CAbility") for row in rows) != 197:
        raise ValueError("incomplete ability registry (expected 197 CAbility classes)")
    return sorted(rows, key=lambda row: row.code)


# Keep the mapping readable like s_skills.c, with enough evidence to inspect each row.
def render(rows, all_classes=False):
    rows = [row for row in rows if all_classes or row.name.startswith("CAbility")]
    lines = [
        "/* Warcraft III demo Game.dll: extracted static class registrations.",
        f" * SHA-256: {SHA256}",
        f" * Image base: 0x{IMAGE_BASE:08X}; register: file 0x{REGISTER:08X}, VA 0x{IMAGE_BASE + REGISTER:08X}.",
        f" * {len(rows)} entries; IDs are case sensitive; offsets below are hexadecimal FILE offsets.",
        " * These are registered type IDs, including internal base types, not all AbilityData.slk aliases.",
        " * ACat is not registered in this DLL; aura is registered as CAbilityAura.",
        f" * Reproduce: python3 tools/extract_wc3_ability_classes.py data/warcraft3demo/game.dll{' --all-classes' if all_classes else ''} -o <output.txt>",
        " */",
        "",
    ]
    for row in rows:
        entry = f'{{ "{row.code}", "{row.name}" }},'
        lines.append(f'{entry:<54} /* parent="{row.parent}" call={row.call:08X} create={row.create:08X} destroy={row.destroy:08X} pool={row.pool:08X} rtti={row.rtti:08X} */')
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dll", type=Path, help="original Warcraft III demo Game.dll")
    parser.add_argument("-o", "--output", type=Path, required=True, help="reference .txt file to write")
    parser.add_argument("--all-classes", action="store_true", help="include all 526 static classes, including buffs")
    args = parser.parse_args()
    try:
        if args.dll.resolve() == args.output.resolve():
            raise ValueError("output must differ from the input DLL")
        rows = extract(args.dll.read_bytes())
        output = render(rows, args.all_classes)
        args.output.write_text(output, encoding="utf-8")
    except (OSError, ValueError, struct.error) as exc:
        print(f"extract_wc3_ability_classes: {exc}", file=sys.stderr)
        return 1
    print(f"Wrote {len(rows) if args.all_classes else 197} mappings to {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
