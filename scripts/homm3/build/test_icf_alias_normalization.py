"""ICF twin references compare by name only when their bodies are identical."""
import unittest

from homm3.build.normalize_objs import FunctionBody, _icf_identical


class IcfIdenticalTest(unittest.TestCase):
    def test_relocation_fields_and_linker_fill_are_ignored(self):
        candidate = FunctionBody(b"\x8b\x41\x04\xe9\x01\x02\x03\x04", (4,))
        retail = FunctionBody(b"\x8b\x41\x04\xe9\x00\x00\x00\x00\xcc\xcc", (4,))
        self.assertTrue(_icf_identical(candidate, retail))

    def test_different_code_or_sites_stay_visible(self):
        stores_vtable = FunctionBody(b"\xc7\x01\x00\x00\x00\x00\xe9\x00\x00\x00\x00",
                                     (2, 7))
        jump_only = FunctionBody(b"\xe9\x00\x00\x00\x00\x90\x90\x90", (1,))
        self.assertFalse(_icf_identical(stores_vtable, jump_only))
        self.assertFalse(_icf_identical(FunctionBody(b"\x33\xc0\xc3", ()),
                                        FunctionBody(b"\x33\xc9\xc3", ())))

    def test_trailing_code_is_not_fill(self):
        self.assertFalse(_icf_identical(FunctionBody(b"\xc3", ()),
                                        FunctionBody(b"\xc3\xc3", ())))


if __name__ == "__main__":
    unittest.main()
