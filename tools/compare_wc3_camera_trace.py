#!/usr/bin/env python3
"""Compare retail full-event camera samples with OpenRealm camera events."""

import argparse
import re
import sys

from retail_camera_trace import rows


FIELDS = ("tx", "ty", "tz", "ex", "ey", "ez", "dist", "aoa", "rot", "fov", "roll", "zoff", "farz")
CAMERA_AFTER = re.compile(r"^cinematic-camera-(\d+)-after$")


def read_rows(path):
    with open(path, encoding="utf-8") as stream:
        return list(rows(stream))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("retail", help="retail camtrace-full.txt or extracted preload wrapper")
    parser.add_argument("openrealm", help="OpenRealm CAMTRACE log")
    args = parser.parse_args()
    retail = [row for row in read_rows(args.retail) if CAMERA_AFTER.match(row["label"])]
    openrealm = read_rows(args.openrealm)
    count = min(len(retail), len(openrealm))
    if not count:
        print("no aligned camera samples", file=sys.stderr)
        return 1
    print("camera retail_t openrealm_t " + " ".join(f"d_{field}" for field in FIELDS))
    for index in range(count):
        rr, orow = retail[index], openrealm[index]
        delta = [float(orow[field]) - float(rr[field]) for field in FIELDS]
        print(f"{index + 1:02d} {float(rr['t']):8.3f} {float(orow['t']):10.3f} " +
              " ".join(f"{value:9.3f}" for value in delta))
    if len(retail) != len(openrealm):
        print(f"warning: retail={len(retail)} OpenRealm={len(openrealm)}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
