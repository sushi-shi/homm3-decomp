"""Per-function verdicts for library and compiler-generated census rows."""

import struct
import unittest
from types import SimpleNamespace
from unittest import mock

from homm3.verify import generated_code
from homm3.verify.library_code import Contribution, Verdict

BASE = 0x400000
DIR32, REL32 = 6, 20


class FakePE:
    image_base = BASE

    def __init__(self, memory):
        self.memory = memory

    def read(self, rva, size):
        chunk = bytes(self.memory[rva:rva + size])
        return chunk if len(chunk) == size else None


def _verdict(rva, size, member, section, verdict='exact', kind='code'):
    v = Verdict(Contribution(rva, size, 'LIBCMT.LIB', member, section, f'_{member}', '-', kind))
    v.verdict = verdict
    return v


class RuntimeVerdicts(unittest.TestCase):
    def verdicts(self, census, contributions, memory=None):
        category = {rva: 'runtime' for rva in census}
        pe = FakePE(memory or bytearray(0x200))
        with mock.patch.object(generated_code, '_runtime_names', return_value={}):
            rows = generated_code.runtime_verdicts(pe, census, category, contributions)
        return {int(r['rva'], 16): r for r in rows}

    def test_function_inside_one_exact_section(self):
        rows = self.verdicts({0x10: 8}, [_verdict(0x10, 9, 'a.obj', 3)])
        self.assertEqual(rows[0x10]['verdict'], 'exact')
        self.assertEqual(rows[0x10]['sections'], 'a.obj#3')

    def test_function_spanning_adjacent_sections(self):
        rows = self.verdicts({0x10: 12}, [_verdict(0x10, 5, 'a.obj', 3),
                                          _verdict(0x15, 7, 'a.obj', 4)])
        self.assertEqual(rows[0x10]['verdict'], 'exact')
        self.assertEqual(rows[0x10]['sections'], 'a.obj#3;a.obj#4')

    def test_link_fill_between_sections_is_credited_only_as_int3(self):
        memory = bytearray(0x200)
        memory[0x15:0x18] = b'\xcc' * 3
        rows = self.verdicts({0x10: 12}, [_verdict(0x10, 5, 'a.obj', 3),
                                          _verdict(0x18, 4, 'a.obj', 4)], memory)
        self.assertEqual(rows[0x10]['verdict'], 'exact')
        rows = self.verdicts({0x10: 12}, [_verdict(0x10, 5, 'a.obj', 3),
                                          _verdict(0x18, 4, 'a.obj', 4)])
        self.assertTrue(rows[0x10]['verdict'].startswith('unplaced'))

    def test_uncovered_tail_and_mismatch(self):
        rows = self.verdicts({0x10: 12, 0x40: 4},
                             [_verdict(0x10, 8, 'a.obj', 3),
                              _verdict(0x40, 4, 'b.obj', 1, verdict='mismatch')])
        self.assertTrue(rows[0x10]['verdict'].startswith('unplaced'))
        self.assertTrue(rows[0x40]['verdict'].startswith('mismatch'))

    def test_function_without_contribution(self):
        rows = self.verdicts({0x80: 4}, [_verdict(0x10, 8, 'a.obj', 3)])
        self.assertTrue(rows[0x80]['verdict'].startswith('unplaced'))
        self.assertEqual(rows[0x80]['member'], '-')


class FakeObj:
    """The subset of `homm3.delink.coffx.Obj` the funclet comparison reads."""

    def __init__(self, sections, symbols, relocations):
        self.section_table = sections
        self._symbols = symbols            # [(name, value, section)]
        self._relocations = relocations    # {section: {offset: (target, type)}}
        self.payloads = {}

    def iter_symbols(self):
        for index, (_name, value, section) in enumerate(self._symbols):
            yield index, value, section

    def sym_name(self, index):
        return self._symbols[index][0]

    def section_payload(self, index):
        return self.payloads[index]

    def typed_relocations(self, index):
        return self._relocations.get(index, {})


class FakeImage:
    def __init__(self, sites):
        self.sites = sites

    def relocs_in(self, lo, hi):
        return [s for s in self.sites if lo <= s < hi]


