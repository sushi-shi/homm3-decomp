"""Gruntz data contracts with HoMM3/VC6 inputs and synthetic negative controls."""
import unittest
import struct
import tempfile
from pathlib import Path
from unittest import mock

class DataRelocsControls(unittest.TestCase):
    def test_compiler_literal_does_not_borrow_named_static_relocations(self):
        """Exercise Obj -> canon -> Resolver -> retail/paired oracle routing."""
        from types import SimpleNamespace

        from homm3.verify import data_relocs as dr

        def data_object(name):
            payload = b"literal\0"
            strings = name.encode("ascii") + b"\0"
            header = struct.pack("<HHIIIHH", 0x14c, 1, 0,
                                 60 + len(payload), 1, 0, 0)
            section = struct.pack("<8sIIIIIIHHI", b".data", 0, 0,
                                  len(payload), 60, 0, 0, 0, 0, 0xc0300040)
            symbol = struct.pack("<IIIhHBB", 0, 4, 0, 1, 0, 3, 0)
            return (header + section + payload + symbol
                    + struct.pack("<I", 4 + len(strings)) + strings)

        model = SimpleNamespace(functions=[], data=[SimpleNamespace(
            name="__$S", aliases=[], rva=0x1000, size=24)])
        image = SimpleNamespace(
            jmp_target=mock.Mock(return_value=None),
            relocs_in=mock.Mock(return_value=[(0x1000, 0x2000)]))
        with tempfile.TemporaryDirectory() as td:
            base, target = Path(td, "base.obj"), Path(td, "target.obj")
            with mock.patch("homm3.model.resolve", return_value=model), \
                 mock.patch("homm3.sema.image.retail", return_value=image), \
                 mock.patch.object(dr, "clean_units", return_value=set()), \
                 mock.patch.object(dr.pairscan, "pairs",
                                   return_value={"probe": (base, target)}):
                for name in ("_$S56", "$S56"):
                    with self.subTest(name=name):
                        base.write_bytes(data_object(name))
                        target.write_bytes(data_object(name))
                        image.relocs_in.reset_mock()
                        rows, unpaired, unresolved, stats, _dropped, eh = dr.scan()
                        self.assertEqual((rows, unpaired, unresolved, eh),
                                         ([], [], [], []))
                        self.assertEqual(stats["data symbols paired"], 1)
                        self.assertEqual(stats["data symbols pinned"], 0)
                        image.relocs_in.assert_not_called()

                # A genuinely named static must still use the retail oracle,
                # which catches its missing relocation in this negative control.
                base.write_bytes(data_object("__$S123"))
                target.write_bytes(data_object("__$S123"))
                rows, _unpaired, _unresolved, stats, _dropped, _eh = dr.scan()
                self.assertEqual([(r.verdict, r.oracle) for r in rows],
                                 [("MISSING", "retail")])
                self.assertEqual(stats["data symbols pinned"], 1)

    def test_an_any_comdat_number_is_not_an_associative_ordinal(self):
        """Integration control for the section-manifest consumer.

        cl 5 writes an `Any` COMDAT's own section number into the aux Number
        field.  Passing it through as an association produces an invalid
        `selection=2, associative_ordinal=N` manifest row which the delinker
        must reject.
        """
        import struct

        from homm3.delink import coffx, data_manifest

        header_size = 20
        section_size = 40
        raw_offset = header_size + section_size
        symbol_offset = raw_offset + 4
        header = struct.pack(
            "<HHIIIHH",
            0x14C,
            1,
            0,
            symbol_offset,
            2,
            0,
            0,
        )
        section = struct.pack(
            "<8sIIIIIIHHI",
            b".data\0\0\0",
            0,
            0,
            4,
            raw_offset,
            0,
            0,
            0,
            0,
            0xC0301040,
        )
        symbol = struct.pack("<8sIhHBB", b".data\0\0\0", 0, 1, 0, 3, 1)
        # Length=4, Number=1, Selection=Any.  Number is not an association.
        aux = struct.pack("<IHHIHB3x", 4, 0, 0, 0, 1, 2)
        payload = header + section + b"\0" * 4 + symbol + aux + struct.pack("<I", 4)

        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "probe.obj"
            path.write_bytes(payload)
            parsed = coffx.Obj(path).section_table[0]

        self.assertEqual(parsed["comdat"], 2)
        self.assertEqual(parsed["assoc"], 0)
        row = dict(parsed, object="probe.c", ordinal=1, rva=0x1000,
                   storage="data", provenance="selftest")
        line = data_manifest.section_manifest_bytes([row]).decode().splitlines()[1]
        self.assertEqual(line.split("\t")[7:10], ["2", "-", "data"])

    def test_an_injected_wrong_vtable_slot_is_caught(self):
        from types import SimpleNamespace
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        from homm3.verify import data_relocs as dr
        table = '??_7P'
        symbols = (_symbol(table, 0, 1, 0, 2),
                   _symbol('first', 0, 0, 0, 2), _symbol('second', 0, 0, 0, 2))
        def payload(swapped=False):
            slots = ((0, 2 if swapped else 1, 6), (4, 1 if swapped else 2, 6))
            data = bytearray(_coff((FixtureSection('.rdata', bytes(8), slots),), symbols))
            struct.pack_into('<I', data, 20 + 36, 0x40300040)
            return bytes(data)
        def binding(name, rva, size):
            return SimpleNamespace(name=name, rva=rva, size=size, aliases=[], unit="probe")
        model = SimpleNamespace(functions=[binding('first', 0x2000, 1),
                                           binding('second', 0x3000, 1)],
                                data=[binding(table, 0x1000, 8)])
        image = SimpleNamespace(jmp_target=lambda _rva: None,
                                relocs_in=lambda _lo, _hi: [(0x1000, 0x2000), (0x1004, 0x3000)])
        with tempfile.TemporaryDirectory() as td:
            base, target = Path(td, 'base.obj'), Path(td, 'target.obj')
            target.write_bytes(payload())
            with mock.patch('homm3.model.resolve', return_value=model), \
                 mock.patch('homm3.sema.image.retail', return_value=image), \
                 mock.patch.object(dr, 'clean_units', return_value=set()), \
                 mock.patch.object(dr.pairscan, 'pairs', return_value={'probe': (base, target)}):
                base.write_bytes(payload())
                self.assertEqual(dr.scan()[0], [])
                base.write_bytes(payload(swapped=True))
                self.assertTrue(any(r.verdict == 'WRONG' for r in dr.scan()[0]))


