"""Run the independent 3x3 sample-lattice oracle on quadrant lifetimes."""
import unittest
from homm3.vc6.test_rmg_families import generator

_base = generator("test_rmg_noise_midpoint.py")


class NoiseQuadrantTests(_base.NoiseMidpointTests):
    def setUp(self):
        super().setUp()
        self.module = generator("generate-rmg-noise-quadrant-family.py")

    def test_manifest(self):
        axis = self.module.make_axes(self.source)[0]
        self.assertEqual(len(axis["options"]), 12)
        self.assertEqual(axis["options"][0]["replace"], axis["find"])
        self.assertEqual(len({option["replace"] for option in axis["options"]}), 12)


if __name__ == "__main__":
    unittest.main()
