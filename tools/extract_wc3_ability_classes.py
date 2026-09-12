#!/usr/bin/env python3
"""Extract registered FOURCC -> C++ classes from the WC3 demo or TFT 1.29.2.

Uses only Python's standard library. Offsets are for the SHA-256 values below; other
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
TFT_SHA256 = "a1950f17905b9cd7d5461d45e6723af36dda5304e12dba27a1fea85593b15f3f"
TFT_REGISTER = 0x3F5760  # File offset; VA 0x007F6360 in TFT 1.29.2.9231.
TFT_ROOT_CALL = 0x3F49A9  # Unpooled +aga root; computed IDs and no pool RTTI, like the demo roots.
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
    desc: int = 0
    table: int = 0
    init: int = 0


@dataclass(frozen=True)
class RTTIEntry:
    name: str
    offset: int
    va: int


class PE:
    # Section headers own VA/file conversion; TFT's section RVAs differ from raw offsets.
    def __init__(self, data):
        self.data = data
        pe = u32(data, 60)
        self.base = u32(data, pe + 52)
        count = struct.unpack_from("<H", data, pe + 6)[0]
        opt = struct.unpack_from("<H", data, pe + 20)[0]
        schema = struct.Struct("<8sIIII16x")
        self.sections = [schema.unpack_from(data, pe + 24 + opt + i * schema.size) for i in range(count)]

    def offset(self, va):
        for _, _, rva, size, raw in self.sections:
            if self.base + rva <= va < self.base + rva + size:
                return va - self.base - rva + raw
        raise ValueError(f"VA 0x{va:08X} is not file-backed")

    def section(self, name):
        for label, size, _, _, raw in self.sections:
            if label.rstrip(b"\0") == name:
                return raw, raw + size
        raise ValueError(f"missing PE section {name!r}")

    def address(self, off):
        for _, _, rva, size, raw in self.sections:
            if raw <= off < raw + size:
                return self.base + rva + off - raw
        raise ValueError(f"file offset 0x{off:08X} is outside PE sections")


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def fourcc(value):
    # The x86 immediate is little endian, but WC3's numeric ID reads high byte first.
    return value.to_bytes(4, "big").decode("ascii")


# Reject unknown builds before interpreting pointers using this demo's section layout.
def extract_demo(data):
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


# TFT passes a descriptor object whose startup-assigned vtable owns all three callbacks.
def extract_tft(data):
    pe = PE(data)
    start, end = pe.section(b".text")
    assign = {}
    for match in re.finditer(rb"(?=\xc7\x05)", data[start:end]):
        off = start + match.start()
        assign.setdefault(u32(data, off + 2), []).append((u32(data, off + 6), off))
    rows, calls = [], set()
    for match in CALL.finditer(data, start, end):
        off = match.start()
        if off + 5 + struct.unpack_from("<i", data, off + 1)[0] != TFT_REGISTER:
            continue
        calls.add(off)
        if off == TFT_ROOT_CALL:
            continue
        if bytes(data[off - k] for k in (16, 11, 6, 5)) == b"\x68\xe8\x50\x68":
            desc, code = u32(data, off - 15), u32(data, off - 4)
            getter = off - 6 + struct.unpack_from("<i", data, off - 10)[0]
            if data[getter] != 0xB8 or data[getter + 5] != 0xC3:
                raise ValueError(f"unrecognized parent getter at file offset 0x{getter:08X}")
            parent = u32(data, getter + 1)
        elif bytes(data[off - k] for k in (15, 10, 5)) == b"\x68\x68\x68":
            desc, parent, code = (u32(data, off - k) for k in (14, 9, 4))
        else:
            raise ValueError(f"unrecognized TFT registration at file offset 0x{off:08X}")
        writes = assign.get(desc, [])
        if len(writes) != 1:
            raise ValueError(f"descriptor VA 0x{desc:08X} has {len(writes)} vtable assignments")
        table, init = pe.offset(writes[0][0]), writes[0][1]
        create, destroy, pool = (pe.offset(u32(data, table + k)) for k in (0, 4, 8))
        if not all(start <= ptr < end for ptr in (create, destroy, pool)):
            raise ValueError(f"invalid TFT callbacks at file offset 0x{off:08X}")
        if data[pool:pool + 5] != b"\x6a\x01\x6a\xfe\x68":
            raise ValueError(f"unrecognized TFT pool cleanup at file offset 0x{pool:08X}")
        rtti = pe.offset(u32(data, pool + 5))
        name = RTTI.match(data, rtti)
        if not name:
            raise ValueError(f"invalid TFT RTTI at file offset 0x{rtti:08X}")
        rows.append(Entry(fourcc(code), name[1].decode("ascii"), fourcc(parent), off, create, destroy, pool, rtti, desc, table, init))
    if len(calls) != 1077 or len(rows) != 1076 or TFT_ROOT_CALL not in calls:
        raise ValueError(f"incomplete TFT registry: {len(calls)} calls, {len(rows)} static registrations")
    if len({row.code for row in rows}) != len(rows):
        raise ValueError("duplicate TFT registered FOURCC")
    if sum(row.name.startswith("CAbility") for row in rows) != 489:
        raise ValueError("incomplete TFT ability registry (expected 489 CAbility classes)")
    return sorted(rows, key=lambda row: row.code)


# Select only verified layouts; unknown binaries must never produce plausible partial output.
def extract(data):
    digest = hashlib.sha256(data).hexdigest()
    readers = {SHA256: extract_demo, TFT_SHA256: extract_tft}
    if digest not in readers:
        raise ValueError(f"unsupported DLL SHA-256 {digest}; expected demo {SHA256} or TFT {TFT_SHA256}")
    return readers[digest](data)


# Inventory the remaining ability-named RTTI without inventing factory IDs or callbacks.
def extract_helpers(data, rows):
    names = {row.name for row in rows}
    pe = PE(data)
    helpers = []
    for match in RTTI.finditer(data):
        name = match[1].decode("ascii")
        if name.startswith("CAbility") and name not in names:
            helpers.append(RTTIEntry(name, match.start(), pe.address(match.start())))
    return sorted(helpers, key=lambda row: (row.name, row.offset))


# Keep the mapping readable like s_skills.c, with enough evidence to inspect each row.
def render(rows, all_classes=False, helpers=()):
    tft = any(row.desc for row in rows)
    rows = [row for row in rows if all_classes or row.name.startswith("CAbility")]
    title = "Warcraft III TFT 1.29.2.9231" if tft else "Warcraft III demo Game.dll"
    digest = TFT_SHA256 if tft else SHA256
    base, reg, va = (0x400000, TFT_REGISTER, 0x7F6360) if tft else (IMAGE_BASE, REGISTER, IMAGE_BASE + REGISTER)
    source = '"data/Warcraft III/Warcraft III.exe"' if tft else "data/warcraft3demo/game.dll"
    lines = [
        f"/* {title}: extracted static class registrations.",
        f" * SHA-256: {digest}",
        f" * Image base: 0x{base:08X}; register: file 0x{reg:08X}, VA 0x{va:08X}.",
        f" * {len(rows)} entries; IDs are case sensitive; offsets below are hexadecimal FILE offsets.",
        " * These are registered type IDs, including internal base types, not all AbilityData.slk aliases.",
        " * ACat is not registered in this DLL; aura is registered as CAbilityAura.",
        f" * Reproduce: python3 tools/extract_wc3_ability_classes.py {source}{' --all-classes' if all_classes else ''} -o <output.txt>",
    ]
    if tft:
        lines.extend([
            " * desc_va is a runtime descriptor VA; table/init are its vtable/startup-assignment FILE offsets.",
            " * Excludes the unpooled +aga root registration at file 003F49A9 (no pool RTTI).",
        ])
    if all_classes and helpers:
        lines.append(f" * Plus {len(helpers)} unregistered CAbility RTTI entries in the helper appendix; NULL means no registered FOURCC.")
    lines.extend([" */", ""])
    for row in rows:
        entry = f'{{ "{row.code}", "{row.name}" }},'
        extra = f" desc_va={row.desc:08X} table={row.table:08X} init={row.init:08X}" if tft else ""
        lines.append(f'{entry:<54} /* parent="{row.parent}" call={row.call:08X} create={row.create:08X} destroy={row.destroy:08X} pool={row.pool:08X} rtti={row.rtti:08X}{extra} */')
    if all_classes and helpers:
        lines.extend(["", "/* Unregistered CAbility RTTI: database/helper classes, absent from the factory registry.",
                      " * rtti/type_desc are FILE offsets; rtti_va/type_desc_va are preferred virtual addresses.",
                      " * type_desc points to the two-DWORD MSVC TypeDescriptor header preceding the decorated name.", " */"])
        for row in helpers:
            entry = f'{{ NULL, "{row.name}" }},'
            lines.append(f'{entry:<54} /* rtti={row.offset:08X} rtti_va={row.va:08X} type_desc={row.offset - 8:08X} type_desc_va={row.va - 8:08X} decorated=".?AV{row.name}@@" */')
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dll", type=Path, help="Warcraft III demo Game.dll or TFT 1.29.2.9231 Warcraft III.exe")
    parser.add_argument("-o", "--output", type=Path, required=True, help="reference .txt file to write")
    parser.add_argument("--all-classes", action="store_true", help="include all static classes and unregistered ability RTTI (demo: 526; TFT: 1076 + 4 helpers)")
    args = parser.parse_args()
    try:
        if args.dll.resolve() == args.output.resolve():
            raise ValueError("output must differ from the input DLL")
        data = args.dll.read_bytes()
        rows = extract(data)
        helpers = extract_helpers(data, rows) if args.all_classes else []
        output = render(rows, args.all_classes, helpers)
        args.output.write_text(output, encoding="utf-8")
    except (OSError, ValueError, struct.error) as exc:
        print(f"extract_wc3_ability_classes: {exc}", file=sys.stderr)
        return 1
    count = sum(args.all_classes or row.name.startswith("CAbility") for row in rows)
    extra = f" and {len(helpers)} unregistered RTTI entries" if helpers else ""
    print(f"Wrote {count} mappings{extra} to {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
