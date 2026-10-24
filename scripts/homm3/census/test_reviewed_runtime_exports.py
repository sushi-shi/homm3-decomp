"""Original public labels come only from complete reviewed runtime sections."""
from pathlib import Path
from types import SimpleNamespace
import tempfile
import struct
import unittest
from unittest.mock import patch

from homm3.census.libraries import reviewed_runtime_exports, reviewed_runtime_names
from homm3.verify.test_library_code import _coff, _symbol, CODE, DATA


def function_symbol(name, value, section, storage, base_type=0):
    record = bytearray(_symbol(name, value, section, storage))
    struct.pack_into('<H', record, 14, 0x20 | base_type)
    return bytes(record)


def fixture():
    raw = b'\xc3\x90\xc3\xc3'
    obj = _coff([('.text', CODE, raw, []), ('.data', DATA, bytes(4), [])],
                [function_symbol('_first', 0, 1, 2), function_symbol('_second', 2, 1, 2, base_type=4),
                 function_symbol('local', 3, 1, 3), function_symbol('end', 4, 1, 2),
                 _symbol('constant', 3, 1, 2),
                 _symbol('datum', 0, 2, 2)])
    mapped = SimpleNamespace(rva=0x1000, raw_offset=0, size=4, executable=True)
    image = SimpleNamespace(data=raw, section_of=lambda r: mapped if r == 0x1000 else None)
    row = dict(rva='0x1000', size='4', kind='code', library='LIBCMT.LIB',
               member='asm.obj', section='1', symbol='_first', ownership='whole-section')
    return obj, image, mapped, row


class ReviewedRuntimeExportsTest(unittest.TestCase):
    def run_exports(self, *, changes=None, functions=None, mutate=None):
        obj, image, mapped, row = fixture()
        row.update(changes or {})
        if mutate:
            mutate(image, mapped)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'library.lib'
            path.touch()
            with patch('homm3.verify.library_code.archive', return_value=({'asm.obj': obj}, {})):
                return reviewed_runtime_exports(image, [row], {'LIBCMT': path},
                                                functions if functions is not None else
                                                {0x1000: 1, 0x1002: 1, 0x1003: 1, 0x1004: 1})

    def test_only_original_public_code_entries_are_named(self):
        self.assertEqual(self.run_exports(), [(0x1000, '_first', 'LIBCMT', 'asm.obj'),
                                              (0x1002, '_second', 'LIBCMT', 'asm.obj')])

    def test_no_new_census_start_or_partial_body_is_invented(self):
        self.assertEqual(self.run_exports(functions={0x1000: 4}),
                         [(0x1000, '_first', 'LIBCMT', 'asm.obj')])
        self.assertEqual(self.run_exports(functions={0x1002: 3}), [])

    def test_alias_and_legacy_prefix_rows_do_not_export_ownership(self):
        for change in ({'kind': 'alias'}, {'kind': 'data'}, {'ownership': '-'}, {'size': '3'}):
            with self.subTest(change=change):
                self.assertEqual(self.run_exports(changes=change), [])

    def test_section_and_native_symbol_are_checked(self):
        for change in ({'section': '99'}, {'member': 'missing.obj'},
                       {'symbol': 'missing'}, {'section': '2', 'symbol': 'datum'},
                       {'library': 'UNKNOWN.LIB'}):
            with self.subTest(change=change):
                self.assertEqual(self.run_exports(changes=change), [])

    def test_nonzero_public_offset_does_not_relabel_the_section_base(self):
        exports = self.run_exports(changes={'symbol': '_second'})
        _obj, _image, _mapped, row = fixture()
        row['symbol'] = '_second'
        runtime, alternate = reviewed_runtime_names({0x1000, 0x1002}, [], [row],
                                                    exports=exports)
        self.assertEqual(runtime, [(0x1000, '_first', 'LIBCMT', 'asm.obj'),
                                   (0x1002, '_second', 'LIBCMT', 'asm.obj')])
        self.assertEqual(alternate, [])

    def test_whole_section_without_a_proven_export_supplies_no_name(self):
        _obj, _image, _mapped, row = fixture()
        self.assertEqual(reviewed_runtime_names({0x1000}, [], [row]), ([], []))

    def test_changed_native_fixed_bytes_or_short_reads_are_rejected(self):
        for payload in (b'\x90\x90\xc3\xc3', b'\xc3\x90\xc3'):
            self.assertEqual(self.run_exports(mutate=lambda image, mapped: setattr(image, 'data', payload)), [])

    def test_readable_executable_extent_is_required(self):
        for field, value in (('executable', False), ('size', 3)):
            self.assertEqual(self.run_exports(mutate=lambda image, mapped: setattr(mapped, field, value)), [])
        self.assertEqual(self.run_exports(changes={'rva': '0x999'}), [])

    def test_public_exports_merge_before_reviewed_aliases_without_duplicates(self):
        exports = self.run_exports()
        alias = dict(rva='0x1002', size='1', kind='alias', library='LIBCMT.LIB',
                     member='other.obj', section='1', symbol='_fold')
        runtime, alternate = reviewed_runtime_names({0x1000, 0x1002}, [exports[0]],
                                                     [alias], exports=exports)
        self.assertEqual(runtime, exports)
        self.assertEqual(alternate, [(0x1002, '_fold', 'LIBCMT', 'other.obj')])


if __name__ == '__main__':
    unittest.main()
