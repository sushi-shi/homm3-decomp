import struct
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace

from homm3.build.test_eh_handler_normalization import (
    FixtureSection, _coff, _section_aux, _symbol)
from homm3.delink.coffx import Obj
from homm3.verify import source_padding
from homm3.verify.shared_initializers import proposals
from homm3.verify.startup_bodies import Body

ALIGN16 = 0x00500000
BODY = b'\x33\xc0\xc3'                     # xor eax,eax; ret
STUB = b'\xb8' + bytes(4) + b'\xe9' + bytes(4)
CLEANUP = b'\x8d\x4d\xf0\xe9' + bytes(4)   # lea ecx,[ebp-0x10]; jmp dtor


def objects(sections, symbols):
    """A COFF fixture whose sections are 16-byte aligned code COMDATs."""
    data = bytearray(_coff(sections, symbols))
    for index in range(len(sections)):
        offset = 20 + index * 40 + 36
        struct.pack_into('<I', data, offset,
                         struct.unpack_from('<I', data, offset)[0] | ALIGN16)
    with TemporaryDirectory() as tmp:
        path = Path(tmp, 'unit.obj')
        path.write_bytes(bytes(data))
        return source_padding.inspect(Obj(path))


def retail(payloads, relocations=()):
    image = dict(payloads)

    def read(start, size):
        return b''.join(bytes([image.get(start + i, 0)]) for i in range(size))
    return (SimpleNamespace(read=read),
            lambda lo, hi: [site for site in relocations if lo <= site < hi])


def bytes_at(start, payload):
    return {start + i: b for i, b in enumerate(payload)}


class FunctionPaddingTests(unittest.TestCase):
    def check(self, emitted, retail_gap, *, following=0x110, size=len(BODY)):
        loaded = objects((FixtureSection('.text', emitted, ()),),
                         (_symbol('fn', 0, 1, 0x20, 2),))
        pe, relocs = retail(bytes_at(0x100, BODY + retail_gap))
        entry = source_padding.Entry('unit', 'fn', 0x100, size, 'unit:fn')
        return source_padding.emitted_padding(entry, loaded, pe, relocs, [0x100, following])

    def test_emitted_fill_through_next_boundary_is_exact(self):
        extent, kind, _ = self.check(BODY + b'\x90' * 13, b'\x90' * 13)
        self.assertEqual((extent, kind), ((0x103, 0x110), 'exact'))

    def test_other_content_or_boundary_is_withheld(self):
        self.assertIsNone(self.check(BODY + b'\x90' * 13, b'\xcc' * 13)[0])
        self.assertIsNone(self.check(BODY + b'\x90' * 13, b'\x90' * 13,
                                     following=0x120)[0])

    def test_non_exact_body_uses_its_own_alignment_and_fill(self):
        longer = b'\x90' + BODY + b'\x90' * 12       # emitted body differs
        extent, kind, _ = self.check(longer, b'\x90' * 13)
        self.assertEqual((extent, kind), ((0x103, 0x110), 'aligned'))
        self.assertIsNone(self.check(longer, b'\x00' * 13)[0])

    def test_no_fill_proof_no_credit(self):
        # A section without its own padded tail proves no fill byte.
        body = b'\x90' * 13 + BODY
        self.assertIsNone(self.check(body, b'\x90' * 13)[0])


class LinkerPaddingTests(unittest.TestCase):
    def check(self, gap, *, following=0x220, stub=0x208):
        aux = _section_aux(len(CLEANUP + STUB), 2, parent=1, selection=5)
        loaded = objects((FixtureSection('.text', BODY + b'\x90' * 13, ()),
                          FixtureSection('.text$x', CLEANUP + STUB,
                                         ((4, 3, 20), (9, 3, 6), (14, 3, 20)))),
                         (_symbol('fn', 0, 1, 0x20, 2),
                          _symbol('.text$x', 0, 2, 0, 3, aux),
                          _symbol('handler', 0, 0, 0x20, 2)))
        pe, relocs = retail(bytes_at(0x200, CLEANUP + STUB) | bytes_at(0x212, gap),
                            relocations=(0x209,))
        group = SimpleNamespace(stub=stub, start=0x200)
        return source_padding.eh_padding(group, 'fn', loaded, pe, relocs,
                                         {0x200, following})

    def test_int3_to_next_contribution_is_linker_fill(self):
        extent, reason, content = self.check(b'\xcc' * 14)
        self.assertEqual((extent, reason, content), ((0x212, 0x220), '', 'exact'))

    def test_other_fill_or_unknown_start_is_withheld(self):
        self.assertIsNone(self.check(b'\x90' * 14)[0])
        self.assertIsNone(self.check(b'\xcc' * 14, following=0x230)[0])


