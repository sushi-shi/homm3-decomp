#!/usr/bin/env python3
"""Controls for relocation pairing: votes, verdicts, folds and the rewrite."""
from __future__ import annotations

import struct
import unittest

from homm3.delink import reloc_pairing as rp

DIR32, REL32 = 0x6, 0x14
BASE = 0x400000


def coff(functions, *, externals=(), statics=()):
    """A minimal i386 COFF: one .text section holding `functions`
    [(name, body, [(site, symbol name, type)])] back to back."""
    text = b"".join(body for _name, body, _relocs in functions)
    names = [name for name, _b, _r in functions] + [
        name for name in externals if name not in {f[0] for f in functions}]
    for _name, _body, relocs in functions:
        for _site, target, _typ in relocs:
            if target not in names:
                names.append(target)
    index = {name: i + 1 for i, name in enumerate(names)}   # 0 = section symbol
    relocs, offset, values = [], 0, {}
    for name, body, rows in functions:
        values[name] = offset
        relocs += [(offset + site, index[target], typ) for site, target, typ in rows]
        offset += len(body)
    header = 20 + 40
    raw = header
    reloc_at = raw + len(text)
    symtab = reloc_at + 10 * len(relocs)
    strings = bytearray()
    records = bytearray(b".text\0\0\0" + struct.pack("<IhHBB", 0, 1, 0, 3, 0))
    for name in names:
        encoded = name.encode()
        if len(encoded) <= 8:
            field = encoded.ljust(8, b"\0")
        else:
            field = struct.pack("<II", 0, 4 + len(strings))
            strings += encoded + b"\0"
        defined = name in values
        records += field + struct.pack("<IhHBB", values.get(name, 0),
                                       1 if defined else 0, 0x20,
                                       3 if name in statics else 2, 0)
    out = bytearray(struct.pack("<HHIIIHH", 0x14C, 1, 0, symtab, 1 + len(names), 0, 0))
    out += b".text\0\0\0" + struct.pack("<IIIIIIHHI", 0, 0, len(text), raw, reloc_at,
                                        0, len(relocs), 0, 0x60000020)
    out += text
    for site, symbol, typ in relocs:
        out += struct.pack("<IIH", site, symbol, typ)
    out += records + struct.pack("<I", 4 + len(strings)) + strings
    return bytes(out)


def coff_with_datum(body, site, datum, data_section):
    """.text holding `_f` (DIR32 at `site` to `datum`) plus one data section
    ('.data' or '.bss') defining `datum`."""
    text_raw = 20 + 80
    data_raw = text_raw + len(body)
    reloc_at = data_raw + (4 if data_section == ".data" else 0)
    symtab = reloc_at + 10
    strings = bytearray()

    def field(name):
        encoded = name.encode()
        if len(encoded) <= 8:
            return encoded.ljust(8, b"\0")
        nonlocal strings
        out = struct.pack("<II", 0, 4 + len(strings))
        strings += encoded + b"\0"
        return out
    records = (field("_f") + struct.pack("<IhHBB", 0, 1, 0x20, 2, 0) +
               field(datum) + struct.pack("<IhHBB", 0, 2, 0, 2, 0))
    flags = 0xC0000040 if data_section == ".data" else 0xC0000080
    out = bytearray(struct.pack("<HHIIIHH", 0x14C, 2, 0, symtab, 2, 0, 0))
    out += b".text\0\0\0" + struct.pack("<IIIIIIHHI", 0, 0, len(body), text_raw,
                                        reloc_at, 0, 1, 0, 0x60000020)
    out += data_section.encode().ljust(8, b"\0") + struct.pack(
        "<IIIIIIHHI", 0, 0, 4, data_raw if data_section == ".data" else 0, 0, 0, 0, 0, flags)
    out += body + (bytes(4) if data_section == ".data" else b"")
    out += struct.pack("<IIH", site, 1, DIR32)
    out += records + struct.pack("<I", 4 + len(strings)) + strings
    return bytes(out)