def _layout(types=()):
    from homm3.verify.layout import Layout
    return Layout({"types": list(types), "units": {}, "tree_hash": "ctrl"})

def _prim(t, sz):
    return {"k": "prim", "t": t, "sz": sz}

def _arr(el, n, t="T[]"):
    return {"k": "arr", "t": t, "sz": el["sz"] * n, "n": n, "el": el}

def _claim(rva, node, name="?g_probe@@3HA", extent=None, channel="src",
           unit="probe"):
    from homm3.verify.access_map import Claim
    return Claim(rva=rva, name=name, unit=unit, channel=channel, kind="",
                 section=".data", space="data",
                 extent=extent if extent is not None else (node or {}).get("sz")
                 or 4, node=node, pct=100.0)

class LayoutOracleControls(unittest.TestCase):
    def test_an_array_element_offset_is_absolute(self):
        """The measured own-goal: resolving +0x4 of `int[32]` returned the
        ELEMENT-relative 0, so every array offset past the first read as
        'lands INSIDE field [1]' - 42 fabricated width findings."""
        lay = _layout()
        node = _arr(_prim("int", 4), 32, "int[32]")
        f = lay.field_at(node, 4)
        self.assertEqual((f.off, f.size, f.path, f.tag), (4, 4, "[1]", ""))
        self.assertEqual(lay.field_at(node, 0x7C).off, 0x7C)
        self.assertEqual(lay.field_at(node, 0x80).tag, "out")

    def test_a_union_is_laid_out_but_never_adjudicated(self):
        lay = _layout()
        u = {"k": "rec", "t": "U", "sz": 8, "u": 1,
             "m": [[0, ".a", _prim("int", 4)], [0, ".b", _prim("float", 4)]]}
        self.assertFalse(lay.field_at(u, 0).resolved)

    def test_the_vptr_slot_is_tagged_not_reported_as_a_hole(self):
        lay = _layout()
        poly = {"k": "rec", "t": "CFoo", "sz": 8, "poly": 1,
                "m": [[0, ".__vfptr", _prim("void *", 4)],
                      [4, ".m_x", _prim("int", 4)]]}
        self.assertEqual(lay.field_at(poly, 0).path, ".__vfptr")
        plain = {"k": "rec", "t": "CBar", "sz": 8,
                 "m": [[4, ".m_x", _prim("int", 4)]]}
        self.assertEqual(lay.field_at(plain, 0).tag, "hole")

    def test_the_harvest_joins_every_src_claim_on_this_tree(self):
        """The oracle is only worth its verdicts if it actually reaches the
        claims: a silent join failure would look exactly like a clean tree."""
        from homm3.core.paths import BUILD
        from homm3.model import resolve
        from homm3.verify.layout import CACHE, harvest
        if not CACHE.is_file() or not (BUILD / "objdiff/base").is_dir():
            self.skipTest("layout cache absent (unbuilt tree)")
        lay, _p = harvest()
        miss = [b.name for b in resolve().data
                if b.channel == "src" and not lay.var(b.unit, b.name)]
        self.assertEqual(miss[:5], [])

