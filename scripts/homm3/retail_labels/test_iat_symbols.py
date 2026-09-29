"""IAT labels must spell vendor stdcall imports as cl references them."""
import unittest

from homm3.retail_labels import iat


class UndecoratedImportSymbolTest(unittest.TestCase):
    def test_vendor_decorated_exports_take_bare_prefix(self):
        self.assertEqual(iat.undecorated_import_symbol("_SmackClose@4"),
                         "__imp__SmackClose@4")
        self.assertEqual(iat.undecorated_import_symbol("_AIL_startup@0"),
                         "__imp__AIL_startup@0")
        self.assertEqual(iat.undecorated_import_symbol("_BinkPause@8"),
                         "__imp__BinkPause@8")

    def test_c_and_cpp_names_keep_their_conventions(self):
        self.assertEqual(iat.undecorated_import_symbol("wsprintfA"),
                         "__imp__wsprintfA")
        self.assertEqual(
            iat.undecorated_import_symbol("??0CImmMouse@@QAE@XZ"),
            "__imp_??0CImmMouse@@QAE@XZ")


if __name__ == "__main__":
    unittest.main()