def mov_eax(value):
    return b"\xa1" + struct.pack("<I", value)


class VoteTests(unittest.TestCase):
    def voter(self, body, relocs, retail, sites, rva=0x1000):
        candidate = rp.CandidateObject(coff([("_f", body, relocs)]))
        return rp.function_votes(rp.Voter("u", "_f", rva, len(retail)), candidate,
                                 retail, sites, BASE)

    def test_identical_function_votes_every_stable_relocation(self):
        body = mov_eax(0) + mov_eax(4) + b"\xc3"
        retail = mov_eax(BASE + 0x5000) + mov_eax(BASE + 0x5004) + b"\xc3"
        votes, reason = self.voter(body, [(1, "?g_a@@3HA", DIR32), (6, "?g_a@@3HA", DIR32)],
                                   retail, [0x1001, 0x1006])
        self.assertEqual(reason, "")
        self.assertEqual({(v.symbol, v.owner, v.addend) for v in votes},
                         {("?g_a@@3HA", 0x5000, 0), ("?g_a@@3HA", 0x5000, 4)})

    def test_instruction_difference_withdraws_every_vote(self):
        body = mov_eax(0) + b"\x90\xc3"
        retail = mov_eax(BASE + 0x5000) + b"\x40\xc3"       # inc eax, not nop
        votes, reason = self.voter(body, [(1, "?g_a@@3HA", DIR32)], retail, [0x1001])
        self.assertEqual(votes, [])
        self.assertEqual(reason, "instruction: bytes")

    def test_different_relocation_sites_withdraw_votes(self):
        body = mov_eax(0) + b"\xc3"
        retail = mov_eax(BASE + 0x5000) + b"\xc3"
        votes, reason = self.voter(body, [(1, "?g_a@@3HA", DIR32)], retail, [])
        self.assertEqual((votes, reason), ([], "instruction: absolute relocation sites"))

    def test_false_literal_retail_site_keeps_the_voter(self):
        # Retail lists 0x1001 (mov eax, 0x4c4b40 == 5000000) as an address;
        # the candidate holds the same literal without a relocation.
        body = mov_eax(0x4C4B40) + mov_eax(0) + b"\xc3"
        retail = mov_eax(0x4C4B40) + mov_eax(BASE + 0x6000) + b"\xc3"
        votes, reason = self.voter(body, [(6, "?g_a@@3HA", DIR32)], retail,
                                   [0x1001, 0x1006])
        self.assertEqual((reason, [v.owner for v in votes]), ("", [0x6000]))
        # A different literal at that site is an instruction difference.
        votes, reason = self.voter(mov_eax(0x4C4B41) + mov_eax(0) + b"\xc3",
                                   [(6, "?g_a@@3HA", DIR32)], retail, [0x1001, 0x1006])
        self.assertEqual((votes, reason), ([], "instruction: absolute relocation sites"))

    def test_volatile_names_do_not_vote(self):
        body = mov_eax(0) + b"\xc3"
        retail = mov_eax(BASE + 0x5000) + b"\xc3"
        votes, reason = self.voter(body, [(1, "$SG1234", DIR32)], retail, [0x1001])
        self.assertEqual((votes, reason), ([], ""))


def vote(symbol, target, addend=0, function=0x1000, site=None, typ=DIR32):
    return rp.Vote(symbol, target - addend, target, addend, function,
                   site if site is not None else function + target % 0x100, typ, "u", "_f")


def decide(votes, claimed=None, bound=None):
    claimed = claimed or {}
    bound = bound or {}
    return rp.decide(votes, region_of=lambda rva: "text" if rva < 0x5000 else "data",
                     claimed_name_at=claimed.get, claimed_rva_of=bound.get)


