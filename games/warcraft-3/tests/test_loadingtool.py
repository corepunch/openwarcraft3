"""Exercise the real batch writer with native-path MPQ fixtures, independent of installed Warcraft data."""
import json
from pathlib import Path
import random
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
TOOL = ROOT / "build/bin/loadingtool"
MPQ = ROOT / "build/bin/mpqtool"


def run(*args):
    result = subprocess.run([str(arg) for arg in args], capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stderr + result.stdout
    return result.stdout


def main():
    with tempfile.TemporaryDirectory(prefix="loadingtool-") as tmp:
        data = Path(tmp)
        shutil.copyfile(ROOT / "build/tests/tests.mpq", data / "War3.mpq")
        maps = data / "Maps/Campaign"
        maps.mkdir(parents=True)
        path = maps / "Human02.w3m"
        # ROC W3I v18: header, four strings, camera bounds, area, flags, tileset,
        # background row, loading fields, prologue fields, empty players and forces.
        strings = lambda *values: b"".join(value.encode() + b"\0" for value in values)
        w3i = struct.pack("<3I", 18, 0, 0) + strings("TRIGSTR_1", "", "", "")
        w3i += bytes(48 + 8 + 4) + b"L" + struct.pack("<I", 0)
        w3i += strings("TRIGSTR_3", "", "TRIGSTR_2") + bytes(4) + strings("", "", "") + bytes(8)
        (data / "war3map.w3i").write_bytes(w3i)
        title, subtitle = 'Fixture "chapter" \\ é', "Chapter One"
        rng = random.Random(7)
        for text, shortened in [("Short body.", False), ("".join(rng.choices("abcdefghijklmnopqrstuvwxyz ", k=900)), True)]:
            wts = "".join(f"STRING {i}\n{{\n{value}\n}}\n" for i, value in enumerate([title, subtitle, text], 1))
            (data / "war3map.wts").write_text(wts)
            run(MPQ, "-mpq", path, "create", "8")
            run(MPQ, "-mpq", path, "pack", data / "war3map.w3i", "war3map.w3i", data / "war3map.wts", "war3map.wts")
            run(MPQ, "-mpq", data / "War3Patch.mpq", "pack", path, "Maps/Campaign/Human02.w3m")
            records = [json.loads(line) for line in run(TOOL, "-data", data, "-roc", "Human02").splitlines()]
            row, summary = records
            original = [entry["text"] for entry in row["original_texts"]]
            shown = [entry["text"] for entry in row["shown_texts"]]
            assert title in original and subtitle in original and text in original, original
            assert title in shown and subtitle in shown
            assert row["shortened"] == shortened
            assert row["sent_compressed_bytes"] <= 512
            assert summary["summary"]["maps"] == 1 and summary["summary"]["failed"] == 0
            if shortened:
                assert row["slots_without_shortening"] > 2 and text not in shown
                assert any(text.startswith(value) and value for value in shown)
            else:
                assert original == shown
        print("loadingtool: WTS resolution, title fallback, JSON escaping, size accounting, and displayed truncation passed")


if __name__ == "__main__":
    main()
