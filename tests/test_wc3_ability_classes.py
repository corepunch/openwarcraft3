"""Local binary integration checks: python3 tests/test_wc3_ability_classes.py.

Requires the original, untracked data/warcraft3demo/game.dll. Not part of the
redistributable fixture suite; never substitutes guessed data for that binary.
"""

from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "tools"))
import extract_wc3_ability_classes as extractor


class AbilityClassesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dll = ROOT / "data/warcraft3demo/game.dll"
        cls.data = cls.dll.read_bytes()
        cls.rows = extractor.extract(cls.data)

    def test_complete_class_inventory_and_known_offsets(self):
        abilities = {row.name for row in self.rows if row.name.startswith("CAbility")}
        exposed = {name.decode() for name in re.findall(rb"\.\?AV(CAbility\w*)@@\x00", self.data)}
        self.assertEqual(abilities, exposed)
        rows = {row.code: row for row in self.rows}
        self.assertEqual(len(rows), 526)
        self.assertEqual(len(abilities), 197)
        self.assertNotIn("ACat", rows)
        self.assertEqual(rows["aura"], extractor.Entry("aura", "CAbilityAura", "ABon", 0x221C89, 0x221C90, 0x221D50, 0x221D70, 0x56DE30))
        self.assertEqual(rows["AOwk"].name, "CAbilityWindWalk")
        self.assertEqual(rows["AOws"].name, "CAbilityStomp")
        self.assertEqual(rows["Awar"].name, "CAbilityWarStomp")
        self.assertEqual(rows["buff"].name, "CBuff")

    def test_reference_is_reproducible(self):
        saved = ROOT / "docs/games/warcraft-3/demo-ability-classes.txt"
        self.assertEqual(extractor.render(self.rows), saved.read_text())

    def test_cli_all_classes_and_bad_input_preserves_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            output, bad = Path(tmp) / "classes.txt", Path(tmp) / "bad.dll"
            cmd = [sys.executable, str(ROOT / "tools/extract_wc3_ability_classes.py")]
            result = subprocess.run(cmd + [str(self.dll), "--all-classes", "-o", str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output.read_text(), extractor.render(self.rows, all_classes=True))
            bad.write_bytes(self.data[:-1])
            saved = output.read_bytes()
            result = subprocess.run(cmd + [str(bad), "-o", str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertIn("unsupported DLL SHA-256", result.stderr)
            self.assertEqual(output.read_bytes(), saved)
            result = subprocess.run(cmd + [str(bad), "-o", str(bad)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertIn("output must differ", result.stderr)
            self.assertEqual(bad.read_bytes(), self.data[:-1])


if __name__ == "__main__":
    unittest.main()
