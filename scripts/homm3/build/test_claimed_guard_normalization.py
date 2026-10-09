"""A candidate's local-static guard takes the target's DATA_COMPGEN_GUARD name,
or the masked `$S` spelling another image's placements give it."""
import unittest

from homm3.build import canonicalize_data_symbols as canon
from homm3.build.normalize_objs import _canonicalize_claimed_guards, _canonicalize_masked_guards
from homm3.build.test_icf_alias_normalization import CALL, coff

SCOPE = "@?1??load@@YI_NXZ"
GUARD = f"_?$S28{SCOPE}@4EA"
OWNER = f"_?strings{SCOPE}@4V?$TAutoArrayPtr@D@@A"
OTHER = f"_?names{SCOPE}@4V?$TAutoArrayPtr@D@@A"
CLAIM = "__h3cg$unit$static_init_guard$stringsGuard"
OWNERS = {CLAIM: ("strings", 0x2938d4)}


def names(payload: bytes) -> set[str]:
    return {symbol.name for symbol in canon.CoffObject(payload).symbols.values()}


class ClaimedGuardTest(unittest.TestCase):
    def test_guard_in_the_owner_scope_takes_the_claim_name(self):
        base = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)], [GUARD, OWNER])
        target = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, CLAIM)], [CLAIM])
        renamed, count = _canonicalize_claimed_guards(base, target, "unit", OWNERS)
        self.assertEqual(count, 1)
        self.assertIn(CLAIM, names(renamed))
        self.assertNotIn(GUARD, names(renamed))

    def test_a_target_spelling_the_guard_stays_unchanged(self):
        base = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)], [GUARD, OWNER])
        target = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)], [GUARD])
        self.assertEqual(_canonicalize_claimed_guards(base, target, "unit", OWNERS),
                         (base, 0))

    def test_another_units_claim_or_an_ambiguous_scope_stays_visible(self):
        base = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)], [GUARD, OWNER])
        target = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, CLAIM)], [CLAIM])
        self.assertEqual(_canonicalize_claimed_guards(base, target, "other", OWNERS),
                         (base, 0))
        second = f"_?$S29{SCOPE}@4EA"
        base = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)],
                    [GUARD, second, OWNER, OTHER])
        self.assertEqual(_canonicalize_claimed_guards(base, target, "unit", OWNERS),
                         (base, 0))


MASKED = f"_?$S{SCOPE}@4EA"


class MaskedGuardTest(unittest.TestCase):
    # the target defines the guard it names (the helper puts definitions in .text)
    def test_the_counter_spelling_takes_the_targets_masked_name(self):
        base = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)], [GUARD, OWNER])
        target = coff(CALL, [("?load@@YI_NXZ", 0), (MASKED, 4)], [(1, MASKED)])
        renamed, count = _canonicalize_masked_guards(base, target)
        self.assertEqual(count, 1)
        self.assertIn(MASKED, names(renamed))
        self.assertNotIn(GUARD, names(renamed))

    def test_two_guards_of_one_spelling_stay_visible(self):
        second = f"_?$S29@?2??load@@YI_NXZ@4EA"
        base = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)], [GUARD, second, OWNER])
        target = coff(CALL, [("?load@@YI_NXZ", 0), (MASKED, 4)], [(1, MASKED)])
        self.assertEqual(_canonicalize_masked_guards(base, target), (base, 0))

    def test_a_target_without_masked_guards_leaves_the_candidate(self):
        base = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)], [GUARD, OWNER])
        target = coff(CALL, [("?load@@YI_NXZ", 0)], [(1, GUARD)], [GUARD])
        self.assertEqual(_canonicalize_masked_guards(base, target), (base, 0))


if __name__ == "__main__":
    unittest.main()
