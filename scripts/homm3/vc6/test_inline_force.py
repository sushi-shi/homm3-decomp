"""Tooling contracts of the inline decision-forcing oracle (no compiler runs)."""
from __future__ import annotations

import unittest
from unittest.mock import patch

from homm3.vc6 import inline_force

LOG = """sym 00000010 ?root@@YAXXZ
sym 00000020 ?wrap@@YAXXZ
sym 00000030 ?leaf@@YAHXZ
main 00000010 cb=600
site root=00000010 owner=00000010 callee=00000020 cb=99 budget=1200 depth=1 remain=3 running=600
site root=00000010 owner=00000020 callee=00000030 cb=45 budget=60 depth=2 remain=1 running=699
site root=00000010 owner=00000010 callee=00000020 cb=99 budget=1101 depth=1 remain=2 running=744
site root=00000010 owner=00000020 callee=00000030 cb=45 budget=20 depth=2 remain=1 running=843 force=E
site root=00000099 owner=00000099 callee=00000030 cb=45 budget=9 depth=1 remain=1 running=1
"""


class RuleTest(unittest.TestCase):
    def test_line_uses_wildcards_and_rejects_tabs(self):
        self.assertEqual(inline_force.Rule("", "?leaf", 2, "K").line(), "*\t?leaf\t2\tK")
        with self.assertRaises(ValueError):
            inline_force.Rule("a\tb", "c", 1, "E").line()
        with self.assertRaises(ValueError):
            inline_force.Rule("a", "c", 1, "X").line()

    def test_long_names_become_fitting_prefixes(self):
        line = inline_force.Rule("?" + "o" * 300, "?" + "c" * 300, 1, "E").line()
        owner, callee, _, _ = line.split("\t")
        self.assertEqual((len(owner), len(callee)), (95, 159))


class ParseSitesTest(unittest.TestCase):
    def test_root_filter_occurrence_and_admission(self):
        sites = inline_force.parse_sites(LOG, "?root@@YAXXZ")
        self.assertEqual(len(sites), 4)  # the foreign root's site is dropped
        leaf = [s for s in sites if s["callee"] == "?leaf@@YAHXZ"]
        self.assertEqual([s["occurrence"] for s in leaf], [1, 2])
        self.assertEqual([s["admitted"] for s in leaf], [True, True])
        self.assertEqual(leaf[1]["force"], "E")
        self.assertEqual(leaf[1]["owner"], "?wrap@@YAXXZ")

    def test_budget_refusal_is_not_admitted(self):
        text = LOG.replace(" force=E", "")
        leaf = [s for s in inline_force.parse_sites(text, "?root@@YAXXZ")
                if s["callee"] == "?leaf@@YAHXZ"]
        self.assertFalse(leaf[1]["admitted"])


class DeriveRulesTest(unittest.TestCase):
    def test_expand_earliest_kept_and_keep_latest_admitted(self):
        sites = inline_force.parse_sites(LOG.replace(" force=E", ""), "?root@@YAXXZ")
        divergence = {"under": [("?leaf@@YAHXZ", 1, 0)], "over": [("?wrap@@YAXXZ", 0, 1)]}
        with patch.object(inline_force.inline_model, "ordered_divergence",
                          return_value=divergence):
            rules = inline_force.derive_rules(sites, "", "", [])
        self.assertEqual([(r.callee, r.occurrence, r.action) for r in rules],
                         [("?leaf@@YAHXZ", 2, "E"), ("?wrap@@YAXXZ", 2, "K")])

    def test_existing_rules_are_not_repeated(self):
        sites = inline_force.parse_sites(LOG.replace(" force=E", ""), "?root@@YAXXZ")
        fixed = [inline_force.Rule("?wrap@@YAXXZ", "?leaf@@YAHXZ", 2, "E")]
        divergence = {"under": [("?leaf@@YAHXZ", 1, 0)], "over": []}
        with patch.object(inline_force.inline_model, "ordered_divergence",
                          return_value=divergence):
            self.assertEqual(inline_force.derive_rules(sites, "", "", fixed), [])


class DeficitTest(unittest.TestCase):
    def test_forced_expand_reports_budget_shortfall(self):
        sites = inline_force.parse_sites(LOG, "?root@@YAXXZ")
        rows = inline_force.deficits(sites)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["need"], "budget +25")


class ParseColorsTest(unittest.TestCase):
    def test_root_filter_and_eligible_mask(self):
        text = LOG + ("color root=00000010 k=1 chosen=7 eligible=000001C2 priority=-3\n"
                      "color root=00000099 k=1 chosen=1 eligible=00000002 priority=9\n"
                      "color root=00000010 k=2 chosen=2 eligible=00000006 priority=4 forced=1\n")
        rows = inline_force.parse_colors(text, "?root@@YAXXZ")
        self.assertEqual([r["k"] for r in rows], [1, 2])
        self.assertEqual(rows[0]["eligible"], [1, 6, 7, 8])
        self.assertEqual((rows[0]["priority"], rows[1]["forced"]), (-3, 1))


class StrictStreamTest(unittest.TestCase):
    def test_switch_table_addend_is_masked(self):
        text = ("00000000 <?f@@YAXXZ>:\n"
                "       0: ff 24 9d 58 0a 00 00         \tjmp\tdword ptr [4*ebx + 0xa58]\n")
        other = text.replace("58 0a", "00 00").replace(" + 0xa58", "")
        self.assertEqual(inline_force.strict_stream(text), inline_force.strict_stream(other))


if __name__ == "__main__":
    unittest.main()
