"""Negative control for the unclaimed-`__h3cg$` gate in the canonicalizer.

The gate exists to catch a compiler-function symbol that no VA_COMPGEN owns.
It compared the object's `__h3cg$` function symbols against the rename set
only - the ANONYMOUS kinds (STATIC_DTOR and friends), which are the ones this
module renames - and a unit's DIRECT-symbol claims were invisible to it.

That is fine while a unit has no anonymous claim at all, because the whole
check is skipped then. The moment one lands, every DIRECT claim whose base
symbol the compile does not emit - those keep the `__h3cg$` spelling in the
delinked object rather than the mangled one - is reported as unclaimed.
Measured 2026-09-06 on game.obj, where adding one STATIC_DTOR claim made the
build refuse over SCampaign's implicit copy assign and the two
type_map_hero_identity rows, all three of them claimed in game.cpp.

Both arms below must hold: the gate still fires on a genuinely unowned name,
and it no longer fires on a name some other claim accounts for.
"""

import unittest

from homm3.build.canonicalize_data_symbols import (
    COMPGEN_PREFIX, unexpected_compgen_names)


UNIT = "game"


def _name(kind: str, owner: str) -> str:
    return f"{COMPGEN_PREFIX}{UNIT}${kind}${owner}"


class CompgenClaimAccountingTest(unittest.TestCase):
    def test_the_gate_still_fires_on_a_genuinely_unowned_name(self):
        # THE defect the gate is for: a compiler-function symbol reached the
        # object with no claim of any kind behind it.
        orphan = _name("static_dtor", "someUnclaimedStatic")
        self.assertEqual(
            unexpected_compgen_names([orphan], frozenset()),
            [orphan])

    def test_a_direct_kind_claim_accounts_for_its_own_name(self):
        # THE regression: an unjoined DIRECT claim keeps the `__h3cg$`
        # spelling and must not be reported just because it is not in the
        # rename set.
        direct = _name("implicit_copy_assign", "SCampaign")
        anonymous = _name("static_dtor", "immMouse")
        self.assertEqual(
            unexpected_compgen_names([direct, anonymous],
                                     frozenset({direct, anonymous})),
            [])
        # ...and dropping the direct claim from the accounting brings the
        # report straight back, which is what makes this a control.
        self.assertEqual(
            unexpected_compgen_names([direct, anonymous],
                                     frozenset({anonymous})),
            [direct])

    def test_names_without_the_prefix_are_never_reported(self):
        self.assertEqual(
            unexpected_compgen_names(
                ["??4SCampaign@@QAEAAV0@ABV0@@Z", "?Save@game@@QAEHPAX@Z"],
                frozenset()),
            [])

    def test_the_report_is_sorted_and_complete(self):
        names = [_name("std_copy", "type_map_hero_identity"),
                 _name("implicit_copy_assign", "SCampaign"),
                 _name("vector_destroy", "type_map_hero_identity")]
        self.assertEqual(unexpected_compgen_names(names, frozenset()),
                         sorted(names))


if __name__ == "__main__":
    unittest.main()
