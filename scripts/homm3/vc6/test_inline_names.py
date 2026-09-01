#!/usr/bin/env python3
"""Hermetic tests for the inline-divergence name pairing (homm3.vc6.inline_model).

Run: `python3 -m homm3.vc6.test_inline_names` (rc != 0 on any failure); also
wired as part of the `locator` gate of `homm3 vc6 check`.

THE DEFECT THESE EXIST FOR (2026-08-19). `predict-inline` compares the two
sides' out-of-line CALL multisets BY SYMBOL NAME, and the two sides do not
name the same function the same way: our base obj emits the real mangled
symbol, while the delinked target carries whatever the synth PDB called the
callee - a working label (`sub_f6570`), a runtime-map label (`exe_new`) or a
flat carcass name (`hero_load`). One call then booked TWICE, as an
under-inline on our side and an over-inline on retail's, and since `_route`
puts the inliner upstream of registers and blocks, the phantom buried the
function's true diagnosis. Measured on the tree that day: 77 of the 135
plateaus reporting inline divergence were this and nothing else.

The NEGATIVE controls are the point. Pairing that swallows a real surplus
would be worse than the original defect - it would report agreement where a
callee really is expanded on one side only - so the cases below pin both
directions: a genuine surplus must still report, and a name BOTH sides emit
must never be paired away.
"""
from __future__ import annotations

import unittest
from collections import Counter

from homm3.vc6 import inline_model as im


class UnresolvableName(unittest.TestCase):
    """Only a name MSVC cannot emit counts as unresolvable."""

    def test_working_label_is_unresolvable(self):
        self.assertTrue(im._unresolvable("sub_f6570"))
        self.assertTrue(im._unresolvable("game_a7250_sub00_127f60"))

    def test_runtime_and_carcass_labels_are_unresolvable(self):
        self.assertTrue(im._unresolvable("exe_new"))
        self.assertTrue(im._unresolvable("hero_load"))
        self.assertTrue(im._unresolvable("NewfullMap_Load"))

    def test_emittable_symbols_are_resolvable(self):
        # NEGATIVE CONTROL: these are names our compiled side really emits,
        # so a count difference on them is a REAL divergence and must never
        # be paired away.
        self.assertFalse(im._unresolvable("?_Tidy@?$basic_string@DU@std@@AAEX_N@Z"))
        self.assertFalse(im._unresolvable("_sprintf"))
        self.assertFalse(im._unresolvable("@deflateReset@4"))
        self.assertFalse(im._unresolvable("__h3cg$artifact$static_dtor$x"))


