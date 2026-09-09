"""Pair naturally emitted nested-vector copy loops without aliasing scalar copy."""
import unittest

from homm3.retail_labels import source


class StdCopyVectorKeyTest(unittest.TestCase):
    def test_artifact_vector_copy(self):
        symbol = ("?copy@std@@YIPAV?$vector@Utype_artifact@@"
                  "V?$allocator@Utype_artifact@@@std@@@1@PBV21@0PAV21@@Z")
        self.assertEqual(source._demangle_key(symbol),
                         "type_artifact_vector@std_copy")

    def test_scalar_and_backward_copy_stay_distinct(self):
        symbols = [
            "?copy@std@@YIPAUtype_artifact@@PBU2@0PAU2@@Z",
            "?copy_backward@std@@YIPAV?$vector@Utype_artifact@@"
            "V?$allocator@Utype_artifact@@@std@@@1@PAV21@00@Z",
        ]
        for symbol in symbols:
            self.assertNotEqual(source._demangle_key(symbol),
                                "type_artifact_vector@std_copy")


if __name__ == "__main__":
    unittest.main()