class FuncletGroup(unittest.TestCase):
    """One parent at 0x100 with one cleanup funclet at 0x800 and its stub."""

    def setUp(self):
        # lea ecx,[ebp-0x10]; jmp ??1T@@QAE@XZ | mov eax,$T1; jmp ___CxxFrameHandler
        self.raw = bytes.fromhex('8d4df0e900000000') + bytes.fromhex('b800000000e900000000')
        sections = [dict(index=1, name='.text', comdat=2, assoc=0, characteristics=0x60000020,
                         alignment=16),
                    dict(index=2, name='.text$x', comdat=5, assoc=1, characteristics=0x60000020,
                         alignment=4),
                    dict(index=3, name='.xdata$x', comdat=5, assoc=1, characteristics=0x40000040,
                         alignment=4)]
        self.obj = FakeObj(sections,
                           [('?f@@YAXXZ', 0, 1), ('$T1', 0, 3)],
                           {2: {4: ('??1T@@QAE@XZ', REL32), 9: ('$T1', DIR32),
                                14: ('___CxxFrameHandler', REL32)}})
        self.obj.payloads[2] = self.raw
        self.loaded = (self.obj, {'?f@@YAXXZ': [(0, 1)]}, set(), True)
        self.group = SimpleNamespace(owner_rva=0x100, funclets=(0x800,), stub=0x808,
                                     funcinfo=0x9000)
        self.names = {('', '??1T@@QAE@XZ'): {0x300}, ('', '___CxxFrameHandler'): {0x700}}

    def memory(self, dtor=0x300, handler=0x700, frame=0xf0):
        memory = bytearray(0x1000)
        memory[0x800:0x812] = (bytes([0x8d, 0x4d, frame, 0xe9]) + struct.pack('<i', dtor - 0x808)
                               + b'\xb8' + struct.pack('<I', BASE + 0x9000)
                               + b'\xe9' + struct.pack('<i', handler - 0x812))
        return memory

    def compare(self, memory, sites=(0x809,), names=None, comdats=None):
        return generated_code.funclet_group(
            self.group, '?f@@YAXXZ', 'unit', self.loaded, None, FakePE(memory),
            FakeImage(list(sites)), names or self.names, 0x40, comdats)

    def test_exact_group(self):
        self.assertEqual(self.compare(self.memory())[:2], ('exact', ''))

    def test_wrong_destructor_target(self):
        verdict, reason, _ = self.compare(self.memory(dtor=0x310))
        self.assertEqual(verdict, 'mismatch')
        self.assertIn('??1T@@QAE@XZ', reason)

    def test_frame_offset_difference(self):
        self.assertEqual(self.compare(self.memory(frame=0xe8))[0], 'mismatch')

    def test_absolute_sites_must_agree(self):
        self.assertEqual(self.compare(self.memory(), sites=())[0], 'mismatch')

    def test_unknown_destructor_resolves_only_through_an_identical_comdat(self):
        names = {('', '___CxxFrameHandler'): {0x700}}
        self.assertEqual(self.compare(self.memory(), names=names)[0], 'unresolved')
        seen = []
        verdict = self.compare(self.memory(), names=names,
                               comdats=lambda name, rva: seen.append((name, rva)) or True)[0]
        self.assertEqual(verdict, 'exact')
        self.assertEqual(seen, [('??1T@@QAE@XZ', 0x300)])

    def test_missing_object(self):
        verdict, _reason, start = generated_code.funclet_group(
            self.group, '?f@@YAXXZ', 'unit', None, None, FakePE(self.memory()),
            FakeImage([0x809]), self.names, 0x40)
        self.assertEqual((verdict, start), ('unavailable', None))


class InitializerVerdicts(unittest.TestCase):
    def test_sources_and_thunks(self):
        parts = dict(
            dynamic=dict(matches=[dict(rva=0x10, size=8, owner=dict(source='a.cpp', name='g'))]),
            startup=dict(matches=[], dependencies=[dict(rva=0x20, size=4, unit='b')]),
            shared=dict(matches=[dict(rva=0x30, size=6, unit='c', symbol='$E1')],
                        dependencies=[]),
            cleanups=dict(matches=[], dependencies=[]),
            thunks=dict(matches=[dict(rva=0x50, dll='KERNEL32.dll', imported='Sleep')]))
        census = {0x10: 8, 0x20: 4, 0x30: 7, 0x40: 3, 0x50: 6, 0x60: 6}
        category = {0x10: 'init-thunk', 0x20: 'init-thunk', 0x30: 'init-thunk',
                    0x40: 'init-thunk', 0x50: 'import-thunk', 0x60: 'import-thunk'}
        out = generated_code.initializer_verdicts(parts, census, category)
        self.assertEqual({rva: v[0] for rva, v in out.items()},
                         {0x10: 'exact', 0x20: 'exact', 0x30: 'mismatch',
                          0x40: 'unverified', 0x50: 'exact', 0x60: 'unverified'})
        self.assertEqual(out[0x50][1], 'KERNEL32.dll!Sleep')


if __name__ == '__main__':
    unittest.main()
