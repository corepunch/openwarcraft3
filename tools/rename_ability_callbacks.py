#!/usr/bin/env python3
"""Rename one ability's local validate/execute callbacks; preview a diff unless --write is given."""

import argparse
import difflib
from pathlib import Path
import re
import sys


# Leave comments and literals intact, including strings containing C identifiers.
TOKENS = re.compile(r'''(?P<skip>//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|(?P<ident>\b[A-Za-z_]\w*\b)''')
FIELDS = {"validate": "Validate", "execute": "Execute"}


def rename_callbacks(source, ability):
    """Read the flat ability initializer, then rename C identifier tokens."""
    if not re.fullmatch(r"CAbility[A-Za-z0-9_]+", ability):
        raise ValueError("ability must be a CAbility name")
    code = TOKENS.sub(lambda m: re.sub(r"[^\n]", " ", m[0]) if m["skip"] else m[0], source)

    def initializer(name):
        found = re.search(r"\b" + re.escape(name) + r"\s*=\s*\{([^{}]*)\}\s*;", code)
        if not found:
            raise ValueError(f"no flat initializer for {name}; macro-generated definitions are not supported")
        return found[1]

    body = initializer(ability)
    names = {}
    found = False
    for field, suffix in FIELDS.items():
        entry = re.search(r"\." + field + r"\s*=\s*&?\s*(\w+)\s*(?=,|$)", body)
        if not entry or entry[1] == "NULL":
            continue
        found = True
        old, new = entry[1], f"{ability}_{suffix}"
        if old == new:
            continue
        if re.search(r"\b" + re.escape(new) + r"\b", code):
            raise ValueError(f"destination identifier already exists: {new}")
        if not re.search(r"\bstatic\s+[^;{}=]+?\b" + re.escape(old) + r"\s*\([^;{}]*\)\s*\{", code):
            raise ValueError(f"{old} must have a static function definition in this file")
        if old in names and names[old] != new:
            raise ValueError(f"{old} is assigned to multiple callback roles")
        names[old] = new
    if not found:
        raise ValueError(f"no validate/execute callbacks found for {ability}")
    return TOKENS.sub(lambda m: names.get(m[0], m[0]) if m["ident"] else m[0], source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path)
    parser.add_argument("--ability", required=True, help="explicit owner, e.g. CAbilityHolyBolt")
    parser.add_argument("--write", action="store_true", help="apply the previewed rename")
    args = parser.parse_args()
    try:
        with args.file.open(encoding="utf-8", newline="") as stream:
            source = stream.read()
        result = rename_callbacks(source, args.ability)
        if args.write:
            if result != source:
                with args.file.open("w", encoding="utf-8", newline="") as stream:
                    stream.write(result)
        else:
            sys.stdout.writelines(difflib.unified_diff(source.splitlines(True), result.splitlines(True), str(args.file), str(args.file)))
    except (OSError, ValueError) as error:
        parser.exit(1, f"rename-ability-callbacks: {error}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
