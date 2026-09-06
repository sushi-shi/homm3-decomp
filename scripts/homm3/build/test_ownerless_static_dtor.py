"""Negative control for the owner-free static-destructor arm.

`_compgen_renames` binds a STATIC_DTOR claim to one of the object's
volatile `$E<n>` functions by looking for a relocation to the OWNED DATUM -
`owner_present`.  That is right whenever the destructor touches the object
it is destroying, and wrong for an EMPTY holder.

game.obj's `immMouse` (retail 0x4b6910, 58 B) is the shape: the holder is
eight bytes of nothing and `TImmMouseRuntime::~TImmMouseRuntime` closes and
deletes two OTHER globals, so once VC6 expands the destructor into the
thunk the unused `this` - and with it the only relocation naming the datum
- disappears.  The strict arms then report ZERO candidates and the claim is
left unbound with a warning, which is exactly what happened before this
arm existed.

The relaxed arm runs only as a second pass over claims the strict arms left
unbound, so it can never take a candidate away from an owner-present claim;
the last two cases pin that and the `_atexit` refusal.
"""

import unittest
import warnings

from homm3.build import canonicalize_data_symbols as cds


TEXT = cds.Section(index=1, header_offset=0, name=".text", raw_size=0x80,
                   raw_offset=0, reloc_offset=0, reloc_count=0,
                   characteristics=cds.MEM_EXECUTE)
DATA = cds.Section(index=2, header_offset=0, name=".data", raw_size=0x40,
                   raw_offset=0, reloc_offset=0, reloc_count=0,
                   characteristics=0xC0000040)


class FakeCoff:
    """The three attributes and one method `_compgen_renames` reads."""

    def __init__(self, symbols, relocations):
        self.sections = [TEXT, DATA]
        self.symbols = {symbol.index: symbol for symbol in symbols}
        self.relocations = list(relocations)

    def section_bytes(self, section):
        # all-`nop` so validate_extent accepts any claimed size
        return b"\x90" * section.raw_size


def _fn(index, name, value):
    return cds.Symbol(index=index, offset=0, name=name, value=value,
                      section=1, typ=cds.FUNCTION_TYPE,
                      storage_class=cds.STATIC_STORAGE, aux_count=0)


def _local_data(index, name):
    return cds.Symbol(index=index, offset=0, name=name, value=0, section=2,
                      typ=0, storage_class=cds.STATIC_STORAGE, aux_count=0)


def _ext(index, name):
    return cds.Symbol(index=index, offset=0, name=name, value=0, section=0,
                      typ=0, storage_class=cds.EXTERNAL_STORAGE, aux_count=0)


def _reloc(site, symbol_index):
    return cds.Relocation(section=1, site=site, symbol_index=symbol_index,
                          typ=cds.DIR32)


CLAIM = cds.CompgenClaim(name="__h3cg$game$static_dtor$immMouse",
                         kind="STATIC_DTOR", owner="immMouse", size=0x3a)


def _renames(symbols, relocations, claims=(CLAIM,)):
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always")
        renames, rows = cds._compgen_renames(
            FakeCoff(symbols, relocations), tuple(claims))
    return renames, rows, [str(w.message) for w in caught]


class OwnerlessStaticDtorTest(unittest.TestCase):
    def test_the_ownerless_teardown_binds(self):
        # THE defect: the expanded destructor names gImmProject, the
        # imported ~CImmProject and operator delete, and never `immMouse`.
        symbols = [_fn(1, "_$E77", 0),
                   _ext(2, "?gImmProject@@3PAVCImmProject@@A"),
                   _ext(3, "__imp_??1CImmProject@@QAE@XZ"),
                   _ext(4, "??3@YAXPAX@Z")]
        relocations = [_reloc(2, 2), _reloc(9, 3), _reloc(0x20, 4)]
        renames, rows, caught = _renames(symbols, relocations)
        self.assertEqual(renames, {1: CLAIM.name})
        self.assertEqual(caught, [])
        self.assertEqual(rows[0].proof, "semantic-relocation-role")

    def test_an_owner_present_teardown_still_binds_the_strict_way(self):
        # the established shape must be untouched
        symbols = [_fn(1, "_$E77", 0),
                   _ext(2, "_immMouse"),
                   _ext(3, "??1TImmMouseRuntime@@QAE@XZ")]
        renames, _rows, caught = _renames(symbols, [_reloc(2, 2),
                                                    _reloc(9, 3)])
        self.assertEqual(renames, {1: CLAIM.name})
        self.assertEqual(caught, [])

    def test_an_atexit_registrar_is_refused(self):
        # the ctor-side role: `$E<n>` that registers rather than tears down
        symbols = [_fn(1, "_$E77", 0),
                   _ext(2, "_atexit"),
                   _ext(3, "??3@YAXPAX@Z")]
        renames, _rows, caught = _renames(symbols, [_reloc(2, 2),
                                                    _reloc(9, 3)])
        self.assertEqual(renames, {})
        self.assertEqual(len(caught), 1)
        self.assertIn("leaving claim unbound", caught[0])

    def test_two_ownerless_teardowns_leave_the_claim_unbound(self):
        # the relaxed arm must not guess between two candidates
        symbols = [_fn(1, "_$E77", 0), _fn(2, "_$E80", 0x40),
                   _ext(3, "??3@YAXPAX@Z")]
        renames, _rows, caught = _renames(symbols, [_reloc(2, 3),
                                                    _reloc(0x42, 3)])
        self.assertEqual(renames, {})
        self.assertEqual(len(caught), 1)
        self.assertIn("has 2 semantic compiler-function candidates", caught[0])

    def test_a_teardown_naming_THIS_object_s_own_datum_is_not_ownerless(self):
        # game.obj's `_$E80`: a vector teardown that pushes the address of
        # its own TPickRandomTownName array. It HAS an owner, so it must
        # not compete for the ownerless claim - and with it excluded the
        # immMouse thunk is the single candidate again.
        symbols = [_fn(1, "_$E77", 0), _fn(2, "_$E80", 0x40),
                   _ext(3, "??3@YAXPAX@Z"),
                   _ext(4, "??1TPickRandomTownName@@QAE@XZ"),
                   _local_data(5, "?gRandomTownNames@@3PAVTPickRandomTownName@@A")]
        relocations = [_reloc(2, 3),
                       _reloc(0x42, 5), _reloc(0x48, 4)]
        renames, _rows, caught = _renames(symbols, relocations)
        self.assertEqual(renames, {1: CLAIM.name})
        self.assertEqual(caught, [])

    def test_the_relaxed_arm_never_steals_from_a_strict_binding(self):
        # two claims, two thunks: the owner-present one must take its own
        # candidate first, leaving the ownerless one for the second pass
        other = cds.CompgenClaim(name="__h3cg$game$static_dtor$gTimers",
                                 kind="STATIC_DTOR", owner="gTimers",
                                 size=0x10)
        symbols = [_fn(1, "_$E77", 0), _fn(2, "_$E80", 0x40),
                   _ext(3, "??3@YAXPAX@Z"),
                   _local_data(4, "_gTimers"),
                   _ext(5, "??1TTimerTable@@QAE@XZ")]
        relocations = [_reloc(2, 3),
                       _reloc(0x42, 4), _reloc(0x48, 5)]
        renames, _rows, caught = _renames(symbols, relocations,
                                          claims=(CLAIM, other))
        self.assertEqual(renames, {1: CLAIM.name, 2: other.name})
        self.assertEqual(caught, [])


if __name__ == "__main__":
    unittest.main()