class DataCoverageControls(unittest.TestCase):
    def _row(self, **kw):
        row = {"rva": 0x1000, "length": 16, "section": ".data",
               "verdict": "NONZERO", "addressed": 1, "touched": 8, "sites": 2,
               "payload_nonzero": 8, "relocs": 0, "prev_object": "probe",
               "prev_name": "?g_a@@3HA", "next_object": "probe",
               "next_name": "?g_b@@3HA", "first_bytes": "01"}
        row.update(kw)
        return row

    def test_a_touched_nonzero_gap_inside_one_unit_fails(self):
        from homm3.verify import data_coverage as dc
        self.assertEqual(len(dc.gate_rows([self._row()])), 1)

    def test_the_library_frontier_and_the_iat_do_not(self):
        from homm3.verify import data_coverage as dc
        self.assertEqual(dc.gate_rows([self._row(prev_object="library_data",
                                                 next_object="library_data")]),
                         [])
        self.assertEqual(dc.gate_rows([self._row(prev_object="probe",
                                                 next_object="other")]), [])
        self.assertEqual(dc.gate_rows([self._row(section=".idata")]), [])
        self.assertEqual(dc.gate_rows([self._row(touched=0, sites=0)]), [])
        self.assertEqual(dc.gate_rows([self._row(verdict="ZERO-GAP",
                                                 payload_nonzero=0)]), [])

    def test_a_folded_comdat_is_not_an_overlap_but_two_extents_are(self):
        from homm3.verify import data_coverage as dc
        folded = [{"rva": 0x1000, "size": 8, "name": "??_7C@@6B@",
                   "object": "a", "storage": "rdata"},
                  {"rva": 0x1000, "size": 8, "name": "??_7C@@6B@",
                   "object": "b", "storage": "rdata"}]
        self.assertEqual(dc.overlaps(folded), [])
        clash = folded + [{"rva": 0x1004, "size": 8, "name": "?g_x@@3HA",
                           "object": "c", "storage": "data"}]
        self.assertEqual(len(dc.overlaps(clash)), 1)

