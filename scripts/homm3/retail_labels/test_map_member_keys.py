"""Public map members must join independently of the underlying tree."""
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.retail_labels import source
from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS

FIND = (
    '?find@?$map@VTCacheMapKey@ResourceManager@@PAVresource@@U?$less@VTCacheMapKey@ResourceMana'
    'ger@@@std@@V?$allocator@PAVresource@@@5@@std@@QAE?AViterator@?$_Tree@VTCacheMapKey@Resourc'
    'eManager@@U?$pair@$$CBVTCacheMapKey@ResourceManager@@PAVresource@@@std@@U_Kfn@?$map@VTCach'
    'eMapKey@ResourceManager@@PAVresource@@U?$less@VTCacheMapKey@ResourceManager@@@std@@V?$allo'
    'cator@PAVresource@@@5@@4@U?$less@VTCacheMapKey@ResourceManager@@@4@V?$allocator@PAVresourc'
    'e@@@4@@2@ABVTCacheMapKey@ResourceManager@@@Z'
)
INSERT = (
    '?insert@?$map@VTCacheMapKey@ResourceManager@@HU?$less@VTCacheMapKey@ResourceManager@@@2@V?'
    '$allocator@H@2@@std@@QAE?AU?$pair@Viterator@?$_Tree@VTCacheMapKey@ResourceManager@@U?$pair'
    '@$$CBVTCacheMapKey@ResourceManager@@H@2@U_Kfn@?$map@VTCacheMapKey@ResourceManager@@HU?$les'
    's@VTCacheMapKey@ResourceManager@@@2@V?$allocator@H@2@@2@U?$less@VTCacheMapKey@ResourceMana'
    'ger@@@2@V?$allocator@H@2@@std@@_N@2@ABU?$pair@$$CBVTCacheMapKey@ResourceManager@@H@2@@Z'
)

class MapMemberKeyTest(unittest.TestCase):
    def test_cstr_pointer_pair_constructor_keeps_both_types_and_overload(self):
        symbol = "??0?$pair@PBDPAVresource@@@std@@QAE@ABQBDABQAVresource@@@Z"
        self.assertEqual(source._demangle_key(symbol), "cstr_resource_pair@pair_ctor")
        self.assertEqual(source._demangle_key(symbol.replace("resource", "Archive")),
                         "cstr_archive_pair@pair_ctor")
        for other in (symbol.replace("ABQBDABQAVresource@@", "ABU01@"),
                      symbol.replace("PBD", "PAD", 1),
                      symbol.replace("ABQAVresource", "ABQAVArchive")):
            self.assertNotEqual(source._demangle_key(other),
                                "cstr_resource_pair@pair_ctor")

    def test_named_keys_and_overloads_are_distinct(self):
        for member, symbol in (("find", FIND), ("insert", INSERT)):
            with self.subTest(member=member):
                key = "tcachemapkey@map_" + member
                self.assertEqual(source._demangle_key(symbol), key)
                self.assertEqual(source._demangle_key(symbol.replace(
                    "TCacheMapKey", "OtherCacheKey")),
                    "othercachekey@map_" + member)
                self.assertEqual(source._demangle_key(symbol.replace(
                    "VTCacheMapKey", "UTCacheMapKey")), key)
                for other in (symbol.replace("?$map@", "?$multimap@", 1),
                              symbol.replace("@@QAE?A", "@@QBE?A", 1)):
                    self.assertNotEqual(source._demangle_key(other), key)
        hinted = INSERT.replace("QAE?AU?$pair@Viterator@", "QAE?AViterator@", 1)
        self.assertNotEqual(source._demangle_key(hinted), "tcachemapkey@map_insert")

    def test_equal_sized_public_layers_join_by_owner_and_member(self):
        symbols = (FIND, INSERT, FIND.replace("TCacheMapKey", "OtherCacheKey"))
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "resourcemanager.cpp"
            path.write_text(
                "VA_COMPGEN(0x0055d3b0, 86, MAP_FIND, TCacheMapKey)\n"
                "VA_COMPGEN(0x0055d380, 86, MAP_INSERT, TCacheMapKey)\n"
                "VA_COMPGEN(0x0055e3b0, 86, MAP_FIND, OtherCacheKey)\n")
            rows = source.scan_file(path, {0x15d3b0, 0x15d380, 0x15e3b0})
        groups = {source._demangle_key(name): [(name, 86)]
                  for name in reversed(symbols)}
        self.assertEqual(len(groups), 3)
        with mock.patch.object(source, "_base_authority_scan", return_value=(groups, {})):
            source.join_unit("resourcemanager", rows)
        expected = {0x15d3b0: FIND, 0x15d380: INSERT, 0x15e3b0: symbols[2]}
        for row in rows:
            self.assertEqual(row.get("joined"), expected[row["rva"]])
        for kind in ("MAP_FIND", "MAP_INSERT"):
            self.assertIn(kind, source.COMPGEN_KINDS)
            self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)
