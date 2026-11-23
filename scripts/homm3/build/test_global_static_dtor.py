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

    def interleaved(self, opcode):
        # push OFFSET $E7; <opcode> [owner]/call; call _atexit
        coff = fixture()
        parent = (b"\xb9\0\0\0\0\x68\0\0\0\0" + bytes([opcode]) + b"\0\0\0\0"
                  + b"\xe8\0\0\0\0\xc3")
        bodies = [parent, b"\xe8\0\0\0\0\xc3", b"\0\0"]
        coff.symbols[8] = Symbol(8, 0, "_other", 0, 0, 0, 2, 0)
        middle = (Relocation(1, 11, 3, DIR32) if opcode == 0xa3
                  else Relocation(1, 11, 8, 0x14))
        coff.relocations = [Relocation(1, 1, 3, DIR32), Relocation(1, 6, 2, DIR32),
                            middle, Relocation(1, 16, 4, 0x14),
                            Relocation(2, 1, 5, 0x14)]
        coff.sections[0].raw_size = len(parent)
        coff.section_bytes = lambda section: bodies[section.index - 1]
        return self.bind(coff)[0]

    def test_owner_stores_may_separate_the_push_from_atexit(self):
        # createRiver: the table's stores are scheduled between the two.
        self.assertEqual(self.interleaved(0xa3), {2: "__h3cg$static_dtor$holder"})

    def test_another_call_between_push_and_atexit_is_not_registration(self):
        self.assertEqual(self.interleaved(0xe8), {})

    def test_extent_gate_still_rejects_a_wrong_sized_claim(self):
        with self.assertRaises(ValueError):
            self.bind(fixture(), size=7)


def dispatcher_fixture(extra=False):
    """apHeroFlagSprite's shape (h3maped ObjectSprites.obj): the dispatcher
    `call $E256; push OFFSET $E257; call _atexit; pop ecx; ret` names the
    datum nowhere, the constructor thunk stores into it, and the empty
    teardown is a bare `ret`. With `extra`, the dispatcher also calls an
    unrelated function before returning."""
    symbols = {
        1: Symbol(1, 0, "_$E259", 0, 1, FUNCTION_TYPE, 3, 0),
        2: Symbol(2, 0, "_$E256", 0, 2, FUNCTION_TYPE, 3, 0),
        3: Symbol(3, 0, "_$E257", 0, 3, FUNCTION_TYPE, 3, 0),
        4: Symbol(4, 0, "?holder@@3VHolder@@A", 0, 0, 0, 2, 0),
        5: Symbol(5, 0, "_atexit", 0, 0, 0, 2, 0),
        6: Symbol(6, 0, "_other", 0, 0, 0, 2, 0),
    }
    dispatcher = b"\xe8\0\0\0\0\x68\0\0\0\0\xe8\0\0\0\0\x59"
    refs = [Relocation(1, 1, 2, 0x14), Relocation(1, 6, 3, DIR32),
            Relocation(1, 11, 5, 0x14), Relocation(2, 2, 4, DIR32)]
    if extra:
        dispatcher += b"\xe8\0\0\0\0"
        refs.append(Relocation(1, 17, 6, 0x14))
    bodies = [dispatcher + b"\xc3", b"\x83\x25\0\0\0\0\0\xc3", b"\xc3"]
    sections = [SimpleNamespace(index=i+1, raw_size=len(body), characteristics=MEM_EXECUTE)
                for i, body in enumerate(bodies)]
    return SimpleNamespace(symbols=symbols, sections=sections, relocations=refs,
                           section_bytes=lambda section: bodies[section.index - 1])


CTOR = CompgenClaim("__h3cg$static_ctor$holder", "STATIC_CTOR", "holder", 0)
DTOR = CompgenClaim("__h3cg$static_dtor$holder", "STATIC_DTOR", "holder", 1)


class DispatcherRegisteredStaticDestructorTest(unittest.TestCase):
    def bind(self, coff, claims=(CTOR, DTOR)):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            return _compgen_renames(coff, claims)[0]

    def test_the_bound_constructor_s_dispatcher_proves_the_empty_teardown(self):
        self.assertEqual(self.bind(dispatcher_fixture()),
                         {2: CTOR.name, 3: DTOR.name})

    def test_claim_order_does_not_matter(self):
        self.assertEqual(self.bind(dispatcher_fixture(), (DTOR, CTOR)),
                         {2: CTOR.name, 3: DTOR.name})

    def test_without_the_constructor_claim_the_teardown_has_no_owner(self):
        self.assertEqual(self.bind(dispatcher_fixture(), (DTOR,)), {})

    def test_another_owner_s_constructor_is_no_evidence(self):
        other = CompgenClaim("__h3cg$static_dtor$other", "STATIC_DTOR", "other", 1)
        self.assertEqual(self.bind(dispatcher_fixture(), (CTOR, other)), {2: CTOR.name})

    def test_a_dispatcher_reaching_anything_else_is_no_evidence(self):
        self.assertEqual(self.bind(dispatcher_fixture(extra=True)), {2: CTOR.name})


if __name__ == "__main__":
    unittest.main()
