"""A placed vtordisp thunk's jump to the weak `??_E` names the scalar `??_G`
it defaults to, so placement reaches the `??_G` body."""
import unittest

from homm3.build.test_weak_default_normalization import SCALAR, THUNK, WEAK, weak_coff
from homm3.census.placements import _functions_of
from homm3.delink.coffx import Obj


def thunk_referent(payload: bytes) -> str:
    rows = {name: relocs for name, _sec, _off, _body, relocs in _functions_of(Obj(payload))}
    ((referent, _kind),) = rows[THUNK].values()
    return referent


class WeakDefaultPlacementTest(unittest.TestCase):
    def test_the_thunk_names_the_defined_scalar_destructor(self):
        self.assertEqual(thunk_referent(weak_coff()), SCALAR)

    def test_an_undefined_default_keeps_the_weak_name(self):
        self.assertEqual(thunk_referent(weak_coff(define_scalar=False)), WEAK)


if __name__ == "__main__":
    unittest.main()