class SourceNameRewriteControls(unittest.TestCase):
    """The rewrite rules are COMPLETE, proven once per build over the corpus.

    Labelling spells every claim from source (core.msvc_names), so a rule gap
    can no longer hide as a silent per-claim drop - it has to fail here. The
    control is the same assertion the dropped per-claim authority check made,
    lifted to the whole claim set: for EVERY extracted source claim, the name
    equals the emitting base object's own symbol modulo the volatile ordinals
    both sides mask. Reading the objects is fine HERE; it is a test, not the
    extraction path.
    """

    @staticmethod
    def _corpus():
        from homm3.core.paths import BUILD
        from homm3.retail_labels import fragments
        base = BUILD / "objdiff/base"
        claims = [c for c in fragments.all_claims()
                  if c.channel == "src-DATA" and c.meta.get("defined") == "1"
                  and c.meta.get("type")]
        return base, claims

    #: every decoration cl 5.0 could have chosen instead - if one of THESE is
    #: in the object, the claim named the right body and spelled it wrong.
    @staticmethod
    def _alternate_spellings(name: str) -> set[str]:
        import re
        out = set()
        for n in {name, re.sub(r"@@([0-9])P", r"@@\1Q", name)}:
            for m in {n, n.removesuffix("$S"), n + "$S"}:
                out |= {m, "_" + m, m.removeprefix("_")}
        return out - {name}

    def test_every_source_claim_is_cls_own_spelling(self):
        from homm3.core.coff import Coff
        from homm3.core.msvc_names import mask
        from homm3.model import unmaterialized
        base, claims = self._corpus()
        if not claims:
            self.skipTest("no extracted claims - run `homm3 labels --all`")
        objs: dict[str, tuple[set[str], set[str]] | None] = {}
        for unit in {c.unit for c in claims}:
            path = base / f"{unit}.obj"
            if not path.is_file():
                objs[unit] = None
                continue
            coff = Coff(path)
            objs[unit] = ({mask(n) for n in coff.code_names()},
                          {mask(n) for n in coff.all_names()})
        absent = sorted(u for u, v in objs.items() if v is None)
        if absent:
            self.skipTest(f"{len(absent)} unit(s) have no base obj "
                          f"(e.g. {absent[0]}) - run `homm3 build`")
        # A header inline's macro reaches every including TU, but cl
        # materializes the COMDAT only where it is odr-used - so a claim with
        # no symbol in ITS OWN unit is expected. The rewrite is in question
        # only when NO unit claiming that (kind, rva, name) carries it.
        claimed, proven = {}, set()
        for c in claims:
            code, every = objs[c.unit]
            key = (c.kind, c.rva, mask(c.name))
            claimed.setdefault(key, []).append(c)
            if key[2] in (code if c.kind == "func" else every):
                proven.add(key)
        # ... and a gap is a SPELLING defect only if some other decoration of
        # the same claim IS in one of those objects. A gap with no spelling at
        # all is a missing body - a modelling question the Model reports.
        misspelled, bodiless = [], []
        for key in sorted(set(claimed) - proven):
            cs = claimed[key]
            alts = self._alternate_spellings(cs[0].name)
            hit = next((a for c in cs for a in sorted(alts)
                        if a in objs[c.unit][1]), None)
            row = f"{key[0]} 0x{key[1]:06x} {cs[0].name}"
            (misspelled if hit else bodiless).append(
                f"{row} -> cl spells it {hit}" if hit else row)
        self.assertFalse(
            misspelled,
            f"{len(misspelled)} of {len(claimed)} source claim(s) are spelled "
            f"differently by cl - the rewrite rules are incomplete "
            f"(first: {misspelled[0] if misspelled else ''})")
        # the bodiless class must stay LOUD somewhere: a claim with no other
        # spelling at its rva is a Model violation, one WITH another spelling
        # is recorded as that binding's alias. Nothing may be silent.
        gaps = {(c.kind, c.rva, mask(c.name)) for c in unmaterialized(
            [c for cs in claimed.values() for c in cs])}
        aliased = {rva for kind, rva, _n in proven if kind == "func"}
        unreported = sorted(
            k for k in set(claimed) - proven - gaps
            if k[0] == "func" and k[1] not in aliased)
        self.assertFalse(
            unreported,
            f"{len(unreported)} claim(s) match no object symbol and are "
            f"reported by nothing (first: {unreported[0] if unreported else ''})")

    def test_a_missing_rewrite_rule_fails_that_control(self):
        """The negative control: undo two rules, the corpus control must fail.

        A gate that would pass an incomplete rewrite is not a gate."""
        import io
        import re
        from homm3.retail_labels import fragments

        _base, claims = self._corpus()
        if not claims:
            self.skipTest("no extracted claims - run `homm3 labels --all`")

        def poisoned():
            out = []
            for c in claims:
                name = re.sub(r"@@([0-9])Q", r"@@\1P", c.name)   # undo Q -> P
                if name.endswith("$S"):                          # undo _x$S
                    name = (name[1:] if name.startswith("_") else name)[:-2]
                out.append(c._replace(name=name))
            return out

        with mock.patch.object(fragments, "all_claims", poisoned):
            case = SourceNameRewriteControls(
                "test_every_source_claim_is_cls_own_spelling")
            result = unittest.TextTestRunner(stream=io.StringIO()).run(
                unittest.TestSuite([case]))
        self.assertEqual(len(result.failures), 1,
                         "a broken rewrite rule did not fail the corpus control")
        self.assertIn("rewrite rules are incomplete", result.failures[0][1])

    def test_masking_never_merges_two_object_symbols(self):
        """The mask is only sound while it is injective per object."""
        from homm3.core.coff import Coff
        from homm3.core.msvc_names import mask
        base, _claims = self._corpus()
        objs = sorted(base.glob("*.obj"))
        if not objs:
            self.skipTest("no base objs")
        collisions = []
        for path in objs:
            try:
                names = Coff(path).all_names()
            except ValueError:
                continue
            seen: dict[str, str] = {}
            for name in sorted(names):
                other = seen.setdefault(mask(name), name)
                if other != name:
                    collisions.append(f"{path.stem}: {other} / {name}")
        self.assertFalse(collisions,
                         f"{len(collisions)} object symbol pair(s) mask "
                         f"together (first: {collisions[0] if collisions else ''})")

    def test_the_rewrite_rules_are_the_measured_ones(self):
        from homm3.core import msvc_names as m
        # the i386 COFF global prefix, applied to what LLVM did not mangle
        self.assertEqual(m.func("?Foo@C@@QAEXXZ"), "?Foo@C@@QAEXXZ")
        self.assertEqual(m.func("_stdcall_thing@8", decorated=True),
                         "_stdcall_thing@8")
        self.assertEqual(m.func("ordinary"), "_ordinary")
        # clang's array storage class
        self.assertEqual(m.data("?g_cmdBitTable@@3QBGB", internal=False),
                         "?g_cmdBitTable@@3QBGB")
        # TU-local storage: `_` and `$S` arrive together, whatever the mangling
        self.assertEqual(m.data("s_MAIN", internal=True), "_s_MAIN")
        self.assertEqual(m.data("_kDegToRad", internal=True, decorated=True),
                         "_kDegToRad")
        self.assertEqual(m.data("?s_x@?1??F@@QAEHXZ@4HA", internal=True),
                         "_?s_x@?1??F@@QAEHXZ@4HA")
        # the mask meets cl's own object on both ordinals
        self.assertEqual(m.mask("_?s_x@?BA@??F@@QAEHXZ@4HA$S35536"),
                         "_?s_x@?1??F@@QAEHXZ@4HA$S")
        self.assertEqual(m.mask("_?$S47@?1??G@@QAEHXZ@4EA$S20267"),
                         "_?$S@?1??G@@QAEHXZ@4EA$S")
        # an rva-keyed name is NOT an ordinal: it must survive masking
        self.assertEqual(m.mask("$S2277272"), "$S2277272")
        self.assertEqual(m.mask(m.discriminate("_s_x$S", 0x244970)), "_s_x$S")

