import struct
import unittest
from homm3.census.reference_initializers import reference_initializer, startup_bodies

BASE = 0x400000
BODY = b'\xc7\x05' + bytes(8) + b'\xc3'
NAME = '__h3cg$context$static_ctor$reference'


class ReferenceInitializerTests(unittest.TestCase):
    def inputs(self):
        native = b'\xc7\x05' + struct.pack('<II', BASE + 0x3000, BASE + 0x3010) + b'\xc3'
        memory = {0x1000: b'\xe9' + struct.pack('<i', 0x10 - 5),
                  0x1010: native, 0x3010: struct.pack('<I', 3)}
        read = lambda rva, size: memory.get(rva, b'')[:size]
        starts = startup_bodies({0x1000: 5, 0x1010: 11}, [(0x1000, '7')], read)
        kwargs = dict(starts=starts, read=read, image_base=BASE,
                      data_names={'reference': {0x3000}},
                      data_definers={'reference': ('context', 4, bytes(4)),
                                     'backing': ('context', 4, struct.pack('<I', 3))},
                      data_relocs={}, absolute_sites={0x1012, 0x1016},
                      is_data=lambda rva, size: 0x3000 <= rva and rva + size <= 0x4000)
        return memory, kwargs

    def run_case(self, **changes):
        _memory, kwargs = self.inputs()
        kwargs.update(changes)
        return reference_initializer(NAME, BODY, {2: ('reference', 6), 6: ('backing', 6)}, **kwargs)

    def test_named_destination_backing_and_crt_path_prove_one_body(self):
        self.assertEqual(self.run_case(), (0x1010, 'backing', 0x3010))

    def test_direct_crt_body_is_supported(self):
        self.assertEqual(self.run_case(starts={0x1010: 11}), (0x1010, 'backing', 0x3010))

    def test_unknown_or_ambiguous_destination_is_rejected(self):
        for names in ({}, {'reference': {0x3000, 0x3004}}, {'reference': {0x3004}}):
            with self.subTest(names=names):
                self.assertIsNone(self.run_case(data_names=names))

    def test_missing_crt_path_or_wrong_extent_is_rejected(self):
        for starts in ({}, {0x1010: 10}, {0x1010: 12}):
            with self.subTest(starts=starts):
                self.assertIsNone(self.run_case(starts=starts))

    def test_all_native_relocation_sites_must_agree(self):
        for sites in ({0x1012}, {0x1016}, {0x1012, 0x1016, 0x101a}):
            with self.subTest(sites=sites):
                self.assertIsNone(self.run_case(absolute_sites=sites))

    def test_wrong_or_relocated_backing_is_rejected(self):
        memory, kwargs = self.inputs()
        memory[0x3010] = struct.pack('<I', 4)
        self.assertIsNone(reference_initializer(NAME, BODY,
            {2: ('reference', 6), 6: ('backing', 6)}, **kwargs))
        self.assertIsNone(self.run_case(data_relocs={'backing': {0: ('pointer', 6)}}))

    def test_backing_must_have_a_complete_initialized_word(self):
        for definition in (None, ('context', 4, None), ('context', 8, bytes(8)),
                           ('context', 4, bytes(3))):
            _memory, kwargs = self.inputs()
            if definition is None:
                del kwargs['data_definers']['backing']
            else:
                kwargs['data_definers']['backing'] = definition
            with self.subTest(definition=definition):
                self.assertIsNone(reference_initializer(NAME, BODY,
                    {2: ('reference', 6), 6: ('backing', 6)}, **kwargs))

    def test_known_conflicting_backing_identity_is_rejected(self):
        for names in ({'reference': {0x3000}, 'backing': {0x3020}},
                      {'reference': {0x3000}, 'other': {0x3010}}):
            with self.subTest(names=names):
                self.assertIsNone(self.run_case(data_names=names))

    def test_code_backing_is_rejected(self):
        self.assertIsNone(self.run_case(is_data=lambda rva, size: rva == 0x3000))

    def test_two_valid_crt_bodies_are_ambiguous(self):
        memory, kwargs = self.inputs()
        memory[0x1020] = memory[0x1010]
        kwargs['starts'][0x1020] = 11
        kwargs['absolute_sites'] |= {0x1022, 0x1026}
        self.assertIsNone(reference_initializer(NAME, BODY,
            {2: ('reference', 6), 6: ('backing', 6)}, **kwargs))

    def test_ordinary_function_or_different_store_shape_is_rejected(self):
        _memory, kwargs = self.inputs()
        for name, body, relocs in (
                ('_function', BODY, {2: ('reference', 6), 6: ('backing', 6)}),
                (NAME, BODY[:-1] + b'\x90', {2: ('reference', 6), 6: ('backing', 6)}),
                (NAME, BODY, {2: ('reference', 6), 6: ('backing', 20)}),
                (NAME, BODY, {2: ('reference', 6)}),
                (NAME, BODY[:6] + struct.pack('<I', 4) + BODY[10:],
                 {2: ('reference', 6), 6: ('backing', 6)})):
            with self.subTest(name=name, body=body, relocs=relocs):
                self.assertIsNone(reference_initializer(name, body, relocs, **kwargs))

    def test_cleanup_slots_and_non_jump_wrappers_do_not_prove_target(self):
        memory, kwargs = self.inputs()
        functions = {0x1000: 5, 0x1010: 11}
        self.assertEqual(startup_bodies(functions, [(0x1000, '-')], kwargs['read']), {})
        memory[0x1000] = b'\xe8' + struct.pack('<i', 11)
        self.assertNotIn(0x1010, startup_bodies(functions, [(0x1000, '7')], kwargs['read']))
        self.assertNotIn(0x1020, startup_bodies(functions, [(0x1020, '7')], kwargs['read']))


if __name__ == '__main__':
    unittest.main()