class DecisionTests(unittest.TestCase):
    def verdicts(self, pairings):
        return {(p.symbol, p.owner): p.verdict for p in pairings}

    def test_unanimous_pairing_binds(self):
        pairings, aliases = decide([vote("?g_a@@3HA", 0x6000),
                                    vote("?g_a@@3HA", 0x6000, function=0x1100)])
        self.assertEqual(self.verdicts(pairings), {("?g_a@@3HA", 0x6000): "admitted"})
        self.assertEqual(aliases, [])

    def test_conflicting_votes_do_not_bind(self):
        # One symbol at two addresses, and one address for two symbols.
        pairings, _ = decide([vote("?g_width@@3HA", 0x6000), vote("?g_width@@3HA", 0x6004),
                              vote("?g_height@@3HA", 0x6004)])
        self.assertEqual(set(self.verdicts(pairings).values()), {"held"})

    def test_claimed_address_or_symbol_does_not_bind(self):
        pairings, _ = decide([vote("?g_a@@3HA", 0x6000)], claimed={0x6000: "?g_b@@3HA"})
        self.assertEqual(pairings[0].verdict, "held")
        pairings, _ = decide([vote("?g_a@@3HA", 0x6000)], bound={"?g_a@@3HA": 0x7000})
        self.assertEqual(pairings[0].verdict, "held")
        pairings, _ = decide([vote("?g_a@@3HA", 0x6000)], claimed={0x6000: "?g_a@@3HA"})
        self.assertEqual(pairings[0].verdict, "confirmed")

    def test_consistent_addends_bind_and_emit_interior_aliases(self):
        pairings, aliases = decide([vote("?g_t@@3PAHA", 0x6000),
                                    vote("?g_t@@3PAHA", 0x6008, addend=8)])
        self.assertEqual(self.verdicts(pairings), {("?g_t@@3PAHA", 0x6000): "admitted"})
        self.assertEqual([(a.target, a.addend) for a in aliases], [(0x6008, 8)])

    def test_addend_mismatch_is_not_bound(self):
        # g_t+8 read at 0x6004 puts the owner at 0x5ffc, not the anchored 0x6000.
        pairings, aliases = decide([vote("?g_t@@3PAHA", 0x6000),
                                    vote("?g_t@@3PAHA", 0x6004, addend=8)])
        self.assertEqual(set(self.verdicts(pairings).values()), {"held"})
        self.assertEqual(aliases, [])

    def test_single_interior_addend_is_unanchored(self):
        pairings, _ = decide([vote("?g_t@@3PAHA", 0x6008, addend=8)])
        self.assertEqual((pairings[0].verdict, pairings[0].reason),
                         ("held", "unanchored addend"))

    def test_content_named_copies_pair_per_unit(self):
        # Two retail copies of one literal: each compiland's votes agree on
        # its own copy, so the pairing is scoped, not held or admitted.
        votes = [rp.Vote("??_C@_03x@", 0x7000, 0x7000, 0, 0x1000, 0x1001, DIR32, "a", "_f"),
                 rp.Vote("??_C@_03x@", 0x7010, 0x7010, 0, 0x1100, 0x1101, DIR32, "b", "_g")]
        pairings, _ = decide(votes)
        self.assertEqual({(p.unit, p.owner, p.verdict) for p in pairings},
                         {("a", 0x7000, "unit-candidate"), ("b", 0x7010, "unit-candidate")})
        votes.append(rp.Vote("??_C@_03x@", 0x7010, 0x7010, 0, 0x1200, 0x1201, DIR32, "a", "_h"))
        pairings, _ = decide(votes)
        self.assertEqual({(p.unit, p.verdict) for p in pairings},
                         {("a", "held"), ("b", "unit-candidate")})

    def test_writable_data_at_one_address_stays_a_conflict(self):
        # Only content-named read-only data may share an address.
        pairings, _ = decide([vote("?g_smallFont@@3PAVfont@@A", 0x6000),
                              vote("?g_tinyFont@@3PAVfont@@A", 0x6000)])
        self.assertEqual({p.verdict for p in pairings}, {"held"})

    def test_code_pairings_need_a_body_proof(self):
        pairings, _ = decide([vote("?size@a@@QBEIXZ", 0x2000, typ=REL32)],
                             claimed={0x2000: "?size@b@@QBEIXZ"})
        self.assertEqual(pairings[0].verdict, "candidate")
        rp.prove_folds(pairings, name_at={0x2000: "?size@b@@QBEIXZ"}.get,
                       prove=lambda symbol, rva: "")
        self.assertEqual(pairings[0].verdict, "folded")
        pairings, _ = decide([vote("?raise@a@@QAEXXZ", 0x2000, typ=REL32)],
                             claimed={0x2000: "?test@a@@QAEXXZ"})
        rp.prove_folds(pairings, name_at={0x2000: "?test@a@@QAEXXZ"}.get,
                       prove=lambda symbol, rva: "body size differs")
        self.assertEqual((pairings[0].verdict, pairings[0].reason),
                         ("held", "body size differs"))


