#!/usr/bin/env python3
"""Hermetic policy tests for ``homm3 vc6 why-reg`` mutations.

The source cleanliness gate rejects ``volatile`` qualifiers.  The guided
solver must therefore never recommend adding one as a register-allocation
lever, even when that mutation would improve its instruction-distance score.
"""
from __future__ import annotations

import unittest

from homm3.vc6 import reg_model


class MutationPolicyTest(unittest.TestCase):
    def test_volatile_is_not_an_offered_mutation(self):
        source = ("int sample(int value)\n"
                  "{\n"
                  "    int homed = value;\n"
                  "    return homed;\n"
                  "}\n")
        mutations = reg_model._mutations(source, "sample")
        self.assertTrue(mutations)
        self.assertFalse(any(
            "volatile" in mutation["label"]
            or "volatile" in mutation["text"]
            for mutation in mutations))


if __name__ == "__main__":
    unittest.main()
