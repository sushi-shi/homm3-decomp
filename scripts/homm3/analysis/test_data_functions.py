"""Code dataflow must consume raw relocation identity, not its placeholder word."""
from collections import defaultdict
import struct
import unittest

from homm3.analysis.candidate_data import inventory
from homm3.analysis.data_functions import Functions
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
from homm3.sema.data_match import Identities
from homm3.sema.test_data_match import binding
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


class DataFunctionsTest(unittest.TestCase):
    def provider(self, target=1, bindings=True):
        raw = bytearray(_coff((FixtureSection('.text', bytes.fromhex('c7050000000007000000c3'), ((2, target, 6),)),
                              FixtureSection('.data', bytes(8), ())),
                             (_symbol('init', 0, 1, 0x20, 2), _symbol('one', 0, 2, 0, 2),
                              _symbol('two', 4, 2, 0, 2))))
        struct.pack_into('<I', raw, 96, 0xc0300040)
        obj = CoffObject(bytes(raw))
        rows = inventory('a', obj, 'raw')
        enrolled = [binding(r, address, 4) for r, address in zip(rows, (0x2000, 0x2010))] if bindings else []
        identities = Identities(rows, enrolled, {'a': obj})
        return Functions(Layout(fixture()), {}, defaultdict(set), {'a': obj}, identities)

    def test_changed_relocation_changes_actual_written_owner(self):
        for target, rva in [(1, 0x2000), (2, 0x2010)]:
            profile = self.provider(target).closure(('a', 0))
            self.assertTrue(profile['complete'])
            self.assertEqual(profile['writes'][0]['rva'], rva)
            self.assertEqual(profile['writes'][0]['value'], ('integer', 7))

    def test_missing_binding_cannot_turn_address_placeholder_into_proof(self):
        profile = self.provider(bindings=False).closure(('a', 0))
        self.assertFalse(profile['complete'])
        self.assertFalse(profile['writes'])
        self.assertEqual(profile['issues'][0]['kind'], 'unresolved-write')

    def test_same_function_spelling_in_another_unit_does_not_supply_identity(self):
        obj = CoffObject(_coff((FixtureSection('.text', b'\xc3', ()),),
                              (_symbol('helper', 0, 1, 0x20, 3),)))
        provider = Functions(Layout(fixture()), {}, defaultdict(set), {'a': obj, 'b': obj}, None,
                             [dict(unit='a', symbol='helper', rva=0x1000)])
        self.assertEqual(provider.anchors['a', 0], {0x1000})
        self.assertFalse(provider.anchors['b', 0])

    def test_scope_free_runtime_anchor_requires_external_linkage(self):
        def obj(storage):
            return CoffObject(_coff((FixtureSection('.text', b'\xc3', ()),),
                                   (_symbol('helper', 0, 1, 0x20, storage),)))
        provider = Functions(Layout(fixture()), {}, defaultdict(set), {'a': obj(2), 'b': obj(3)}, None,
                             [dict(symbol='helper', rva=0x1000)])
        self.assertEqual(provider.anchors['a', 0], {0x1000})
        self.assertFalse(provider.anchors['b', 0])


if __name__ == '__main__':
    unittest.main()
