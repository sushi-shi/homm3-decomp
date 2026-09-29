import unittest
from types import SimpleNamespace
from unittest.mock import patch

from homm3.match import universe


class FunctionUniverseTest(unittest.TestCase):
    def test_reviewed_cleanup_is_excluded_from_game_denominator(self):
        rows = {universe.FUNCTIONS: [['0x100', '10']],
                universe.FUNCLETS: [], universe.RUNTIME_MAP: [],
                universe.ZLIB_MAP: [], universe.INIT_THUNKS: [['0x100', '-']]}
        image = SimpleNamespace(data=bytes(256), image_base=0x400000,
                                sections=[SimpleNamespace(name='.text', rva=0x100)],
                                blob=lambda _: bytes(10))
        with patch.object(universe, '_rows', side_effect=lambda path: rows[path]):
            categories, sizes = universe.classify(image)
        self.assertEqual(categories, {0x100: 'init-thunk'})
        self.assertEqual(sizes, {0x100: 10})

    def test_zlib_data_does_not_enter_function_denominator(self):
        for suffix in ([], ['func']):
            with self.subTest(schema=suffix):
                rows = {universe.FUNCTIONS: [['0x100', '1']],
                        universe.FUNCLETS: [], universe.RUNTIME_MAP: [],
                        universe.INIT_THUNKS: [],
                        universe.ZLIB_MAP: [['0x100', '1', '_crc32', 'crc32', *suffix],
                                           ['0x200', '1024', '_crc_table', 'crc32', 'data']]}
                image = SimpleNamespace(data=bytes(256), image_base=0x400000,
                                        sections=[SimpleNamespace(name='.text', rva=0x100)],
                                        blob=lambda _: b'\xc3')
                with patch.object(universe, '_rows', side_effect=lambda path: rows[path]):
                    categories, sizes = universe.classify(image)
                self.assertEqual(categories, {0x100: 'zlib'})
                self.assertEqual(sizes, {0x100: 1})


if __name__ == '__main__':
    unittest.main()
