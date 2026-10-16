"""Late-discovered initializer targets must not enlarge an intervening ctor."""
import struct
import unittest
from types import SimpleNamespace

from homm3.census import eh, functions


class Image:
    image_base = 0x400000

    def __init__(self, code):
        header = bytearray(0x100)
        struct.pack_into('<I', header, 0x3c, 0x40)
        struct.pack_into('<I', header, 0x40 + 24 + 16, 0x1000)
        self.data = bytes(header) + code
        self.sections = [SimpleNamespace(name='.text', rva=0x1000, size=len(code),
                                         raw_offset=0x100, executable=True)]

    def blob(self, section):
        return self.data[section.raw_offset:section.raw_offset + section.size]

    def section_of(self, rva):
        return next((s for s in self.sections if s.rva <= rva < s.rva + s.size), None)

    def in_image(self, va):
        return self.section_of(va - self.image_base) is not None


class InitializerTailTest(unittest.TestCase):
    def thunk(self, code):
        c = functions.Census(Image(code))
        seen = set()
        at = 0x1000
        while at < c.text_hi:
            seen.add(at)
            at += c.insn(at).size
        return functions.call_then_tail_thunk(c, 0x1000, seen)

    def test_direct_call_and_jump_are_a_forwarding_thunk(self):
        self.assertTrue(self.thunk(bytes.fromhex('e80b000000e926000000')))

    def test_call_and_return_are_not_a_forwarding_thunk(self):
        self.assertFalse(self.thunk(bytes.fromhex('e80b000000c3')))

    def test_indirect_call_is_not_admitted(self):
        self.assertFalse(self.thunk(bytes.fromhex('ffd0e926000000')))

    def test_indirect_jump_is_not_admitted(self):
        self.assertFalse(self.thunk(bytes.fromhex('e80b000000ffe0')))

    def test_a_longer_body_is_not_admitted(self):
        self.assertFalse(self.thunk(bytes.fromhex('e80b00000090e926000000')))

    def test_jump_within_the_known_body_is_not_a_new_function(self):
        # The first call names 1020, so the jump to 1010 stays within the
        # wrapper's own range and reaches its return. It is not a tail call.
        code = (bytes.fromhex('e81b000000e906000000') + b'\x90' * 6 +
                b'\xc3' + b'\x90' * 15 + b'\xc3')
        c = functions.run(Image(code), log=lambda _: None)
        self.assertNotIn(0x1010, c.starts)
        self.assertEqual(set(c.starts), {0x1000, 0x1020})

    def test_late_call_then_tail_target_has_its_own_extent(self):
        # 1000: call 1010; jmp 1030 (the CRT initializer wrapper)
        # 1010: call 1020; ret       (initializes a global)
        # 1020: ret                  (the intervening constructor)
        # 1030: ret                  (the registration tail)
        code = (bytes.fromhex('e80b000000e926000000') + b'\x90' * 6 +
                bytes.fromhex('e80b000000c3') + b'\x90' * 10 +
                b'\xc3' + b'\x90' * 15 + b'\xc3')
        c = functions.run(Image(code), log=lambda _: None)
        self.assertIn(0x1030, c.starts)
        self.assertEqual(c.starts[0x1030], 'tail')
        spans = {r: n for r, n, _ in functions.partition(c)}
        self.assertEqual(spans, {0x1000: 10, 0x1010: 6, 0x1020: 1, 0x1030: 1})


class RegistrationRowsTest(unittest.TestCase):
    def census(self, pop=b'\x59'):
        wrapper = bytes.fromhex('e80b000000e926000000') + b'\x90' * 6
        init = bytes.fromhex('e80b000000c3') + b'\x90' * 10
        ctor = b'\xc3' + b'\x90' * 15
        registration = (b'\x68' + struct.pack('<I', Image.image_base + 0x1050) +
                        bytes.fromhex('e826000000') + pop + b'\xc3')
        code = wrapper + init + ctor + registration + b'\x90' * 20
        code += b'\xc3' + b'\x90' * 15 + b'\xc3'
        return functions.run(Image(code), log=lambda _: None)

    def test_only_registration_body_inherits_crt_slot(self):
        c = self.census()
        self.assertEqual(eh.registration_rows(c, [(0x1000, 7)], 0x1060),
                         [(0x1030, 7)])
        self.assertEqual(eh.cleanup_rows(c, {0x1000, 0x1030}, 0x1060),
                         [(0x1050, '-')])

    def test_a_different_callee_is_not_atexit_registration(self):
        self.assertEqual(eh.registration_rows(self.census(), [(0x1000, 7)], 0x1020), [])

    def test_an_ordinary_caller_does_not_classify_its_tail(self):
        self.assertEqual(eh.registration_rows(self.census(), [], 0x1060), [])

    def test_unknown_cleanup_address_is_not_classified(self):
        c = self.census()
        del c.starts[0x1050]
        self.assertEqual(eh.registration_rows(c, [(0x1000, 7)], 0x1060), [])

    def test_an_existing_initializer_is_not_duplicated(self):
        rows = [(0x1000, 7), (0x1030, 8)]
        self.assertEqual(eh.registration_rows(self.census(), rows, 0x1060), [])

    def test_different_stack_cleanup_is_not_the_proven_body(self):
        self.assertEqual(eh.registration_rows(self.census(b'\x5a'), [(0x1000, 7)], 0x1060), [])


if __name__ == '__main__':
    unittest.main()
