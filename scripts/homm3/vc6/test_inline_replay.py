"""The /Ob2 budget replay reproduces traced roots and answers what-ifs."""
import unittest

from homm3.vc6 import inline_replay


def site(depth, budget, remaining, cb, running, name):
    return dict(depth=depth, budget=budget, remaining=remaining, cb=cb, running=running,
                symbol=name, owner="root", budget_allows=cb <= 40 or budget >= cb)


ROOT = dict(symbol="root", cb=400, initial_budget=1000, sites=[
    site(1, 1000, 3, 56, 400, "wrapper"),
    site(2, 314, 1, 48, 456, "ctor"),
    site(1, 896, 2, 30, 552, "accessor"),
    site(1, 896, 1, 48, 582, "ctor"),
])


class InlineReplayTests(unittest.TestCase):
    def test_reproduces_budgets_verdicts_and_sizes(self):
        self.assertTrue(inline_replay.reproduces(ROOT))
        changed = dict(ROOT, sites=[dict(s) for s in ROOT["sites"]])
        changed["sites"][1]["budget"] = 313
        self.assertFalse(inline_replay.reproduces(changed))

    def test_free_sites_lower_a_nested_budget_below_the_cost(self):
        rows = inline_replay.replay(ROOT, extra_sites={2: 17})
        nested = rows[1]
        self.assertEqual(nested["budget"], (1000 - 56) // 20)
        self.assertFalse(nested["expands"])
        self.assertTrue(rows[0]["expands"])

    def test_a_smaller_caller_has_less_budget(self):
        rows = inline_replay.replay(ROOT, cb=450)
        self.assertEqual(rows[0]["budget"], 1000)
        self.assertEqual(inline_replay.replay(ROOT, cb=600)[0]["budget"], 1200)

    def test_nothing_expands_past_the_size_cap(self):
        big = dict(ROOT, cb=34990, initial_budget=35000, sites=[
            site(1, 35000, 2, 50, 34990, "first"),
            site(1, 34950, 1, 50, 35040, "second")])
        rows = inline_replay.replay(big)
        self.assertEqual([r["expands"] for r in rows], [True, False])
        self.assertTrue(rows[1]["budget_allows"])


if __name__ == "__main__":
    unittest.main()
