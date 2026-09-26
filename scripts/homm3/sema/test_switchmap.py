"""Switch mapping catches behavioral permutations hidden by aggregate scores."""
import unittest

from homm3.sema.switchmap import Case, Switch, compare


class SwitchMapTest(unittest.TestCase):
    def test_unique_case_permutation_is_reported(self):
        base = [Switch(80, None, (Case(0, 4, frozenset({"imm:a"})),
                                  Case(1, 8, frozenset({"imm:b"}))))]
        retail = [Switch(80, None, (Case(0, 40, frozenset({"imm:b"})),
                                    Case(1, 44, frozenset({"imm:a"}))))]
        self.assertEqual([value for _, value, _ in compare(base, retail)], [0, 1])

    def test_relocated_arms_with_same_cases_are_not_reported(self):
        base = [Switch(80, None, (Case(0, 4, frozenset({"imm:a"})),
                                  Case(1, 4, frozenset({"imm:a"})),
                                  Case(2, 8, frozenset({"imm:b"}))))]
        retail = [Switch(90, None, (Case(0, 40, frozenset({"imm:a"})),
                                    Case(1, 40, frozenset({"imm:a"})),
                                    Case(2, 44, frozenset({"imm:b"}))))]
        self.assertEqual(compare(base, retail), [])


if __name__ == "__main__":
    unittest.main()
