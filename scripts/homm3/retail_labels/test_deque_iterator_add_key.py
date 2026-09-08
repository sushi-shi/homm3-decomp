"""Bind the void deque advance helper separately from its returning operators."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


ADD = "?_Add@const_iterator@?$deque@HV?$allocator@H@std@@@std@@IAEXH@Z"
KEY = "int@deque_const_iterator_add"
OPERATOR = "??Yiterator@?$deque@HV?$allocator@H@std@@@std@@QAEAAV012@H@Z"


class DequeIteratorAddKeyTest(unittest.TestCase):
    def test_owner_allocator_access_and_void_signature_are_required(self):
        self.assertEqual(source._demangle_key(ADD), KEY)
        for other in (
            OPERATOR,
            OPERATOR.replace("Yiterator", "Yconst_iterator"),
            ADD.replace("const_iterator", "iterator"),
            ADD.replace("IAEX", "QAEX"),
            ADD.replace("IAEX", "IBEX"),
            ADD.replace("IAEX", "IAEH"),
            ADD.replace("IAEXH", "IAEXJ"),
            ADD.replace("IAEXH", "IAEXXZ"),
            ADD.replace("allocator@H", "allocator@J"),
            ADD.replace("deque@H", "deque@J"),
            ADD.replace("_Add", "_Subtract"),
            ADD + "suffix",
        ):
            with self.subTest(symbol=other):
                self.assertNotEqual(source._demangle_key(other), KEY)

    def test_claim_joins_by_identity_even_when_operator_has_same_size(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "combatcontrolsubwindow.cpp"
            path.write_text(
                "VA_COMPGEN(0x004491c0, 0x69, DEQUE_CONST_ITERATOR_ADD, int)\n")
            rows = source.scan_file(path, {0x491c0})
        groups = {source._demangle_key(OPERATOR): [(OPERATOR, 105)],
                  KEY: [(ADD, 105)]}
        with mock.patch.object(source, "_base_authority_scan", return_value=(groups, {})):
            source.join_unit("combatcontrolsubwindow", rows)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0].get("joined"), ADD)
        self.assertIn("DEQUE_CONST_ITERATOR_ADD", source.COMPGEN_KINDS)
        self.assertIn("DEQUE_CONST_ITERATOR_ADD", DIRECT_SYMBOL_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
