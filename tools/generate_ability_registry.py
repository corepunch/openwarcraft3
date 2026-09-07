#!/usr/bin/env python3
"""Generate the WC3 ability registry grouped by AbilityStrings file."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

BEGIN = "    /* BEGIN GENERATED ABILITY STRINGS */"
END = "    /* END GENERATED ABILITY STRINGS */"
TODO_BEGIN = "    /* BEGIN GENERATED TODO ABILITIES */"
TODO_END = "    /* END GENERATED TODO ABILITIES */"
ENTRY = re.compile(r'^\s*\{\s*"([A-Za-z0-9]{4})",\s*&([^}]+)\}(.*)$')
TODO_ENTRY = re.compile(r'^\s*// TODO:\s*\{\s*"([A-Za-z0-9]{4})",\s*&([^}]+)\}(.*)$')
HEADER = re.compile(r"^\[([^]\r\n]{4})\]\s*$")
NAME = re.compile(r"^Name=(.*)$")
FALLBACK_NAMES = {"Agl2": "Gold Mine ability", "Acoi": "Couple Instant"}


def load_sources(strings_dir: Path):
    files = sorted(strings_dir.glob("*AbilityStrings.txt"))
    sources = {}
    for file_index, path in enumerate(files):
        order = 0
        rawcode = None
        display_name = None
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            header = HEADER.match(line)
            if header:
                if rawcode and rawcode not in sources:
                    sources[rawcode] = (file_index, order, display_name or rawcode, path.name)
                    order += 1
                rawcode = header.group(1)
                display_name = None
                continue
            name = NAME.match(line)
            if rawcode and name:
                display_name = name.group(1).strip()
        if rawcode and rawcode not in sources:
            sources[rawcode] = (file_index, order, display_name or rawcode, path.name)
    return [path.name for path in files], sources


def parse_entries(block: str, pattern):
    entries = []
    for line in block.splitlines():
        match = pattern.match(line)
        if match:
            comment = match.group(3).lstrip()
            if comment.startswith(","):
                comment = comment[1:].strip()
            if comment.startswith("//"):
                comment = comment[2:].strip()
            if comment.startswith("/*") and comment.endswith("*/"):
                comment = comment[2:-2].strip()
            entries.append((match.group(1), match.group(2).strip(), comment))
    return entries


def registry_region(text: str):
    begin = text.index(BEGIN)
    start = text.index("\n", begin) + 1
    finish = text.index(END, start)
    after_end = text.index("\n", finish) + 1
    todo_begin = text.index(TODO_BEGIN, after_end)
    todo_start = text.index("\n", todo_begin) + 1
    todo_finish = text.index(TODO_END, todo_start)
    todo_after_end = text.index("\n", todo_finish) + 1
    entries = parse_entries(text[start:finish], ENTRY)
    todo_entries = parse_entries(text[todo_start:todo_finish], TODO_ENTRY)
    if not entries:
        raise ValueError("generated registry contains no entries")
    return text[:begin], text[todo_after_end:], entries, todo_entries


def render_group(lines, entries, file_names, sources, todo=False):
    groups = {index: [] for index in range(len(file_names))}
    other = []
    for rawcode, handler, old_comment in entries:
        source = sources.get(rawcode)
        if source:
            file_index, order, display_name, _ = source
            groups[file_index].append((order, rawcode, handler, display_name))
        else:
                other.append((rawcode, handler, FALLBACK_NAMES.get(rawcode, old_comment or rawcode)))
    for file_index, file_name in enumerate(file_names):
        group = sorted(groups[file_index])
        if not group:
            continue
        lines.extend(["", f"    /* {file_name} */"])
        for _, rawcode, handler, display_name in group:
            prefix = "// TODO: " if todo else ""
            lines.append(f'    {prefix}{{ "{rawcode}", &{handler} }},  /* {display_name} */')
    if other:
        lines.extend(["", "    /* No AbilityStrings source file */"])
        for rawcode, handler, comment in sorted(other):
            prefix = "// TODO: " if todo else ""
            lines.append(f'    {prefix}{{ "{rawcode}", &{handler} }},  /* {comment} */')


def render(text: str, strings_dir: Path) -> str:
    prefix, suffix, entries, todo_entries = registry_region(text)
    file_names, sources = load_sources(strings_dir)
    for group_entries in (entries, todo_entries):
        rawcodes = {rawcode for rawcode, _, _ in group_entries}
        if len(rawcodes) != len(group_entries):
            raise ValueError("generated registry contains duplicate rawcodes")

    lines = [BEGIN]
    render_group(lines, entries, file_names, sources)
    lines.extend(["", END])
    lines.append(TODO_BEGIN)
    render_group(lines, todo_entries, file_names, sources, todo=True)
    lines.extend(["", TODO_END])
    return prefix + "\n".join(lines) + "\n" + suffix


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skills", type=Path, default=Path("games/warcraft-3/game/skills/s_skills.c"))
    parser.add_argument("--strings-dir", type=Path, default=Path("data/strings"))
    parser.add_argument("--write", action="store_true", help="rewrite the generated section")
    args = parser.parse_args()

    original = args.skills.read_text(encoding="utf-8")
    try:
        updated = render(original, args.strings_dir)
    except (OSError, ValueError) as error:
        print(f"generate_ability_registry: {error}", file=sys.stderr)
        return 1
    if args.write:
        if updated != original:
            args.skills.write_text(updated, encoding="utf-8")
        return 0
    if updated != original:
        print(f"{args.skills}: generated section is out of date", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
