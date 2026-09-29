"""VC6 DATA spellings derived from Clang's Microsoft-ABI names."""
import unittest

from homm3.core import msvc_names


class DataNameTests(unittest.TestCase):
    def test_reference_to_const_uses_vc6_storage_class(self):
        # cl 12.00 defines and imports every global reference with `A`;
        # Clang writes the referent's const (`B`).
        self.assertEqual(
            msvc_names.data('?g_spellTraits@@3AAY0FB@$$CBUSSpellTraits@@B',
                            internal=False, decorated=True),
            '?g_spellTraits@@3AAY0FB@$$CBUSSpellTraits@@A')
        self.assertEqual(
            msvc_names.data('?x@C@@2AAY01$$CBHB', internal=False, decorated=True),
            '?x@C@@2AAY01$$CBHA')

    def test_non_reference_storage_class_is_kept(self):
        for name in ('?g_generalText@@3PBVTTextResource@@B',
                     '?g_videoGameState@@3AAHA',
                     '?g_count@@3HB'):
            self.assertEqual(
                msvc_names.data(name, internal=False, decorated=True), name)


if __name__ == '__main__':
    unittest.main()
