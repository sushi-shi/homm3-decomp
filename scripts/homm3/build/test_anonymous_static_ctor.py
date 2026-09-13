"""Bind VC6 anonymous-namespace initializers through their owned data edge."""
import unittest
import warnings

from homm3.build import canonicalize_data_symbols as cds
from homm3.build.test_ownerless_static_dtor import FakeCoff, _fn, _local_data, _ext, _reloc


CLAIM = cds.CompgenClaim("__h3cg$rmg$static_ctor$g_offsets", "STATIC_CTOR", "g_offsets", 0x20)


class AnonymousStaticConstructorTest(unittest.TestCase):
    def bind(self, symbols, relocations):
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            names, rows = cds._compgen_renames(FakeCoff(symbols, relocations), (CLAIM,))
        return names, rows, caught

    def test_owned_anonymous_global_is_independent_of_nonce(self):
        for source in (r"Z:\one\rmg.cpp123", r"Z:\two\rmg.cpp999"):
            with self.subTest(source=source):
                names, rows, caught = self.bind(
                    [_fn(1, "_$E32", 0), _local_data(2, "?g_offsets@?%" + source + "@@3PAUTPoint@@A")],
                    [_reloc(2, 2)])
                self.assertEqual(names, {1: CLAIM.name})
                self.assertEqual(rows[0].proof, "semantic-relocation-role")
                self.assertFalse(caught)

    def test_wrong_owner_and_named_namespace_do_not_match(self):
        for name in ("?g_offsetsExtra@?%rmg.cpp123@@3HA", "?other@?%rmg.cpp123@@3HA",
                     "?g_offsets@Named@@3HA"):
            with self.subTest(name=name):
                self.assertEqual(self.bind([_fn(1, "_$E32", 0), _local_data(2, name)], [_reloc(2, 2)])[0], {})

    def test_exit_registration_is_not_a_constructor(self):
        names, _, _ = self.bind(
            [_fn(1, "_$E32", 0), _local_data(2, "?g_offsets@?%rmg.cpp123@@3HA"), _ext(3, "_atexit")],
            [_reloc(2, 2), _reloc(10, 3)])
        self.assertEqual(names, {})

    def test_two_same_named_anonymous_owners_stay_ambiguous(self):
        names, _, caught = self.bind(
            [_fn(1, "_$E32", 0), _fn(2, "_$E35", 0x40),
             _local_data(3, "?g_offsets@?%first.cpp123@@3HA"),
             _local_data(4, "?g_offsets@?%second.cpp456@@3HA")],
            [_reloc(2, 3), _reloc(0x42, 4)])
        self.assertEqual(names, {})
        self.assertTrue(any("2 semantic compiler-function candidates" in str(w.message) for w in caught))


if __name__ == "__main__":
    unittest.main()
