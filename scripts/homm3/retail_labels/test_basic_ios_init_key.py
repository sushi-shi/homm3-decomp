"""Keep the retained two-argument basic_ios initializer distinct from its callers."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


INIT = ("?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IAEXPAV"
        "?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z")
KEY = "char@basic_ios_init"


class BasicIosInitKeyTest(unittest.TestCase):
    def test_exact_stream_type_and_protected_two_argument_signature(self):
        self.assertEqual(source._demangle_key(INIT), KEY)
        for other in (
            INIT.replace("@D", "@G"),
            INIT.replace("_N@Z", "H@Z"),
            INIT.replace("_N@Z", "@Z"),
            INIT.replace("IAEX", "IBEX"),
            INIT.replace("IAEX", "QAEX"),
            INIT.replace("?init@", "?_Init@"),
            INIT.replace("basic_streambuf", "basic_stringbuf"),
            INIT.replace("char_traits", "other_traits"),
            INIT + "suffix",
            "?_Init@ios_base@std@@IAEXXZ",
        ):
            with self.subTest(symbol=other):
                self.assertNotEqual(source._demangle_key(other), KEY)

    def test_claim_joins_the_emitted_member_and_registers_normalization(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "quickherowindow.cpp"
            path.write_text("VA_COMPGEN(0x0052f440, 0x47, BASIC_IOS_INIT, char)\n")
            rows = source.scan_file(path, {0x12f440})
        groups = {KEY: [(INIT, 71)]}
        with mock.patch.object(source, "_base_authority_scan", return_value=(groups, {})):
            source.join_unit("quickherowindow", rows)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0].get("joined"), INIT)
        self.assertIn("BASIC_IOS_INIT", source.COMPGEN_KINDS)
        self.assertIn("BASIC_IOS_INIT", DIRECT_SYMBOL_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