class Divergence(unittest.TestCase):

    def test_pure_name_artifact_reports_agreement(self):
        # monsters_sell_out's shape: same call, two names.
        base = Counter({"?AI_bribe_monsters@@YIEPBVhero@@@Z": 1, "_sprintf": 2})
        ref = Counter({"game_a7250_sub00_127f60": 1, "_sprintf": 2})
        under, over, paired = im.divergence(base, ref)
        self.assertEqual((under, over, paired), (0, 0, 1))
        self.assertIsNone(im.divergence_note(base, ref))

    def test_real_surplus_survives_pairing(self):
        # NEGATIVE CONTROL: retail calls a label 3x, we emit only 1 unmatched
        # call - the 2 extra retail calls are a real over-inline and must
        # still report.
        base = Counter({"?f@@YAXXZ": 1})
        ref = Counter({"sub_1000": 3})
        under, over, paired = im.divergence(base, ref)
        self.assertEqual((under, over, paired), (0, 2, 1))
        self.assertIn("2 over-inline", im.divergence_note(base, ref))

    def test_shared_name_count_difference_is_never_paired(self):
        # NEGATIVE CONTROL: `_Tidy` is spelled the same on both sides, so a
        # 38-vs-39 difference is real signal, not a naming artifact.
        tidy = "?_Tidy@?$basic_string@DU@std@@AAEX_N@Z"
        base, ref = Counter({tidy: 38}), Counter({tidy: 39})
        under, over, paired = im.divergence(base, ref)
        self.assertEqual((under, over, paired), (0, 1, 0))

    def test_unmatched_base_call_still_reports_under(self):
        # NEGATIVE CONTROL: nothing on retail's side to pair with, so our
        # extra out-of-line call is a real under-inline.
        base = Counter({"?g@@YAXXZ": 2})
        ref = Counter()
        under, over, paired = im.divergence(base, ref)
        self.assertEqual((under, over, paired), (2, 0, 0))

    def test_retail_only_real_name_is_over_inline(self):
        # A retail-only callee under a MANGLED name cannot be a naming
        # artifact - our side would have emitted that same name.
        base = Counter()
        ref = Counter({"?IsLocalHuman@game@@QBE_NH@Z": 1})
        under, over, paired = im.divergence(base, ref)
        self.assertEqual((under, over, paired), (0, 1, 0))

    def test_two_mangled_singletons_never_pair(self):
        # THE DISCRIMINATING NEGATIVE CONTROL. Both sides have exactly one
        # unmatched call and both spell it with a real mangled name, so this
        # is a genuine 1-under / 1-over: our CL calls `?a`, retail calls `?b`.
        # An over-broad `_unresolvable` (one that treated any unmatched retail
        # callee as a label) would pair them off and report agreement - which
        # is the failure mode this whole change must not introduce. It is the
        # only case here whose verdict actually moves with the predicate.
        base, ref = Counter({"?a@@YAXXZ": 1}), Counter({"?b@@YAXXZ": 1})
        under, over, paired = im.divergence(base, ref)
        self.assertEqual((under, over, paired), (1, 1, 0))
        self.assertIn("1 under-inline", im.divergence_note(base, ref))

    def test_note_records_the_discount(self):
        base = Counter({"?a@@YAXXZ": 1, "?b@@YAXXZ": 1})
        ref = Counter({"sub_1000": 1, "?c@@YAXXZ": 1})
        note = im.divergence_note(base, ref)
        self.assertIn("name-unresolvable pair", note)

    def test_exact_bytes_override_relocation_name_residue(self):
        # The normalized byte verdict is stronger than incomplete/synthetic
        # REL32 naming in a delinked target object.
        base = Counter({"?a@@YAXXZ": 2})
        ref = Counter()
        self.assertEqual(
            im.effective_divergence(base, ref, byte_exact=True),
            (0, 0, 0))

    def test_nonexact_bytes_do_not_hide_real_residue(self):
        # NEGATIVE CONTROL: without a current 100% verdict the same surplus
        # remains visible and routes to diagnosis.
        base = Counter({"?a@@YAXXZ": 2})
        ref = Counter()
        self.assertEqual(
            im.effective_divergence(base, ref, byte_exact=False),
            (2, 0, 0))


class NestedFrontier(unittest.TestCase):

    def test_outer_under_inner_over_is_identified(self):
        outer = "??1GameSelectionHeadersStruct@@QAE@XZ"
        inner = "??1SavedGameHeader@@QAE@XZ"
        under = [(outer, 3, 2)]
        over = [(inner, 0, 1)]
        calls = {outer: Counter({inner: 1})}
        self.assertEqual(
            im.nested_frontiers(under, over, calls),
            ((outer, inner, 1),))

    def test_unrelated_reciprocal_counts_are_not_called_nested(self):
        # NEGATIVE CONTROL: matching count directions alone do not prove a
        # nesting relationship; the outer callee must really call the inner.
        under = [("?outer@@YAXXZ", 2, 1)]
        over = [("?inner@@YAXXZ", 0, 1)]
        calls = {"?outer@@YAXXZ": Counter({"?other@@YAXXZ": 1})}
        self.assertEqual(im.nested_frontiers(under, over, calls), ())


if __name__ == "__main__":
    unittest.main()
