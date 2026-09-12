"""Check gameplay row IDs and implementation codes without retail data."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from check_wc3_ability_registry import read_ability_rows, read_registry

SLK = '''ID;PWXL;N;E
C;X1;Y1;K"alias"
C;X2;K"code"
C;X3;K"comments"
C;X1;Y2;K"AIh1"
C;X2;K"AIhe"
C;X3;K"Healing item"
E
'''
REGISTRY = '''{ STR_CmdMove, CAbilityMove, AB_COMMAND, SPELL_TARGET_NONE },
{ "AIhe", CAbilityItemHeal, AB_ITEM, SPELL_TARGET_NONE },
// TODO: AIh1 CAbilityNoop
'''


class AbilityRegistryTest(unittest.TestCase):
    def test_row_id_and_implementation_are_distinct(self):
        self.assertEqual(read_ability_rows(SLK.splitlines()), {"AIh1": "Healing item"})
        self.assertEqual(read_ability_rows(SLK.splitlines(), 2), {"AIhe": "Healing item"})

    def test_todos_count_as_listed_but_not_active(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "registry.c"
            path.write_text(REGISTRY)
            self.assertEqual(read_registry(path), {"AIhe", "AIh1"})
            self.assertEqual(read_registry(path, active_only=True), {"AIhe"})

    def test_cli_accepts_data_base_code_and_rejects_abstract_class(self):
        with tempfile.TemporaryDirectory() as tmp:
            slk, registry = Path(tmp) / "AbilityData.slk", Path(tmp) / "registry.c"
            slk.write_text(SLK)
            registry.write_text(REGISTRY)
            cmd = [sys.executable, str(ROOT / "tools/check_wc3_ability_registry.py"),
                   "--slk", str(slk), "--skills", str(registry), "--check-active"]
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            registry.write_text(REGISTRY + '{ "AAsm", CAbilitySimpleSpell, AB_SPELL, SPELL_TARGET_NONE },\n')
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertIn("Non-data active rawcodes: 1", result.stdout)
            self.assertIn("AAsm", result.stdout)


if __name__ == "__main__":
    unittest.main()
