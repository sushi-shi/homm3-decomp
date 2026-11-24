"""`_initterm` calls every `.CRT$XCU` slot, so each slot starts a function
even where the function before it ends in a call that never returns and its
descent falls through into the initializer (h3maped's `bitset<8>::_Xran` at
0x416bac throws, then `pop esi`; the initializer starts at 0x416c06)."""
import struct
import unittest
from types import SimpleNamespace

from homm3.census import eh, functions

BASE = 0x400000


def call(at, target):
    return b'\xe8' + struct.pack('<i', target - (at + 5))


def pad(code, to):
    return code + b'\xcc' * (to - 0x1000 - len(code))


def text():
    # 1000 push end; push begin; call _initterm; call thrower; ret
    code = (b'\x68' + struct.pack('<I', BASE + 0x200c) +
            b'\x68' + struct.pack('<I', BASE + 0x2000) +
            call(0x100a, 0x1020) + call(0x100f, 0x1030) + b'\xc3')
    code = pad(code, 0x1020) + b'\xc3'                      # 1020 _initterm
    # 1030 push esi; call the throw; pop esi (never reached)
    code = pad(code, 0x1030) + b'\x56' + call(0x1031, 0x1050) + b'\x5e'
    code += b'\xc3'                                         # 1037 slot 2
    code = pad(code, 0x1040) + b'\xc3'                      # 1040 slot 1
    code = pad(code, 0x1050) + b'\xc3'                      # 1050 the throw
    return code


class Image:
    image_base = BASE

    def __init__(self, code, table):
        header = bytearray(0x100)
        struct.pack_into('<I', header, 0x3c, 0x40)
        struct.pack_into('<I', header, 0x40 + 24 + 16, 0x1000)
        self.data = bytes(header) + code + table
        self.sections = [
            SimpleNamespace(name='.text', rva=0x1000, size=len(code), raw_offset=0x100,
                            executable=True),
            SimpleNamespace(name='.data', rva=0x2000, size=len(table),
                            raw_offset=0x100 + len(code), executable=False),
        ]

    def blob(self, section):
        return self.data[section.raw_offset:section.raw_offset + section.size]

    def section_of(self, rva):
        return next((s for s in self.sections if s.rva <= rva < s.rva + s.size), None)

    def in_image(self, va):
        return self.section_of(va - self.image_base) is not None


def census():
    # __xc_a, slot 1, slot 2, __xc_z
    table = struct.pack('<IIII', 0, BASE + 0x1040, BASE + 0x1037, 0)
    return functions.run(Image(text(), table), log=lambda _: None)


class InitializerSlotTest(unittest.TestCase):
    def test_slots_are_read_from_the_initterm_table(self):
        self.assertEqual(eh.initializer_slots(census()), [(0x1040, 1), (0x1037, 2)])

    def test_slot_after_a_throw_starts_a_function(self):
        c = census()
        self.assertEqual(c.starts.get(0x1037), 'initializer')
        spans = {r: n for r, n, _ in functions.partition(c)}
        self.assertEqual(spans[0x1030], 7)
        self.assertEqual(spans[0x1037], 1)

    def test_an_image_without_data_has_no_slots(self):
        c = functions.Census(Image(text(), b''))
        c.image.sections.pop()
        self.assertEqual(eh.initializer_slots(c), [])


if __name__ == '__main__':
    unittest.main()
