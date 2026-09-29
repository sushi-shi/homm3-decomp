#!/usr/bin/env python3
"""Controls for relocation pairing: votes, verdicts, folds and the rewrite."""
from __future__ import annotations

import struct
import unittest

from homm3.delink import reloc_pairing as rp

DIR32, REL32 = 0x6, 0x14
BASE = 0x400000


def coff(functions, *, externals=()):
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
                                       1 if defined else 0, 0x20, 2, 0)
    out = bytearray(struct.pack("<HHIIIHH", 0x14C, 1, 0, symtab, 1 + len(names), 0, 0))
    out += b".text\0\0\0" + struct.pack("<IIIIIIHHI", 0, 0, len(text), raw, reloc_at,
                                        0, len(relocs), 0, 0x60000020)
    out += text
    for site, symbol, typ in relocs:
        out += struct.pack("<IIH", site, symbol, typ)
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