class SharedInitializerTests(unittest.TestCase):
    body = Body('init', b'\xa0' + bytes(4) + b'\xc3', ((1, 'guard', 6),), b'', 16)
    image = SimpleNamespace(image_base=0x400000)

    def test_private_common_binding_comes_from_every_relocated_word(self):
        actual = b'\xa0' + struct.pack('<I', 0x6abaa0) + b'\xc3'
        self.assertEqual(proposals(self.body, 0x100, actual, self.image, {'guard': 1}),
                         {'guard': 0x2abaa0})

    def test_unrelocated_difference_rejects_the_copy(self):
        actual = b'\xa1' + struct.pack('<I', 0x6abaa0) + b'\xc3'
        self.assertIsNone(proposals(self.body, 0x100, actual, self.image, {'guard': 1}))


if __name__ == '__main__':
    unittest.main()


class FillBeforeTests(unittest.TestCase):
    def test_int3_gap_from_reviewed_end_to_aligned_start(self):
        pe, _ = retail(bytes_at(0x10c, b'\xcc' * 4))
        self.assertEqual(source_padding.fill_before(pe, 0x110, 16, {0x10c}), (0x10c, 0x110))

    def test_gap_must_start_at_a_reviewed_end_and_be_int3(self):
        pe, _ = retail(bytes_at(0x10c, b'\xcc' * 4))
        self.assertIsNone(source_padding.fill_before(pe, 0x110, 16, {0x108}))
        pe, _ = retail(bytes_at(0x10c, b'\x90' * 4))
        self.assertIsNone(source_padding.fill_before(pe, 0x110, 16, {0x10c}))
        self.assertIsNone(source_padding.fill_before(pe, 0x118, 16, {0x10c}))


class AtexitRegistrationTests(unittest.TestCase):
    def test_guard_store_may_separate_push_and_call(self):
        from homm3.verify.local_cleanups import _pushed_to_atexit
        code = (b'\x68' + struct.pack('<I', 0x404df0)       # push callback
                + b'\x88\x15' + bytes(4)                     # mov [guard], dl
                + b'\xe8' + bytes(4))                        # call _atexit
        self.assertEqual(_pushed_to_atexit(code, lambda address: address == 11),
                         [(1, 0x404df0)])

    def test_stack_adjustment_breaks_the_pairing(self):
        from homm3.verify.local_cleanups import _pushed_to_atexit
        code = (b'\x68' + struct.pack('<I', 0x404df0) + b'\x83\xc4\x04'
                + b'\xe8' + bytes(4))
        self.assertEqual(_pushed_to_atexit(code, lambda address: address == 8), [])


class ImportThunkTests(unittest.TestCase):
    def archive(self, member):
        header = b'VERSION.dll/    ' + bytes(32).replace(b'\0', b' ')
        header += str(len(member)).ljust(10).encode() + b'`\n'
        return b'!<arch>\n' + header + member + (b'\n' if len(member) & 1 else b'')

    def short_import(self, symbol, dll, value, name_type):
        names = symbol.encode() + b'\0' + dll.encode() + b'\0'
        return (struct.pack('<HHHHIIHH', 0, 0xffff, 0, 0x14c, 0, len(names),
                            value, name_type << 2) + names)

    def test_short_import_names_and_ordinals(self):
        from homm3.verify.import_thunks import library_imports
        with TemporaryDirectory() as tmp:
            Path(tmp, 'VERSION.LIB').write_bytes(self.archive(
                self.short_import('_VerQueryValueA@16', 'VERSION.dll', 0, 3)))
            self.assertEqual(library_imports(Path(tmp), 'VERSION.dll'),
                             ({'VerQueryValueA'}, True))
            self.assertEqual(library_imports(Path(tmp), 'IFC20.dll'), (set(), False))
        with TemporaryDirectory() as tmp:
            Path(tmp, 'WSOCK32.LIB').write_bytes(self.archive(
                self.short_import('_send@16', 'WSOCK32.dll', 19, 0)))
            self.assertEqual(library_imports(Path(tmp), 'WSOCK32.dll'), ({19}, True))