def private_vote(symbol, target, unit, addend=0, function=0x1000, typ=DIR32):
    return rp.Vote(symbol, target - addend, target, addend, function,
                   function + target % 0x100, typ, unit, "_f", True, symbol)


class CompilandPrivateTests(unittest.TestCase):
    def test_same_named_statics_pair_per_compiland(self):
        # redHue: two compilands' local statics, one retail copy each.
        name = "_?redHue@?1??rgbToHSV@@YIXIIIPAM00@Z@4MB"
        pairings, _ = decide([private_vote(name, 0x6000, "bitmap24"),
                              private_vote(name, 0x6100, "palette", function=0x1100)])
        self.assertEqual({(p.unit, p.owner, p.verdict) for p in pairings},
                         {("bitmap24", 0x6000, "unit"), ("palette", 0x6100, "unit")})

    def test_a_unit_with_two_places_or_a_shared_address_holds(self):
        name = "_g_leftRightSave"
        pairings, _ = decide([private_vote(name, 0x6000, "button"),
                              private_vote(name, 0x6100, "button", function=0x1100)])
        self.assertEqual({p.verdict for p in pairings}, {"held"})
        pairings, _ = decide([private_vote(name, 0x6000, "button"),
                              private_vote(name, 0x6000, "slider", function=0x1100)])
        self.assertEqual({p.verdict for p in pairings}, {"held"})
        pairings, _ = decide([private_vote(name, 0x6000, "button"),
                              vote("_g_other", 0x6000, function=0x1100)])
        self.assertEqual({p.verdict for p in pairings}, {"held"})

    def test_claimed_static_confirms_by_its_masked_name(self):
        pairings, _ = decide([private_vote("_g_lastImHoverId", 0x6000, "mainmenu"),
                              private_vote("_g_lastImHoverId", 0x6100, "puzzlewindow",
                                           function=0x1100)],
                             claimed={0x6000: "_g_lastImHoverId$RVA6000",
                                      0x6100: "_g_lastImHoverId$RVA6100"})
        self.assertEqual({p.verdict for p in pairings}, {"confirmed"})
        pairings, _ = decide([private_vote("_g_lastImHoverId", 0x6000, "mainmenu")],
                             claimed={0x6000: "_g_other"})
        self.assertEqual((pairings[0].verdict, pairings[0].reason),
                         ("held", "address claimed as _g_other"))

    def test_private_code_needs_its_own_units_body(self):
        pairings, _ = decide([private_vote("??1TAutoStrPtr@?A0x1@@QAE@XZ", 0x2000,
                                           "spelldefs", typ=REL32)],
                             claimed={0x2000: "??1TAutoStrPtr@?A0x2@@QAE@XZ"})
        self.assertEqual((pairings[0].unit, pairings[0].verdict, pairings[0].raw),
                         ("spelldefs", "unit-candidate", "??1TAutoStrPtr@?A0x1@@QAE@XZ"))

    def test_candidate_object_lists_static_definitions(self):
        body = mov_eax(0) + b"\xc3"
        obj = rp.CandidateObject(coff([("_f", body, [(1, "_g_x", DIR32)])]))
        self.assertEqual(obj.private, set())
        obj = rp.CandidateObject(coff([("_f", body, [(1, "_s", DIR32)]),
                                       ("_s", b"\xc3", [])], statics=("_s",)))
        self.assertEqual(obj.private, {"_s"})
        self.assertTrue(rp.compiland_private("?g@?%C:\\src\\a.cpp12@@3HA", obj))
        self.assertFalse(rp.compiland_private("_g_x", obj))


