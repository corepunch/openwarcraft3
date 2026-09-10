#!/usr/bin/env python3
"""Convert CAMTRACE lines copied from Warcraft III into analysis-friendly CSV."""

import argparse
import csv
import math
import re
import sys


FIELDS = (
    "n", "t", "label", "tx", "ty", "tz", "ex", "ey", "ez",
    "dist", "aoa", "rot", "fov", "roll", "zoff", "farz",
    "dx", "dy", "dz", "horizontal_distance", "eye_target_distance",
)
TOKEN = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)=([^\s]+)")


def rows(stream):
    for line in stream:
        pos = line.find("CAMTRACE ")
        if pos < 0:
            continue
        values = dict(TOKEN.findall(line[pos:]))
        if not all(key in values for key in FIELDS[:16]):
            continue
        try:
            row = {key: values[key] for key in FIELDS[:16]}
            tx, ty, tz = (float(values[key]) for key in ("tx", "ty", "tz"))
            ex, ey, ez = (float(values[key]) for key in ("ex", "ey", "ez"))
        except ValueError:
            continue
        dx, dy, dz = ex - tx, ey - ty, ez - tz
        row.update({
            "dx": dx,
            "dy": dy,
            "dz": dz,
            "horizontal_distance": math.hypot(dx, dy),
            "eye_target_distance": math.sqrt(dx * dx + dy * dy + dz * dz),
        })
        yield row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=argparse.FileType("r"), default=sys.stdin,
                        help="captured retail output, or stdin")
    parser.add_argument("-o", "--output", type=argparse.FileType("w"), default=sys.stdout,
                        help="CSV destination, or stdout")
    args = parser.parse_args()
    writer = csv.DictWriter(args.output, fieldnames=FIELDS, extrasaction="ignore")
    writer.writeheader()
    writer.writerows(rows(args.input))


if __name__ == "__main__":
    main()
