"""Retained VC6 list<TPoint> members from the RMG branch queue.

The erase overloads must join independently even with identical object sizes.
These are named COMDATs, not anonymous compiler thunks. The source claim's
owner must distinguish unrelated list instantiations as well as containers.
"""
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


LIST = "?$list@UTPoint@@V?$allocator@UTPoint@@@std@@@std@@"
MEMBERS = (
    ("LIST_DTOR", "??1" + LIST + "QAE@XZ"),
    ("LIST_INSERT_SINGLE", "?insert@" + LIST + "QAE?AViterator@12@V312@ABUTPoint@@@Z"),
    ("LIST_ERASE_ITERATOR", "?erase@" + LIST + "QAE?AViterator@12@V312@@Z"),
    ("LIST_ERASE_RANGE", "?erase@" + LIST + "QAE?AViterator@12@V312@0@Z"),
    ("LIST_BUYNODE", "?_Buynode@" + LIST + "IAEPAU_Node@12@PAU312@0@Z"),
)


class ListMemberKeysTest(unittest.TestCase):
    def test_observed_names_keep_distinct_keys_and_direct_symbol_admission(self):
        for kind, symbol in MEMBERS:
            with self.subTest(kind=kind):
                self.assertEqual(source._demangle_key(symbol), "tpoint@" + kind.lower())
                self.assertIn(kind, source.COMPGEN_KINDS)
                self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)

    def test_equal_size_shuffled_groups_join_by_signature(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "rmg.cpp"
            path.write_text("\n".join(
                f"VA_COMPGEN(0x{0x401000 + i * 0x100:x}, 0x45, {kind}, TPoint)"
                for i, (kind, _) in enumerate(MEMBERS)))
            rows = source.scan_file(path, {0x1000 + i * 0x100 for i in range(len(MEMBERS))})
        groups = {source._demangle_key(symbol): [(symbol, 0x45)]
                  for _, symbol in reversed(MEMBERS)}
        self.assertEqual(len(groups), len(MEMBERS))
        with mock.patch.object(source, "_base_authority_scan", return_value=(groups, {})):
            source.join_unit("rmg", rows)
        for row, (_, symbol) in zip(rows, MEMBERS):
            self.assertEqual(row.get("joined"), symbol)
            self.assertEqual(row["channel"], "src-VA+base")

    def test_second_element_and_class_spelling_keep_their_owner(self):
        for kind, symbol in MEMBERS:
            with self.subTest(kind=kind):
                symbol = symbol.replace("UTPoint@@", "VBranchPoint@@")
                self.assertEqual(source._demangle_key(symbol), "branchpoint@" + kind.lower())

    def test_other_container_and_insert_overloads_do_not_join_as_single_list_insert(self):
        vector = "?erase@?$vector@UTPoint@@V?$allocator@UTPoint@@@std@@@std@@QAEPAUTPoint@@PAU3@@Z"
        self.assertEqual(source._demangle_key(vector), "tpoint@vector_erase")
        for tail in ("QAEXViterator@12@IABUTPoint@@@Z", "QAEXViterator@12@PBU3@1@Z"):
            self.assertNotEqual(source._demangle_key("?insert@" + LIST + tail),
                                "tpoint@list_insert_single")
        custom = MEMBERS[1][1].replace("V?$allocator@UTPoint@@@std@@", "VBranchAllocator@@")
        self.assertNotEqual(source._demangle_key(custom), "tpoint@list_insert_single")


if __name__ == "__main__":
    unittest.main()
