"""Registration proof accepts globals-only teardown and rejects false ownership."""
from types import SimpleNamespace
import unittest
import warnings

from homm3.build.canonicalize_data_symbols import (
    CompgenClaim, Symbol, Relocation, _compgen_renames,
    DIR32, FUNCTION_TYPE, MEM_EXECUTE)


def fixture(owner="holder", callback_opcode=0x68, addend=0, second=False, teardown=True):
    symbols = {
        1: Symbol(1, 0, "?Init@@YAXXZ", 0, 1, FUNCTION_TYPE, 2, 0),
        2: Symbol(2, 0, "_$E7", 0, 2, FUNCTION_TYPE, 3, 0),
        3: Symbol(3, 0, f"_?{owner}@?2??Init@@YAXXZ@4VHolder@@A", 0, 3, 0, 3, 0),
        4: Symbol(4, 0, "_atexit", 0, 0, 0, 2, 0),
        5: Symbol(5, 0, "??3@YAXPAX@Z" if teardown else "_ordinary", 0, 0, 0, 2, 0),
        6: Symbol(6, 0, "_?$S6@?2??Init@@YAXXZ@4EA", 1, 3, 0, 3, 0),
    }
    parent = b"\xb9\0\0\0\0" + bytes([callback_opcode]) + addend.to_bytes(4, "little") + b"\xe8\0\0\0\0\xc3"
    bodies = [parent, b"\xe8\0\0\0\0\xc3", b"\0\0"]
    refs = [Relocation(1, 1, 3, DIR32), Relocation(1, 6, 2, DIR32),
            Relocation(1, 11, 4, 0x14), Relocation(2, 1, 5, 0x14)]
    if second:
        symbols[7] = Symbol(7, 0, "_$E8", 0, 4, FUNCTION_TYPE, 3, 0)
        bodies[0] = parent[:-1] + b"\x68\0\0\0\0\xe8\0\0\0\0\xc3"
        bodies.append(bodies[1])
        refs += [Relocation(1, 16, 7, DIR32), Relocation(1, 21, 4, 0x14),
                 Relocation(4, 1, 5, 0x14)]
    sections = [SimpleNamespace(index=i+1, raw_size=len(body), characteristics=MEM_EXECUTE)
                for i, body in enumerate(bodies)]
    return SimpleNamespace(symbols=symbols, sections=sections, relocations=refs,
                           section_bytes=lambda section: bodies[section.index - 1])


class GlobalStaticDestructorTest(unittest.TestCase):
    def bind(self, coff, size=6):
        claim = CompgenClaim("__h3cg$static_dtor$holder", "STATIC_DTOR", "holder", size)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            return _compgen_renames(coff, (claim,))

    def test_globals_only_dtor_uses_owner_registration(self):
        names, rows = self.bind(fixture())
        self.assertEqual(names, {2: "__h3cg$static_dtor$holder"})
        self.assertEqual(rows[0].physical_size, 6)

    def test_global_cleanup_need_not_call_delete_or_a_destructor(self):
        # A callback may close a global resource through an ordinary helper.
        self.assertEqual(self.bind(fixture(teardown=False))[0],
                         {2: "__h3cg$static_dtor$holder"})

    def test_global_cleanup_without_registration_is_not_a_static_destructor(self):
        coff = fixture(teardown=False)
        coff.relocations = [r for r in coff.relocations if r.symbol_index != 4]
        self.assertEqual(self.bind(coff), ({}, ()))

    def test_wrong_owner_call_instead_of_push_and_nonzero_callback_addend_fail(self):
        for kwargs in ({"owner": "other"}, {"callback_opcode": 0xe8},
                       {"addend": 1}, {"second": True}):
            with self.subTest(kwargs=kwargs):
                self.assertEqual(self.bind(fixture(**kwargs)), ({}, ()))

    def test_extent_gate_still_rejects_a_wrong_sized_claim(self):
        with self.assertRaises(ValueError):
            self.bind(fixture(), size=7)


if __name__ == "__main__":
    unittest.main()