class OutsideOperandTests(unittest.TestCase):
    def aliases(self, votes, extent, claimed="_g_t"):
        pairings, aliases = rp.decide(
            votes, region_of=lambda rva: "data", claimed_name_at={0x6000: claimed}.get,
            claimed_rva_of=lambda name: None, extent_of={0x6000: extent}.get)
        return pairings, sorted((a.target, a.addend) for a in aliases)

    def test_one_past_the_end_names_the_claimed_array(self):
        # int g_t[4] at 0x6000: a loop bound g_t + 0x10 lands on the next
        # object; the candidate names g_t, so the site keeps g_t + 0x10.
        pairings, aliases = self.aliases(
            [vote("_g_t", 0x6000), vote("_g_t", 0x6010, addend=0x10)], (0x10, 4))
        self.assertEqual(pairings[0].verdict, "confirmed")
        self.assertEqual(aliases, [(0x6010, 0x10)])

    def test_field_of_the_element_past_the_end(self):
        # SWinSetup[37] (8 bytes each): &g_t[37].field at +0x12a.
        _p, aliases = self.aliases([vote("_g_t", 0x612a, addend=0x12a)], (0x128, 8))
        self.assertEqual(aliases, [(0x612a, 0x12a)])

    def test_one_element_before_the_start(self):
        _p, aliases = self.aliases([vote("_g_t", 0x5ff8, addend=-8),
                                    vote("_g_t", 0x5ffc, addend=-4)], (0x200, 8))
        self.assertEqual(aliases, [(0x5ff8, -8), (0x5ffc, -4)])

    def test_negative_addend_is_written_as_vostok_u32(self):
        (row,) = rp.alias_rows(
            [a for a in rp.decide([vote("_g_t", 0x5ffc, addend=-4)],
                                  region_of=lambda rva: "data",
                                  claimed_name_at={0x6000: "_g_t"}.get,
                                  claimed_rva_of=lambda name: None,
                                  extent_of={0x6000: (0x10, 4)}.get)[1]], [])
        self.assertEqual(row[3:5], ["_g_t", "0xfffffffc"])

    def test_interior_and_distant_operands_are_left_alone(self):
        _p, aliases = self.aliases([vote("_g_t", 0x6004, addend=4),
                                    vote("_g_t", 0x6018, addend=0x18),
                                    vote("_g_t", 0x5ff0, addend=-0x10)], (0x10, 4))
        self.assertEqual(aliases, [])

    def test_non_array_allows_exactly_one_past_the_end(self):
        _p, aliases = self.aliases([vote("_g_t", 0x6008, addend=8),
                                    vote("_g_t", 0x6009, addend=9),
                                    vote("_g_t", 0x5ffc, addend=-4)], (8, 8))
        self.assertEqual(aliases, [(0x6008, 8)])

    def test_unconfirmed_or_unsized_claims_emit_nothing(self):
        pairings, aliases = self.aliases([vote("_g_t", 0x6010, addend=0x10)], (0x10, 4),
                                         claimed="_g_other")
        self.assertEqual((pairings[0].verdict, aliases), ("held", []))
        _p, aliases = self.aliases([vote("_g_t", 0x6010, addend=0x10)], None)
        self.assertEqual(aliases, [])

    def test_claim_extent_reads_the_outer_array_element(self):
        self.assertEqual(rp.claim_extent(0x60, "int[12][2]"), (0x60, 8))
        self.assertEqual(rp.claim_extent(0x128, "SWinSetup[37]"), (0x128, 8))
        self.assertEqual(rp.claim_extent(0xc, "const char *[3]"), (0xc, 4))
        self.assertEqual(rp.claim_extent(8, "TPoint"), (8, 8))
        self.assertEqual(rp.claim_extent(4, "char (*)[4]"), (4, 4))
        self.assertIsNone(rp.claim_extent(None, "int[4]"))


