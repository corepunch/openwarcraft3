#!/usr/bin/env python3
"""Compare Warsmash and OpenRealm WC3_UI_TRACE layout logs.

OpenRealm emits FDF/asset provenance at stage=source and solved client geometry
at stage=final.  This tool joins those records by frame_number, then compares
frames to Warsmash stage=final records by FDF frame name.
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional


QUOTED_FIELD_RE = re.compile(r'([A-Za-z0-9_]+)="([^"]*)"')
PLAIN_FIELD_RE = re.compile(r'([A-Za-z0-9_]+)=([^\s"]+)')
RECT_RE = re.compile(
    r"rect_fdf_tl=\(([-+0-9.eE]+),([-+0-9.eE]+),([-+0-9.eE]+),([-+0-9.eE]+)\)"
)


@dataclass
class TraceRecord:
    fields: dict[str, str]
    rect: Optional[tuple[float, float, float, float]]


@dataclass
class OpenRealmTraceStats:
    source: int = 0
    final: int = 0
    local: int = 0
    joined: int = 0


def parse_trace(path: Path) -> Iterable[TraceRecord]:
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if "WC3_UI_TRACE " not in line:
                continue
            fields = dict(PLAIN_FIELD_RE.findall(line))
            fields.update(dict(QUOTED_FIELD_RE.findall(line)))
            rect_match = RECT_RE.search(line)
            rect = tuple(map(float, rect_match.groups())) if rect_match else None
            yield TraceRecord(fields=fields, rect=rect)


def warsmash_is_fdf_backed(record: TraceRecord) -> bool:
    source = record.fields.get("source", "")
    return bool(source and source not in ("runtime", "<unknown>", "-"))


def load_warsmash(path: Path) -> dict[str, TraceRecord]:
    result: dict[str, TraceRecord] = {}
    for record in parse_trace(path):
        if record.fields.get("engine") != "warsmash" or record.fields.get("stage") != "final":
            continue
        frame = record.fields.get("frame")
        if not frame or not record.rect:
            continue
        previous = result.get(frame)
        # Runtime text/render helper objects can reuse the owning FDF frame's
        # name. Prefer the FDF-backed frame so geometry is compared like-for-like.
        if previous and warsmash_is_fdf_backed(previous) and not warsmash_is_fdf_backed(record):
            continue
        result[frame] = record
    return result


def openrealm_key(record: TraceRecord) -> tuple[str, str, str]:
    return (
        record.fields.get("layer", "-"),
        record.fields.get("generation", "-"),
        record.fields.get("frame_number", ""),
    )


def load_openrealm(path: Path) -> tuple[dict[str, TraceRecord], OpenRealmTraceStats]:
    source_by_number: dict[tuple[str, str, str], TraceRecord] = {}
    final_by_number: dict[tuple[str, str, str], TraceRecord] = {}
    local_by_name: dict[str, TraceRecord] = {}
    stats = OpenRealmTraceStats()

    for record in parse_trace(path):
        if record.fields.get("engine") != "openrealm":
            continue
        stage = record.fields.get("stage")
        if stage == "source":
            stats.source += 1
            key = openrealm_key(record)
            if key[2]:
                source_by_number[key] = record
        elif stage == "final" and record.rect:
            stats.final += 1
            key = openrealm_key(record)
            if key[2]:
                final_by_number[key] = record
        elif stage == "local" and record.rect:
            stats.local += 1
            frame = record.fields.get("frame")
            if frame:
                local_by_name[frame] = record

    result: dict[str, TraceRecord] = dict(local_by_name)
    for key, source in source_by_number.items():
        final = final_by_number.get(key)
        if not final or not final.rect:
            continue
        frame = source.fields.get("frame")
        if not frame:
            continue
        merged = dict(source.fields)
        merged.update(final.fields)
        merged["frame"] = frame
        # Preserve source-stage type/name/provenance fields when final only has numeric type.
        for key in ("type", "source", "inherits", "inherits_source", "parent", "file",
                    "backdrop_bg", "backdrop_edge", "font"):
            if key in source.fields:
                merged[key] = source.fields[key]
        result[frame] = TraceRecord(fields=merged, rect=final.rect)
        stats.joined += 1
    return result, stats


def validate_capture(warsmash: dict[str, TraceRecord], openrealm: dict[str, TraceRecord],
                     stats: OpenRealmTraceStats) -> None:
    if not warsmash:
        raise SystemExit(
            "No Warsmash stage=final WC3_UI_TRACE records were found. "
            "Capture stderr with WARSMASH_UI_LAYOUT_TRACE=1 enabled."
        )
    if openrealm:
        return
    detail = (
        f"OpenRealm trace is unusable: source={stats.source} final={stats.final} "
        f"local={stats.local} joined={stats.joined}."
    )
    if stats.final and not stats.source:
        raise SystemExit(
            detail + " Final client rectangles were captured, but the named FDF source records "
            "were not. Put '+set ui_layout_trace 1' before '+map ...' so tracing is enabled "
            "before the HUD is serialized."
        )
    if stats.source and not stats.final:
        raise SystemExit(
            detail + " Named FDF source records were captured, but no client-solved rectangles "
            "were found. Capture the OpenRealm process stderr through the rendered gameplay frame."
        )
    if stats.source and stats.final:
        raise SystemExit(
            detail + " Source and final records exist but no (layer, generation, frame_number) keys joined. "
            "Use source/final records from the same single-client OpenRealm run."
        )
    raise SystemExit(
        detail + " No OpenRealm WC3_UI_TRACE records were found. Start with "
        "'+set ui_layout_trace 1' before '+map ...' and redirect stderr to the log."
    )


def fmt(value: Optional[float]) -> str:
    return "" if value is None else f"{value:.6f}"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compare WC3 UI rectangles from Warsmash and OpenRealm WC3_UI_TRACE logs."
    )
    parser.add_argument("warsmash_log", type=Path)
    parser.add_argument("openrealm_log", type=Path)
    parser.add_argument("--csv", type=Path, help="Write full comparison table as CSV")
    parser.add_argument(
        "--threshold",
        type=float,
        default=0.0005,
        help="Only print rows whose maximum absolute rectangle delta exceeds this value (default: 0.0005 FDF units)",
    )
    args = parser.parse_args()

    warsmash = load_warsmash(args.warsmash_log)
    openrealm, openrealm_stats = load_openrealm(args.openrealm_log)
    validate_capture(warsmash, openrealm, openrealm_stats)
    print(
        f"OpenRealm trace: source={openrealm_stats.source} final={openrealm_stats.final} "
        f"local={openrealm_stats.local} joined={openrealm_stats.joined} "
        f"named={len(openrealm)}.",
        file=sys.stderr,
    )
    names = sorted(set(warsmash) | set(openrealm))
    rows: list[dict[str, str]] = []

    for name in names:
        w = warsmash.get(name)
        o = openrealm.get(name)
        wr = w.rect if w else None
        or_ = o.rect if o else None
        deltas = None
        if wr and or_:
            deltas = tuple(or_[i] - wr[i] for i in range(4))
        row = {
            "frame": name,
            "status": "both" if w and o else ("warsmash_only" if w else "openrealm_only"),
            "warsmash_x": fmt(wr[0] if wr else None),
            "warsmash_y": fmt(wr[1] if wr else None),
            "warsmash_w": fmt(wr[2] if wr else None),
            "warsmash_h": fmt(wr[3] if wr else None),
            "openrealm_x": fmt(or_[0] if or_ else None),
            "openrealm_y": fmt(or_[1] if or_ else None),
            "openrealm_w": fmt(or_[2] if or_ else None),
            "openrealm_h": fmt(or_[3] if or_ else None),
            "dx": fmt(deltas[0] if deltas else None),
            "dy": fmt(deltas[1] if deltas else None),
            "dw": fmt(deltas[2] if deltas else None),
            "dh": fmt(deltas[3] if deltas else None),
            "warsmash_source": w.fields.get("source", "") if w else "",
            "openrealm_source": o.fields.get("source", "") if o else "",
            "warsmash_file": w.fields.get("file", "") if w else "",
            "openrealm_file": o.fields.get("file", "") if o else "",
        }
        rows.append(row)

    fieldnames = list(rows[0].keys()) if rows else ["frame", "status"]
    if args.csv:
        with args.csv.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)

    mismatches = 0
    for row in rows:
        if row["status"] != "both":
            continue
        # A 0x0 structural frame has no rendered or hit-test area; its origin is
        # under-constrained and can legitimately differ between solvers. Keep
        # raw coordinates in CSV, but do not count origin-only differences.
        if (abs(float(row["warsmash_w"])) <= args.threshold and
                abs(float(row["warsmash_h"])) <= args.threshold and
                abs(float(row["openrealm_w"])) <= args.threshold and
                abs(float(row["openrealm_h"])) <= args.threshold):
            continue
        delta_values = [abs(float(row[k])) for k in ("dx", "dy", "dw", "dh")]
        if max(delta_values) <= args.threshold:
            continue
        mismatches += 1
        print(
            f'{row["frame"]}: dx={row["dx"]} dy={row["dy"]} '
            f'dw={row["dw"]} dh={row["dh"]} '
            f'Warsmash={row["warsmash_source"] or "-"} '
            f'OpenRealm={row["openrealm_source"] or "-"}'
        )

    matched = sum(1 for row in rows if row["status"] == "both")
    warsmash_only = sum(1 for row in rows if row["status"] == "warsmash_only")
    openrealm_only = sum(1 for row in rows if row["status"] == "openrealm_only")
    print(
        f"Compared {matched} matched frames; {mismatches} exceeded {args.threshold:.6f} FDF units; "
        f"Warsmash-only={warsmash_only} OpenRealm-only={openrealm_only} union={len(names)}.",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
