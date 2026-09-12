#!/usr/bin/env python3
r"""Compare AbilityData.slk rawcodes with the WC3 ability registry source.

The checker intentionally counts commented TODO entries as listed: they are
registry work items and must not be reported again as missing.

Examples:
  tools/check_wc3_ability_registry.py --mpq data/Warcraft\ III/War3.mpq
  tools/check_wc3_ability_registry.py --slk /tmp/AbilityData.slk
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


CELL_RE = re.compile(r"^C;(.*)$")
REGISTRY_RE = re.compile(r'^\s*(?://\s*(?:TODO:\s*)?)?\{\s*"([^"]+)"\s*,')


def slk_value(token: str) -> str:
    if not token.startswith("K"):
        return ""
    value = token[1:]
    if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
        return value[1:-1]
    return value


def read_ability_rows(lines: list[str]) -> dict[str, str]:
    rows: dict[str, dict[int, str]] = {}
    cur_y = 0
    for line in lines:
        match = CELL_RE.match(line.rstrip("\r\n"))
        if not match:
            continue
        x = y = None
        value = ""
        for token in match.group(1).split(";"):
            if token.startswith("X") and token[1:].isdigit():
                x = int(token[1:])
            elif token.startswith("Y") and token[1:].isdigit():
                y = int(token[1:])
            elif token.startswith("K"):
                value = slk_value(token)
        if y is not None:
            cur_y = y
        if x is not None and cur_y > 1:
            rows.setdefault(str(cur_y), {})[x] = value

    result = {}
    for row in rows.values():
        rawcode = row.get(1, "")
        comment = row.get(4, "")
        if not comment or comment.replace(".", "", 1).isdigit():
            comment = row.get(3, "")
        if len(rawcode) == 4:
            result.setdefault(rawcode, comment)
    return result


def read_registry(path: Path) -> set[str]:
    result = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        match = REGISTRY_RE.match(line)
        if match and len(match.group(1)) == 4:
            result.add(match.group(1))
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--slk", type=Path, action="append", help="AbilityData.slk; may be repeated")
    parser.add_argument("--mpq", type=Path, action="append", help="MPQ containing AbilityData.slk; may be repeated")
    parser.add_argument("--mpqtool", default="build/bin/mpqtool", help="mpqtool executable used with --mpq")
    parser.add_argument(
        "--skills",
        type=Path,
        default=Path("games/warcraft-3/game/skills/s_skills.c"),
        help="s_skills.c to inspect",
    )
    args = parser.parse_args()

    source = {}
    if args.mpq:
        for archive in args.mpq:
            result = subprocess.run([args.mpqtool, "-mpq", str(archive), "cat", "Units/AbilityData.slk"],
                                    check=True, capture_output=True, text=True)
            source.update(read_ability_rows(result.stdout.splitlines()))
    elif args.slk:
        for slk in args.slk:
            source.update(read_ability_rows(slk.read_text(encoding="utf-8", errors="replace").splitlines()))
    else:
        source = read_ability_rows(sys.stdin.readlines())
    registry = read_registry(args.skills)
    missing = sorted(set(source) - registry)

    print(f"AbilityData rows: {len(source)}")
    print(f"Registry rawcodes: {len(registry)}")
    print(f"Missing rawcodes: {len(missing)}")
    for rawcode in missing:
        comment = source[rawcode].strip() or "(no AbilityData comment)"
        print(f'// TODO: {{ "{rawcode}", &a_unknown }},  /* {comment} */')
    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