class FoldEvidenceTests(unittest.TestCase):
    def pairing(self):
        pairings, _ = decide([vote("?size@a@@QBEIXZ", 0x2000, typ=REL32)],
                             claimed={0x2000: "?size@b@@QBEIXZ"})
        return pairings

    def test_identical_retail_twin_holds_a_fold(self):
        # Retail keeps an unfolded copy with the same bytes at 0x3000: the
        # candidate symbol could be either copy, so the vote does not decide.
        pairings = self.pairing()
        rp.prove_folds(pairings, name_at={0x2000: "?size@b@@QBEIXZ"}.get,
                       prove=lambda symbol, rva: "", twins=lambda rva: [0x3000])
        self.assertEqual(pairings[0].verdict, "held")
        self.assertIn("twin", pairings[0].reason)

    def twin_verdict(self, votes, claimed, twins=(0x3000,)):
        pairings, _ = decide(votes, claimed=claimed)
        rp.prove_folds(pairings, name_at=claimed.get, prove=lambda symbol, rva: "",
                       twins=lambda rva: [t for t in (0x2000, *twins) if t != rva])
        return {(p.symbol, p.owner): (p.verdict, p.reason) for p in pairings}

    def test_confirmed_twin_without_a_conflicting_vote_admits_the_fold(self):
        # _Ufill<widget*> votes reach only 0x2000 (_Ufill<int>); the twin at
        # 0x3000 is claimed as another instantiation that no widget vote
        # reaches, so the symbol's address is unambiguous.
        verdicts = self.twin_verdict(
            [vote("?_Ufill@w@@", 0x2000, typ=REL32),
             vote("?_Ufill@v@@", 0x3000, typ=REL32, function=0x1100)],
            {0x2000: "?_Ufill@i@@", 0x3000: "?_Ufill@v@@"})
        verdict, reason = verdicts[("?_Ufill@w@@", 0x2000)]
        self.assertEqual(verdict, "folded")
        self.assertIn("twin 0x3000 is ?_Ufill@v@@", reason)

    def test_unanimous_vote_confirms_an_unclaimed_twin(self):
        verdicts = self.twin_verdict(
            [vote("?_Ufill@w@@", 0x2000, typ=REL32),
             vote("?_Ufill@v@@", 0x3000, typ=REL32, function=0x1100)],
            {0x2000: "?_Ufill@i@@"})
        self.assertEqual(verdicts[("?_Ufill@w@@", 0x2000)][0], "folded")

    def test_generated_binding_confirms_a_twin(self):
        pairings = self.pairing()
        rp.prove_folds(pairings, name_at={0x2000: "?size@b@@QBEIXZ"}.get,
                       prove=lambda symbol, rva: "", twins=lambda rva: [0x3000],
                       bound_at={0x3000: {"_strlen"}}.get)
        self.assertEqual(pairings[0].verdict, "folded")

    def test_unconfirmed_twin_holds_the_fold(self):
        verdicts = self.twin_verdict([vote("?_Ufill@w@@", 0x2000, typ=REL32)],
                                     {0x2000: "?_Ufill@i@@"})
        self.assertEqual(verdicts[("?_Ufill@w@@", 0x2000)],
                         ("held", "identical retail twin at 0x3000 is unidentified"))
        # A twin whose votes disagree is not confirmed either.
        verdicts = self.twin_verdict(
            [vote("?_Ufill@w@@", 0x2000, typ=REL32),
             vote("?_Ufill@v@@", 0x3000, typ=REL32, function=0x1100),
             vote("?_Ufill@u@@", 0x3000, typ=REL32, function=0x1200)],
            {0x2000: "?_Ufill@i@@"})
        self.assertEqual(verdicts[("?_Ufill@w@@", 0x2000)][0], "held")

    def test_conflicting_vote_at_the_twin_holds_the_fold(self):
        # One widget call reaches the twin: both places are held.
        verdicts = self.twin_verdict(
            [vote("?_Ufill@w@@", 0x2000, typ=REL32),
             vote("?_Ufill@w@@", 0x3000, typ=REL32, function=0x1100)],
            {0x2000: "?_Ufill@i@@", 0x3000: "?_Ufill@v@@"})
        self.assertEqual({v for v, _r in verdicts.values()}, {"held"})
        # A unit-scoped candidate reached at the twin by its own unit's vote.
        pairings = [rp.Pairing("_$E4", 0x2000, "code", [], "unit-candidate", "", "a"),
                    rp.Pairing("_$E4", 0x3000, "code", [], "held", "", "a")]
        rp.prove_folds(pairings, name_at={0x3000: "?g@@"}.get,
                       prove=lambda symbol, rva: "", twins=lambda rva: [0x3000])
        self.assertEqual(pairings[0].verdict, "held")
        # A twin bound to the very symbol is a conflict, not a confirmation.
        pairings, _ = decide([vote("?_Ufill@w@@", 0x2000, typ=REL32)],
                             claimed={0x2000: "?_Ufill@i@@"})
        rp.prove_folds(pairings, name_at={0x2000: "?_Ufill@i@@"}.get,
                       prove=lambda symbol, rva: "", twins=lambda rva: [0x3000],
                       bound_at={0x3000: {"?_Ufill@w@@"}}.get)
        self.assertEqual((pairings[0].verdict, pairings[0].reason),
                         ("held", "twin 0x3000 is bound to the symbol"))

    def test_symbol_claimed_elsewhere_is_not_a_fold(self):
        pairings, _ = decide([vote("?size@a@@QBEIXZ", 0x2000, typ=REL32)],
                             claimed={0x2000: "?size@b@@QBEIXZ"},
                             bound={"?size@a@@QBEIXZ": 0x3000})
        self.assertEqual((pairings[0].verdict, pairings[0].reason),
                         ("held", "symbol claimed at 0x3000"))

    def test_local_guard_pairs_per_unit_as_data(self):
        guard = "_?$S27@?1??init@@YIXXZ@4EA"
        votes = [rp.Vote(guard, 0x6000, 0x6000, 0, 0x1000, 0x1001, DIR32, "a", "_f"),
                 rp.Vote(guard, 0x6100, 0x6100, 0, 0x1100, 0x1101, DIR32, "b", "_g")]
        pairings, _ = decide(votes)
        self.assertEqual({(p.unit, p.kind, p.verdict) for p in pairings},
                         {("a", "data", "unit-candidate"), ("b", "data", "unit-candidate")})

    def test_local_function_pairs_per_unit(self):
        votes = [rp.Vote("_$E47", 0x2000, 0x2000, 0, 0x1000, 0x1001, DIR32, "a", "_f"),
                 rp.Vote("_$E47", 0x2100, 0x2100, 0, 0x1100, 0x1101, DIR32, "b", "_g")]
        pairings, _ = decide(votes)
        self.assertEqual({(p.unit, p.owner, p.verdict) for p in pairings},
                         {("a", 0x2000, "unit-candidate"), ("b", 0x2100, "unit-candidate")})


