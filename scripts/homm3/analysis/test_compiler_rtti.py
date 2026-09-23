"""Compiler structures require bounded fields and independent roots."""
import struct
import unittest

from homm3.analysis.compiler_rtti import Records, throw_roots
from homm3.sema.retail_layout import Layout
from homm3.analysis.test_data_initialization import crt_fixture


def records_fixture():
    image, _, _, _ = crt_fixture()
    layout = Layout(image)
    def put(rva, data):
        offset = layout.raw(rva, len(data))
        image[offset:offset+len(data)] = data
    def words(rva, *values):
        put(rva, struct.pack('<'+'I'*len(values), *values))
    put(0x2000, struct.pack('<II', 0x4021e0, 0)+b'.?AVGeneric@@\0')
    words(0x2020, 0, 0x402000, 0, 0xffffffff, 0, 16, 0x401100)
    words(0x2040, 1, 0x402020)
    words(0x2050, 0, 0x401100, 0, 0x402040)
    words(0x2080, 0, 0, 0, 0x402000, 0x4020a0)
    words(0x20a0, 0, 0, 1, 0x4020b0)
    words(0x20b0, 0x4020c0)
    words(0x20c0, 0x402000, 0, 0, 0xffffffff, 0, 0)
    words(0x21bc, 0x402080, 0x401100)
    words(0x21e0, 0x401100)
    return image


class CompilerRttiTest(unittest.TestCase):
    def test_typed_exception_graph_has_exact_bounded_record_extents(self):
        parser = Records(Layout(records_fixture()), 0x21e0)
        parser.throw_info(0x2050)
        self.assertEqual({k: r['size'] for (k, _), r in parser.rows.items()}, {
            'rtti-type-descriptor': 22, 'eh-catchable-type': 28,
            'eh-catchable-type-array': 8, 'eh-throw-info': 16})

    def test_type_descriptor_needs_proved_vptr_zero_cache_and_terminated_name(self):
        for rva, value in [(0x2000, 0x4021c0), (0x2004, 1), (0x2008, 0x42424242)]:
            image = records_fixture()
            struct.pack_into('<I', image, Layout(image).raw(rva, 4), value)
            with self.assertRaises(ValueError):
                Records(Layout(image), 0x21e0).descriptor(0x2000)
        image = records_fixture()
        image[Layout(image).raw(0x2008, 1):0x800] = b'A'*(0x800-Layout(image).raw(0x2008, 1))
        with self.assertRaises(ValueError):
            Records(Layout(image), 0x21e0).descriptor(0x2000)

    def test_oversized_array_bad_code_pointer_and_newer_flags_are_rejected(self):
        for rva, value in [(0x2040, 1025), (0x2040, 100), (0x2038, 0x402100),
                           (0x2020, 8), (0x205c, 0x403000)]:
            image = records_fixture()
            struct.pack_into('<I', image, Layout(image).raw(rva, 4), value)
            with self.subTest(rva=hex(rva)), self.assertRaises(ValueError):
                Records(Layout(image), 0x21e0).throw_info(0x2050)

    def test_class_graph_validates_count_descriptor_size_and_pointers(self):
        parser = Records(Layout(records_fixture()), 0x21e0)
        parser.locator(0x2080)
        self.assertEqual(len(parser.rows), 5)
        for rva, value in [(0x2080, 1), (0x20a8, 0), (0x20c4, 1), (0x20d4, 0x40)]:
            image = records_fixture()
            struct.pack_into('<I', image, Layout(image).raw(rva, 4), value)
            with self.assertRaises(ValueError):
                Records(Layout(image), 0x21e0).locator(0x2080)

    def test_throw_argument_uses_reachable_stack_dataflow(self):
        image = records_fixture()
        # Unknown earlier call, then push type info, push object, call throw.
        code = bytearray(bytes.fromhex('e800000000685020400050e800000000c3'))
        struct.pack_into('<i', code, 12, 0x1100-(0x1000+12+4))
        offset = Layout(image).raw(0x1000, len(code))
        image[offset:offset+len(code)] = code
        roots, issues = throw_roots(Layout(image), {0x1000: len(code)}, {0x1100})
        self.assertFalse(issues)
        self.assertEqual(roots[0]['rva'], 0x2050)
        # Jump over both argument pushes; their mere presence cannot supply a root.
        code[:5] = bytes.fromhex('eb09909090')
        image[offset:offset+len(code)] = code
        roots, issues = throw_roots(Layout(image), {0x1000: len(code)}, {0x1100})
        self.assertFalse(roots)
        self.assertEqual(len(issues), 1)


if __name__ == '__main__':
    unittest.main()
