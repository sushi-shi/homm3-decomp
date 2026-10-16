"""A shared header emitter must not duplicate an image's source claim."""

import unittest

from homm3.census.placements import remove_redundant_function_placements
from homm3.retail_labels import Claim


class SourcePlacementOwnershipTest(unittest.TestCase):
    name = "??0TError@?$TGDIObjectSelector@VCBitmap@@@@QAE@XZ"

    def claim(self, **changes):
        values = dict(rva=0x29420, name=self.name, kind="func",
                      channel="src-VA+base", size=18,
                      unit="ScenarioPropsProloguePage", meta={})
        values.update(changes)
        return Claim(**values)

    def row(self, **changes):
        values = dict(rva=0x29420, size=18, kind="func", name=self.name,
                      unit="MemoryDC", linkage="external", evidence="own VA claim")
        values.update(changes)
        return tuple(values[key] for key in
                     ("rva", "size", "kind", "name", "unit", "linkage", "evidence"))

    def test_shared_emitter_does_not_duplicate_the_source_owner(self):
        row = self.row()
        self.assertEqual(remove_redundant_function_placements([row], [self.claim()]),
                         ([], [row]))

    def test_real_identity_and_extent_conflicts_remain(self):
        for change in ({"rva": 0x29440}, {"size": 19}, {"name": "other"},
                       {"kind": "data"}, {"linkage": "internal"}):
            with self.subTest(change=change):
                row = self.row(**change)
                self.assertEqual(
                    remove_redundant_function_placements([row], [self.claim()]),
                    ([row], []))

    def test_same_unit_internal_definition_is_redundant(self):
        row = self.row(unit="ScenarioPropsProloguePage", linkage="internal")
        self.assertEqual(remove_redundant_function_placements([row], [self.claim()]),
                         ([], [row]))

    def test_unclaimed_body_and_data_claim_do_not_suppress_a_function(self):
        row = self.row()
        for claims in ([], [self.claim(kind="data")]):
            with self.subTest(claims=claims):
                self.assertEqual(remove_redundant_function_placements([row], claims),
                                 ([row], []))


if __name__ == "__main__":
    unittest.main()