class IdentityRelocationTests(unittest.TestCase):
    def rewrite(self, base_target, target_target, identities, symbol_rvas,
                base_addend=0, target_addend=0):
        from homm3.build import identity_relocations
        from homm3.compare.canonicalize import CoffObject
        body = lambda addend: b"\xe8" + struct.pack("<i", addend) + b"\xc3"
        base = coff([("_f", body(base_addend), [(1, base_target, REL32)])])
        target = coff([("_f", body(target_addend), [(1, target_target, REL32)])])
        out, count = identity_relocations.canonicalize(base, target, symbol_rvas, identities)
        parsed = CoffObject(out)
        name = parsed.symbols[parsed.relocations[0].symbol_index].name
        return count, name

    def test_folded_name_compares_at_the_same_address(self):
        count, name = self.rewrite("?size@a@@QBEIXZ", "?size@b@@QBEIXZ",
                                   {("", "?size@a@@QBEIXZ"): {0x2000}},
                                   {"?size@b@@QBEIXZ": (0x2000, "func")})
        self.assertEqual((count, name), (1, "?size@a@@QBEIXZ"))

    def test_different_address_stays_visible(self):
        count, name = self.rewrite("?size@a@@QBEIXZ", "?size@b@@QBEIXZ",
                                   {("", "?size@a@@QBEIXZ"): {0x3000}},
                                   {"?size@b@@QBEIXZ": (0x2000, "func")})
        self.assertEqual((count, name), (0, "?size@b@@QBEIXZ"))

    def test_unproven_or_ambiguous_name_stays_visible(self):
        count, _ = self.rewrite("?size@a@@QBEIXZ", "?size@b@@QBEIXZ", {},
                                {"?size@b@@QBEIXZ": (0x2000, "func")})
        self.assertEqual(count, 0)
        count, _ = self.rewrite("?size@a@@QBEIXZ", "?size@b@@QBEIXZ",
                                {("", "?size@a@@QBEIXZ"): {0x2000, 0x3000}},
                                {"?size@b@@QBEIXZ": (0x2000, "func")})
        self.assertEqual(count, 0)

    def test_library_datum_compares_by_its_external_name(self):
        from homm3.build import identity_relocations
        from homm3.compare.canonicalize import CoffObject
        name = "?id@?$numpunct@D@std@@2V0locale@2@A"
        body = mov_eax(0) + b"\xc3"
        base = coff_with_datum(body, 1, name, ".bss")
        target = coff_with_datum(body, 1, name, ".data")
        identities = {("", name): {0x2ab1d4}}
        out, count = identity_relocations.canonicalize(
            base, target, {}, identities, library_names=frozenset({name}))
        parsed = CoffObject(out)
        symbol = parsed.symbols[parsed.relocations[0].symbol_index]
        self.assertEqual((count, symbol.name, symbol.section), (1, name, 0))
        # Without library proof the local placements stay compared as they are.
        out, count = identity_relocations.canonicalize(base, target, {}, identities)
        self.assertEqual((out, count), (target, 0))

    def test_placeholder_target_resolves_by_its_address(self):
        count, name = self.rewrite("_memmove", "fn_217590", {("", "_memmove"): {0x217590}}, {})
        self.assertEqual((count, name), (1, "_memmove"))

    def test_unit_scoped_copy_decides_for_its_unit_only(self):
        from homm3.build import identity_relocations
        identities = {("", "??_C@_03x@"): {0x7000},
                      ("u", "??_C@_03x@"): {0x7010}}
        self.assertEqual(identity_relocations.resolve_name(
            "??_C@_03x@", {}, identities, "u"), 0x7010)
        self.assertEqual(identity_relocations.resolve_name(
            "??_C@_03x@", {}, identities, "v"), 0x7000)


if __name__ == "__main__":
    unittest.main()
