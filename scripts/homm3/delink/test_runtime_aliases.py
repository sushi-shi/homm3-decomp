import tempfile
import unittest
from pathlib import Path


class RuntimeAliasIdentityTest(unittest.TestCase):
    def test_alias_rows_become_library_identities(self):
        from homm3.delink.reloc_pairing import runtime_alias_identities
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "runtime-aliases.tsv"
            path.write_text("# generated\n"
                            "rva\tname\tlibrary\tmember\n"
                            "0x108ab0\t?DeleteDC@CDC@@QAEHXZ\tNAFXCW\twingdi.obj\n")
            rows = runtime_alias_identities(path)
        self.assertEqual(rows, [(0x108ab0, "?DeleteDC@CDC@@QAEHXZ", "library",
                                 "identical-code fold of NAFXCW wingdi.obj", "")])

    def test_missing_table_proves_nothing(self):
        from homm3.delink.reloc_pairing import runtime_alias_identities
        self.assertEqual(runtime_alias_identities(Path("/nonexistent/runtime-aliases.tsv")), [])


if __name__ == "__main__":
    unittest.main()
