"""Local binary integration checks: python3 tests/test_wc3_ability_classes.py.

Requires the original, untracked demo DLL and TFT 1.29.2 executable. Not part of the
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
        self.assertEqual(extractor.extract_helpers(self.data, self.rows), [])
        saved = ROOT / "docs/games/warcraft-3/demo-ability-classes.txt"
        self.assertEqual(extractor.render(self.rows), saved.read_text())
        saved = ROOT / "games/warcraft-3/demo-ability-classes.txt"
        self.assertEqual(extractor.render(self.rows, all_classes=True), saved.read_text())

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


class TFTClassesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.exe = ROOT / "data/Warcraft III/Warcraft III.exe"
        cls.data = cls.exe.read_bytes()
        cls.rows = extractor.extract(cls.data)
        cls.helpers = extractor.extract_helpers(cls.data, cls.rows)

    def test_helper_inventory(self):
        expected = [
            extractor.RTTIEntry("CAbilityCustomData", 0xCFC47C, 0x10FD87C),
            extractor.RTTIEntry("CAbilityDB", 0xCFFD1C, 0x110111C),
            extractor.RTTIEntry("CAbilityDatabase", 0xCFC3C8, 0x10FD7C8),
            extractor.RTTIEntry("CAbilityMetaDB", 0xCFF2B8, 0x11006B8),
        ]
        self.assertEqual(self.helpers, expected)
        full = extractor.render(self.rows, all_classes=True, helpers=self.helpers)
        filtered = extractor.render(self.rows, helpers=self.helpers)
        exposed = {name.decode() for name in re.findall(rb"\.\?AV(CAbility\w*)@@\x00", self.data)}
        dumped = set(re.findall(r'^\{ (?:"[^"]+"|NULL), "(CAbility[^"]*)"', full, re.M))
        self.assertEqual(dumped, exposed)
        for row in self.helpers:
            self.assertIn(f'{{ NULL, "{row.name}" }}', full)
            self.assertNotIn(row.name, filtered)
            self.assertEqual(self.data[row.offset - 8:row.offset], bytes.fromhex("88 db ef 00 00 00 00 00"))
        self.assertEqual(full.count('{ NULL, "CAbility'), 4)

    def test_inventory_and_registration_variants(self):
        rows = {row.code: row for row in self.rows}
        abilities = {row.name for row in self.rows if row.name.startswith("CAbility")}
        exposed = {name.decode() for name in re.findall(rb"\.\?AV(CAbility\w*)@@\x00", self.data)}
        helpers = {"CAbilityDatabase", "CAbilityDB", "CAbilityCustomData", "CAbilityMetaDB"}
        self.assertEqual(abilities, exposed - helpers)
        self.assertEqual(len(rows), 1076)
        self.assertEqual(len(abilities), 489)
        self.assertEqual(rows["aura"], extractor.Entry("aura", "CAbilityAura", "ABon", 0x790360, 0x78FE10, 0x78FE80, 0x78FEB0, 0xD1042C, 0x11B5B0C, 0xB35870, 0x18F9A))
        self.assertEqual(rows["ANcl"].name, "CAbilityChannel")
        self.assertEqual(rows["ANcl"].call, 0x898CE0)
        self.assertEqual(rows["AEbl"].name, "CAbilityBlink")
        self.assertEqual(rows["Bblo"].name, "CBuffBloodlust")
        # This registration pushes its parent directly instead of calling a getter.
        self.assertEqual(rows["+w3a"].parent, "+aga")
        self.assertEqual(rows["+w3a"].call, 0x28FFF)
        self.assertNotIn("ACat", rows)
        self.assertNotIn("+aga", rows)  # Explicitly documented unpooled root.

    def test_section_translation(self):
        pe = extractor.PE(self.data)
        for va, off in ((0x7F6360, 0x3F5760), (0xF36A70, 0xB35870), (0x111182C, 0xD1042C)):
            self.assertEqual(pe.offset(va), off)
            self.assertEqual(pe.address(off), va)
        with self.assertRaisesRegex(ValueError, "not file-backed"):
            pe.offset(0x11B5B0C)  # Descriptor lives in zero-filled memory, assigned at startup.

    def test_cli_and_saved_reference(self):
        saved = ROOT / "games/warcraft-3/tft-ability-classes.txt"
        self.assertEqual(saved.read_text(), extractor.render(self.rows, all_classes=True, helpers=self.helpers))
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "classes.txt"
            cmd = [sys.executable, str(ROOT / "tools/extract_wc3_ability_classes.py"), str(self.exe), "-o", str(out)]
            for opts in ([], ["--all-classes"]):
                result = subprocess.run(cmd + opts, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(out.read_text(), extractor.render(self.rows, all_classes=bool(opts), helpers=self.helpers))


if __name__ == "__main__":
    unittest.main()
