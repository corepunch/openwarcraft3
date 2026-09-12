#!/usr/bin/env python3
"""Map WC3 ability registry entries to TFT class names and parent chains.

Parses tft-ability-classes.txt for the full CAbility* hierarchy, then reads
s_skills.c to classify each registered ability (active or TODO) against the
TFT class tree.

Examples:
  python3 tools/wc3_ability_class_audit.py
  python3 tools/wc3_ability_class_audit.py --format=rename   # emit a_* → CAbility* rename table
  python3 tools/wc3_ability_class_audit.py --format=parents  # emit parent-chain wiring for InitAbilities
  python3 tools/wc3_ability_class_audit.py --format=coverage # coverage summary
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

TFT_CLASSES = ROOT / "games" / "warcraft-3" / "tft-ability-classes.txt"
SKILLS_C = ROOT / "games" / "warcraft-3" / "game" / "skills" / "s_skills.c"

# { "Amor", "CAbilityMorph" },  /* parent="AAsp" ... */
TFT_RE = re.compile(
    r'^\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}\s*,?\s*/\*\s*parent="([^"]*)"'
)
# Also handle entries with NULL fourcc (unregistered helpers at the end)
TFT_NULL_RE = re.compile(r'^\{\s*NULL\s*,\s*"([^"]+)"\s*\}')

# Active: { "AHhb", &a_holylight },  /* Holy Light */
ACTIVE_RE = re.compile(r'^\s*\{\s*"([A-Za-z0-9+\-]{2,5})"\s*,\s*&(\w+)\s*(?:,\s*\.alias\s*=\s*true\s*)?\}')
# TODO:   // TODO: { "Ablo", &a_bloodlust },  /* Bloodlust */
TODO_RE = re.compile(r'^\s*//\s*TODO:\s*\{\s*"([A-Za-z0-9+\-]{2,5})"\s*,\s*&(\w+)\s*\}')
# Engine commands use STR_Cmd* instead of string literals
ENGINE_RE = re.compile(r'^\s*\{\s*STR_Cmd\w+\s*,\s*&(\w+)\s*\}')


def load_tft_classes(path: Path) -> dict[str, tuple[str, str]]:
    """Return {fourcc: (class_name, parent_fourcc)}."""
    classes: dict[str, tuple[str, str]] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        m = TFT_RE.match(line)
        if m:
            fourcc, classname, parent = m.group(1), m.group(2), m.group(3)
            classes[fourcc] = (classname, parent)
    return classes


def parent_chain(classes: dict[str, tuple[str, str]], fourcc: str) -> list[str]:
    """Return [fourcc, parent, grandparent, ...] up to root."""
    chain = []
    seen = set()
    cur = fourcc
    while cur and cur not in seen:
        seen.add(cur)
        chain.append(cur)
        if cur in classes:
            cur = classes[cur][1]
        else:
            break
    return chain


def load_registry(path: Path):
    """Return (active, todo) lists of (fourcc, varname, comment)."""
    active = []
    todo = []
    engine = []
    for line in path.read_text(encoding="utf-8").splitlines():
        m = ACTIVE_RE.match(line)
        if m:
            comment = ""
            cm = re.search(r'/\*\s*(.*?)\s*\*/', line)
            if cm:
                comment = cm.group(1)
            active.append((m.group(1), m.group(2), comment))
            continue
        m = TODO_RE.match(line)
        if m:
            comment = ""
            cm = re.search(r'/\*\s*(.*?)\s*\*/', line)
            if cm:
                comment = cm.group(1)
            todo.append((m.group(1), m.group(2), comment))
            continue
        m = ENGINE_RE.match(line)
        if m:
            engine.append(m.group(1))
    return active, todo, engine


def class_to_varname(classname: str) -> str:
    """CAbilityHolyBolt → CAbilityHolyBolt (already a valid C identifier)."""
    return classname


def format_coverage(classes, active, todo):
    """Print coverage summary."""
    active_fourccs = {fc for fc, _, _ in active}
    todo_fourccs = {fc for fc, _, _ in todo}
    all_ability_fourccs = {
        fc for fc, (cn, _) in classes.items()
        if cn.startswith("CAbility") and not fc.startswith("+") and not fc.startswith("B")
        and not fc.startswith("M") and not fc.startswith("X") and not fc.startswith("R")
    }

    implemented = active_fourccs & all_ability_fourccs
    todo_known = todo_fourccs & all_ability_fourccs
    missing = all_ability_fourccs - active_fourccs - todo_fourccs

    print(f"TFT ability classes: {len(all_ability_fourccs)}")
    print(f"Implemented:         {len(implemented)}")
    print(f"TODO (registered):   {len(todo_known)}")
    print(f"Missing (not in registry): {len(missing)}")
    print()
    if missing:
        print("Missing from registry:")
        for fc in sorted(missing):
            cn, par = classes[fc]
            print(f"  {fc:5s} {cn:45s} parent={par}")


def format_rename(classes, active, todo):
    """Print a_* → CAbility* rename mapping for implemented abilities."""
    # Deduplicate: multiple fourccs can map to same a_* variable
    seen_vars = {}
    for fourcc, varname, comment in active:
        if varname in seen_vars:
            continue
        if fourcc in classes:
            cn, par = classes[fourcc]
            seen_vars[varname] = (cn, fourcc, par, comment)
        else:
            seen_vars[varname] = (None, fourcc, None, comment)

    print(f"{'Current a_* name':40s} {'TFT CAbility* name':45s} {'FourCC':6s} {'Parent':6s} Comment")
    print("-" * 145)
    for varname in sorted(seen_vars):
        cn, fourcc, par, comment = seen_vars[varname]
        tft_name = cn or "(no TFT match)"
        par_str = par or ""
        print(f"{varname:40s} {tft_name:45s} {fourcc:6s} {par_str:6s} {comment}")


def format_parents(classes, active, todo):
    """Print parent-chain wiring C code for InitAbilities."""
    # Collect unique ability variables and their primary fourcc
    var_fourcc: dict[str, str] = {}
    for fourcc, varname, _ in active:
        if varname not in var_fourcc and fourcc in classes:
            var_fourcc[varname] = fourcc

    print("/* TFT parent-chain wiring — generated by wc3_ability_class_audit.py */")
    print("static void wire_ability_parents(void) {")
    for varname in sorted(var_fourcc):
        fourcc = var_fourcc[varname]
        cn, parent_fc = classes[fourcc]
        if parent_fc and parent_fc in classes:
            parent_cn, _ = classes[parent_fc]
            print(f"    {cn}.parent = &{parent_cn}; /* {fourcc} → {parent_fc} */")
    print("}")


def format_todo_tft(classes, active, todo):
    """Print TODO abilities mapped to their TFT class names."""
    seen = set()
    print(f"{'FourCC':6s} {'TFT Class Name':45s} {'Parent':6s} {'Current stub':30s} Comment")
    print("-" * 145)
    for fourcc, varname, comment in todo:
        if fourcc in seen:
            continue
        seen.add(fourcc)
        if fourcc in classes:
            cn, par = classes[fourcc]
            print(f"{fourcc:6s} {cn:45s} {par:6s} {varname:30s} {comment}")
        else:
            print(f"{fourcc:6s} {'(no TFT class)':45s} {'':6s} {varname:30s} {comment}")


def format_hierarchy(classes, active, todo):
    """Print the base class hierarchy tree."""
    # Find all base classes that serve as parents
    children: dict[str, list[str]] = {}
    for fc, (cn, par) in classes.items():
        children.setdefault(par, []).append(fc)

    active_fourccs = {fc for fc, _, _ in active}

    def print_tree(fc: str, depth: int = 0):
        if fc not in classes:
            return
        cn, par = classes[fc]
        status = "✓" if fc in active_fourccs else " "
        prefix = "  " * depth
        print(f"{prefix}{status} {fc:5s} {cn}")
        for child in sorted(children.get(fc, [])):
            if child.startswith("+") or child.startswith("B") or child.startswith("M") or child.startswith("X") or child.startswith("R"):
                continue
            print_tree(child, depth + 1)

    # Start from known roots
    for root in ["abil", "ABon", "powr"]:
        if root in classes:
            print_tree(root)
            print()


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tft", type=Path, default=TFT_CLASSES)
    parser.add_argument("--skills", type=Path, default=SKILLS_C)
    parser.add_argument("--format", choices=["rename", "parents", "coverage", "todo", "hierarchy"],
                        default="coverage")
    args = parser.parse_args()

    classes = load_tft_classes(args.tft)
    active, todo, engine = load_registry(args.skills)

    if args.format == "coverage":
        format_coverage(classes, active, todo)
    elif args.format == "rename":
        format_rename(classes, active, todo)
    elif args.format == "parents":
        format_parents(classes, active, todo)
    elif args.format == "todo":
        format_todo_tft(classes, active, todo)
    elif args.format == "hierarchy":
        format_hierarchy(classes, active, todo)


if __name__ == "__main__":
    main()
