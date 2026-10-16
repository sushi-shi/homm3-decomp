"""Pinned COFF evidence recovers embedded code pointers without guessing."""
import struct
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from homm3.census import find_relocs, libraries
from homm3.core.image import Section
from homm3.verify import library_code
from homm3.verify.test_library_code import BASE, CODE16, DATA, DIR32, REL32, _coff, _symbol

RVA = 0x1000


def fixture(*, flags=CODE16, second_type=DIR32, second_section=1):
    # jmp [ecx*4 + table], ret, two absolute entries, two ret targets.
    raw = bytes.fromhex('ff248d00000000c3') + bytes(8) + b'\xc3\xc3'
    obj = _coff(
        [('.text', flags, raw, [(3, 1, DIR32), (8, 2, second_type), (12, 3, DIR32)])],
        [_symbol('f', 0, 1, 2), _symbol('table', 8, 1, 3),
         _symbol('L0', 16, second_section, 3), _symbol('L1', 17, 1, 3)])
    sec = library_code.section('LIBX.LIB', 'x.obj', obj, 1)
    retail = bytearray(raw)
    for site, target in ((3, 8), (8, 16), (12, 17)):
        struct.pack_into('<I', retail, site, BASE + RVA + target)
    return sec, bytes(retail)


class InternalCodeRelocationsTest(unittest.TestCase):
    def test_dispatch_operand_and_embedded_entries_are_proven(self):
        sec, retail = fixture()
        self.assertEqual(libraries.internal_code_relocations(sec, retail, BASE + RVA), {
            3: (BASE + RVA + 8, 'table'),
            8: (BASE + RVA + 16, 'L0'),
            12: (BASE + RVA + 17, 'L1')})

    def test_one_wrong_fixed_byte_refuses_the_section(self):
        sec, retail = fixture()
        wrong = b'\xfe' + retail[1:]
        self.assertEqual(libraries.internal_code_relocations(sec, wrong, BASE + RVA), {})

    def test_one_wrong_linked_local_target_refuses_the_section(self):
        sec, retail = fixture()
        wrong = bytearray(retail)
        struct.pack_into('<I', wrong, 8, BASE + RVA + 17)
        self.assertEqual(libraries.internal_code_relocations(sec, bytes(wrong), BASE + RVA), {})

    def test_relative_and_external_fields_are_not_absolute_local_evidence(self):
        for kwargs in ({'second_type': REL32}, {'second_type': 7}, {'second_section': 0}):
            with self.subTest(**kwargs):
                sec, retail = fixture(**kwargs)
                self.assertEqual(set(libraries.internal_code_relocations(sec, retail, BASE + RVA)),
                                 {3, 12})

    def test_data_sections_and_short_reads_supply_no_sites(self):
        sec, retail = fixture(flags=DATA)
        self.assertEqual(libraries.internal_code_relocations(sec, retail, BASE + RVA), {})
        sec, retail = fixture()
        self.assertEqual(libraries.internal_code_relocations(sec, retail[:-1], BASE + RVA), {})

    def test_coff_addends_are_checked_against_the_linked_value(self):
        sec, retail = fixture()
        original = bytearray(sec.data)
        struct.pack_into('<i', original, 8, -1)
        sec.data = bytes(original)
        linked = bytearray(retail)
        struct.pack_into('<I', linked, 8, BASE + RVA + 15)
        sites = libraries.internal_code_relocations(sec, bytes(linked), BASE + RVA)
        self.assertEqual(sites[8], (BASE + RVA + 15, 'L0'))

    def test_local_addend_cannot_prove_a_target_outside_the_section(self):
        sec, retail = fixture()
        original, linked = bytearray(sec.data), bytearray(retail)
        struct.pack_into('<i', original, 8, 32)
        struct.pack_into('<I', linked, 8, BASE + RVA + 48)
        sec.data = bytes(original)
        self.assertEqual(libraries.internal_code_relocations(sec, bytes(linked), BASE + RVA), {})

    def test_unknown_relocation_width_refuses_the_section(self):
        sec, retail = fixture(second_type=10)
        self.assertEqual(libraries.internal_code_relocations(sec, retail, BASE + RVA), {})

    def test_only_reviewed_code_extents_produce_library_channel_sites(self):
        sec, retail = fixture()
        mapped = Section('.text', RVA, len(retail), len(retail), RVA, True)
        image = SimpleNamespace(
            image_base=BASE, data=bytes(RVA) + retail,
            section_of=lambda rva: mapped if RVA <= rva < RVA + len(retail) else None,
            rva_of=lambda address: address - BASE)
        row = dict(rva=hex(RVA), size=str(len(retail)), kind='code',
                   library='LIBX.LIB', member='x.obj', section='1', symbol='f')
        with tempfile.TemporaryDirectory() as tmp:
            archive = Path(tmp) / 'LIBX.LIB'
            archive.touch()
            with patch.object(library_code, 'archive', return_value=({'x.obj': sec.obj}, {})):
                sites = libraries.reviewed_code_sites(image, [row], {'LIBX': archive})
                self.assertEqual(set(sites), {RVA + 3, RVA + 8, RVA + 12})
                for site in sites.values():
                    self.assertEqual(site.channel, 'library-code')
                    self.assertEqual(find_relocs.target_class(image, site, {}, {RVA}), 'code-interior')
                for change in ({'kind': 'data'}, {'size': '17'}, {'section': '0'}):
                    self.assertEqual(libraries.reviewed_code_sites(
                        image, [dict(row, **change)], {'LIBX': archive}), {})
                self.assertEqual(libraries.reviewed_code_sites(image, [row], {}), {})


if __name__ == '__main__':
    unittest.main()