class DataAlignmentPaddingControls(unittest.TestCase):
    """A named static's identity survives either allocator's alignment gap."""

    @staticmethod
    def obj(payload: bytes, successor: int | None, name="_s_msToSeconds$S7"):
        import struct
        rawptr = 60
        symptr = rawptr + len(payload)
        strings = bytearray(bytes(4))
        symbols = bytearray()
        rows = [(name, 0)] + ([("_next$S9", successor)] if successor else [])
        for symbol, value in rows:
            symbols += struct.pack("<II", 0, len(strings))
            strings += symbol.encode("latin1") + b"\0"
            symbols += struct.pack("<IhHBB", value, 1, 0, 3, 0)
        struct.pack_into("<I", strings, 0, len(strings))
        header = struct.pack("<HHIIIHH", 0x14c, 1, 0, symptr, len(rows), 0, 0)
        section = struct.pack("<8sIIIIIIHHI", b".rdata", 0, 0, len(payload),
                              rawptr, 0, 0, 0, 0, 0x40400040)
        return header + section + payload + symbols + strings

    @staticmethod
    def canonical(data: bytes) -> str:
        from homm3.build.canonicalize_data_symbols import canonicalize_coff
        rows = canonicalize_coff(data).rows
        return next(row.canonical_name for row in rows
                    if row.original_name.startswith("_s_msToSeconds"))

    def test_padded_and_packed_spans_share_one_identity(self):
        value = bytes.fromhex("6f12833a")
        packed = self.obj(value + bytes.fromhex("0000803f"), 4)
        padded = self.obj(value + bytes(4) + bytes(8), 8)
        self.assertEqual(self.canonical(packed), self.canonical(padded))

    def test_content_beyond_the_gap_still_decides_identity(self):
        value = bytes.fromhex("6f12833a")
        self.assertNotEqual(self.canonical(self.obj(value + bytes(4), None)),
                            self.canonical(self.obj(value + bytes([0, 0, 0, 1]), None)))
        self.assertNotEqual(self.canonical(self.obj(value + bytes(12), None)),
                            self.canonical(self.obj(value, None)))
