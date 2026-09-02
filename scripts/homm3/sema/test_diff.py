"""Tests for the pure comparison views of `homm3 sema diff`: the reference
sequences (--calls/--relocs), the first byte-level divergence and the
summary digest (--summary/--why-bytes)."""
from __future__ import annotations

import unittest

from homm3.sema import _asm, diff


def _listing(*rows, start: int = 0, name: str = "fn") -> str:
    """objdump-shaped text from (asm, bytes-hex, [(field, kind, symbol)]) rows;
    `field` is the reloc site's offset inside the instruction."""
    lines = [f"{start:08x} <{name}>:"]
    offset = start
    for asm, hexbytes, *relocs in rows:
        raw = bytes.fromhex(hexbytes.replace(" ", ""))
        mnemonic, _sp, operands = asm.partition(" ")
        lines.append(f"{offset:8x}: {hexbytes}\t{mnemonic}\t{operands}")
        for field, kind, symbol in (relocs[0] if relocs else []):
            lines.append(f"\t\t\t{offset + field:08x}:  IMAGE_REL_I386_{kind}\t{symbol}")
        offset += len(raw)
    return "\n".join(lines) + "\n"


CALL = ("call 0x5", "e8 00 00 00 00")
RET = ("ret", "c3")


def _calls(*symbols, start=0):
    rows = [(f"call 0x{5:x}", "e8 00 00 00 00", [(1, "REL32", s)]) for s in symbols]
    return _listing(*rows, RET, start=start)


class RefSeqTest(unittest.TestCase):
    def test_offsets_rebase_and_kinds(self):
        text = _listing(
            ("call 0x5", "e8 00 00 00 00", [(1, "REL32", "?f@@YAXXZ")]),
            ("mov eax, dword ptr [0x0]", "a1 00 00 00 00", [(1, "DIR32", "gVar")]),
            ("call dword ptr [edx + 0xc]", "ff 52 0c"),
            ("jmp 0x12", "e9 00 00 00 00", [(1, "REL32", "?tail@@YAXXZ")]),
            RET, start=0x16d0)
        refs, stop = diff._ref_seq(text)
        self.assertIsNone(stop)
        self.assertEqual([(r[0], r[1], r[2], r[3]) for r in refs], [
            (0x0, "call", "?f@@YAXXZ", 0),
            (0x5, "data", "gVar", 0),
            (0xa, "ind", "<indirect>[+0xc]", 0),
            (0xd, "jmp", "?tail@@YAXXZ", 0)])
        self.assertEqual(refs[2][4], "call dword ptr [edx + 0xc]")

    def test_pool_words_are_not_references(self):
        text = _listing(
            ("call 0x5", "e8 00 00 00 00", [(1, "REL32", "?f@@YAXXZ")]),
            RET,
            ("add byte ptr [eax], al", "00 00", [(0, "DIR32", "$L1")]),
            ("add byte ptr [eax], al", "00 00"))
        refs, _stop = diff._ref_seq(text)
        self.assertEqual([r[2] for r in refs], ["?f@@YAXXZ"])

    def test_addend_is_the_relocated_field(self):
        text = _listing(("mov eax, dword ptr [0x18]", "a1 18 00 00 00",
                         [(1, "DIR32", "_z_errmsg")]), RET)
        refs, _stop = diff._ref_seq(text)
        self.assertEqual(refs[0][2:4], ("_z_errmsg", 0x18))


class RefsCompareTest(unittest.TestCase):
    def rows(self, base, target):
        res = diff._refs_compare(diff._ref_seq(base)[0], diff._ref_seq(target)[0])
        return [(tag, b[2] if b else None, t[2] if t else None, note)
                for tag, b, t, note in res["rows"]], res

    def test_identical_sequences_agree(self):
        rows, res = self.rows(_calls("?a@@YAXXZ", "?b@@YAXXZ"), _calls("?a@@YAXXZ", "?b@@YAXXZ"))
        self.assertEqual([r[0] for r in rows], ["=", "="])
        self.assertTrue(res["agree"] and res["report_agree"])

    def test_unclaimed_retail_label_differs_but_report_agrees(self):
        rows, res = self.rows(_calls("?f@@YAXXZ"), _calls("sub_f6570"))
        self.assertEqual(rows, [("~", "?f@@YAXXZ", "sub_f6570", "unclaimed")])
        self.assertFalse(res["agree"])
        self.assertTrue(res["report_agree"])
        self.assertEqual((res["counts"]["synthetic"], res["counts"]["real"]), (1, 0))

    def test_two_real_names_differ_without_annotation(self):
        rows, _res = self.rows(_calls("?HeroWindowHandler@heroWindow@@SIHAAVmessage@@@Z"),
                               _calls("?SetFocus@heroWindow@@QAEXH@Z"))
        self.assertEqual(rows[0][0], "~")
        self.assertIsNone(rows[0][3])

    def test_one_inlined_call_does_not_shift_the_pairing(self):
        base = _calls("?A@@YAXXZ", "?size@@QBEIXZ", "??2@YAPAXI@Z", "?_Ucopy@@Z", "??3@YAXPAX@Z")
        target = _calls("?A@@YAXXZ", "exe_new", "game_1510_sub13", "??3@YAXPAX@Z")
        rows, res = self.rows(base, target)
        # the pairing inside the run of unclaimed labels is positional; what
        # must hold is ONE base-only row and no spurious extra difference
        self.assertEqual(sorted(r[0] for r in rows), ["-", "=", "=", "~", "~"])
        self.assertEqual((rows[0], rows[-1][0]), (("=", "?A@@YAXXZ", "?A@@YAXXZ", None), "="))
        self.assertEqual(res["counts"]["-"], 1)
        self.assertFalse(res["report_agree"])

    def test_addend_and_kind_count_as_differences(self):
        base = _listing(("mov eax, dword ptr [0x18]", "a1 18 00 00 00", [(1, "DIR32", "_z_errmsg")]), RET)
        target = _listing(("mov eax, dword ptr [0x0]", "a1 00 00 00 00", [(1, "DIR32", "_z_errmsg")]), RET)
        rows, _res = self.rows(base, target)
        self.assertEqual(rows[0][0], "~")
        base = _listing(("call 0x5", "e8 00 00 00 00", [(1, "REL32", "?f@@YAXXZ")]), RET)
        target = _listing(("jmp 0x5", "e9 00 00 00 00", [(1, "REL32", "?f@@YAXXZ")]), RET)
        rows, _res = self.rows(base, target)
        self.assertEqual(rows[0][0], "~")

    def test_indirect_calls_compare_by_displacement(self):
        base = _listing(("call dword ptr [edx + 0xc]", "ff 52 0c"), RET)
        same = _listing(("call dword ptr [eax + 0xc]", "ff 50 0c"), RET)
        other = _listing(("call dword ptr [eax + 0x10]", "ff 50 10"), RET)
        self.assertEqual(self.rows(base, same)[0][0][0], "=")
        self.assertEqual(self.rows(base, other)[0][0][0], "~")

    def test_rendered_view_lines(self):
        base = _calls("?A@@YAXXZ", "?B@@YAXXZ")
        target = _calls("?A@@YAXXZ", "sub_1234")
        text, agree = diff._refs_view(base, target, 0x1000, "fn", calls_only=True)
        self.assertFalse(agree)
        self.assertIn("[call diff: BASE (compiled) vs TARGET (retail) @ 0x00001000 fn]", text)
        self.assertIn("  base 2 call(s) (2 direct, 0 indirect)   |   target 2 call(s)", text)
        self.assertIn("  #0   +000   =  ?A@@YAXXZ", text)
        self.assertIn("~  ?B@@YAXXZ -> sub_1234  (retail label - unclaimed)", text)
        self.assertIn("1 same, 1 different (1 unclaimed retail labels, 0 real), 0 base-only, 0 target-only", text)
        self.assertIn("name_address: CALL SEQUENCES DIFFER   |   report (none): AGREE", text)
        text, agree = diff._refs_view(_listing(RET), _listing(RET), 0x1000, "fn", calls_only=False)
        self.assertTrue(agree)
        self.assertIn("no relocations on either side.", text)


class FirstDivergenceTest(unittest.TestCase):
    def kind(self, base, target):
        div = diff._first_divergence(base, target)
        return None if div is None else (div["kind"], div["cosmetic"], div.get("note"))

    def test_identical_is_none(self):
        text = _listing(("push 0x4008", "68 08 40 00 00"), RET)
        self.assertIsNone(self.kind(text, text))

    def test_kinds(self):
        push8 = ("push 0x4008", "68 08 40 00 00")
        push9 = ("push 0x4009", "68 09 40 00 00")
        self.assertEqual(self.kind(_listing(push8, RET), _listing(push9, RET)),
                         ("immediate", False, None))
        self.assertEqual(self.kind(_listing(("xor eax, eax", "33 c0"), RET),
                                   _listing(("sub eax, eax", "2b c0"), RET)),
                         ("opcode", False, None))
        self.assertEqual(self.kind(_listing(("mov ecx, esi", "8b ce"), RET),
                                   _listing(("mov ecx, edi", "8b cf"), RET)),
                         ("register", False, None))
        self.assertEqual(self.kind(_listing(push8, push8, RET), _listing(push8, RET)),
                         ("missing", False, None))
        a1 = ("mov eax, dword ptr [0x0]", "a1 00 00 00 00", [(1, "DIR32", "g")])
        modrm = ("mov eax, dword ptr [0x0]", "8b 05 00 00 00 00", [(2, "DIR32", "g")])
        self.assertEqual(self.kind(_listing(a1, RET), _listing(modrm, RET)),
                         ("encoding", False, None))

    def test_flow_divergence(self):
        # identical block structure; only the je lands on a different block
        def listing(je_disp):
            return _listing(("cmp eax, 0x1", "83 f8 01"), (f"je 0x{5 + je_disp:x}", f"74 {je_disp:02x}"),
                            ("xor eax, eax", "33 c0"), ("jmp 0xa", "eb 01"),
                            ("inc eax", "40"), ("ret", "c3"))
        div = diff._first_divergence(listing(5), listing(4))
        self.assertEqual(div["kind"], "flow")
        self.assertFalse(div["cosmetic"])
        self.assertIn("jcc B3", div["note"])
        self.assertIn("jcc B2", div["note"])

    def test_shifted_displacement_alone_is_deferred(self):
        # same target block, the displacement differs only because an
        # instruction in between changed size: the size change is the cause
        base = _listing(("je 0x7", "74 05"), ("push 0x40", "6a 40"),
                        ("mov ecx, esi", "8b ce"), ("nop", "90"), ("ret", "c3"))
        target = _listing(("je 0xa", "74 08"), ("push 0x4000", "68 00 40 00 00"),
                          ("mov ecx, esi", "8b ce"), ("nop", "90"), ("ret", "c3"))
        div = diff._first_divergence(base, target)
        self.assertEqual(div["kind"], "immediate")

    def test_reloc_target_is_cosmetic_only_for_synthetic_names(self):
        real = _listing(("call 0x5", "e8 00 00 00 00", [(1, "REL32", "?f@@YAXXZ")]), RET)
        label = _listing(("call 0x5", "e8 00 00 00 00", [(1, "REL32", "sub_f6570")]), RET)
        other = _listing(("call 0x5", "e8 00 00 00 00", [(1, "REL32", "?g@@YAXXZ")]), RET)
        self.assertEqual(self.kind(real, label), ("reloc-target", True, "unclaimed"))
        self.assertEqual(self.kind(real, other), ("reloc-target", False, None))
        # a real divergence after a cosmetic one wins
        base = _listing(("call 0x5", "e8 00 00 00 00", [(1, "REL32", "?f@@YAXXZ")]),
                        ("push 0x4008", "68 08 40 00 00"), RET)
        target = _listing(("call 0x5", "e8 00 00 00 00", [(1, "REL32", "sub_f6570")]),
                          ("push 0x4009", "68 09 40 00 00"), RET)
        self.assertEqual(self.kind(base, target), ("immediate", False, None))

    def test_render_shows_both_sides_unmasked(self):
        base = _listing(("mov ecx, esi", "8b ce"), ("push 0x4008", "68 08 40 00 00"), RET)
        target = _listing(("mov ecx, esi", "8b ce"), ("push 0x4009", "68 09 40 00 00"), RET)
        lines = diff._render_divergence(diff._first_divergence(base, target))
        self.assertIn("kind IMMEDIATE - a different literal", lines[0])
        self.assertIn("  base    +002: 68 08 40 00 00     push 0x4008", lines)
        self.assertIn("  target  +002: 68 09 40 00 00     push 0x4009", lines)


class SummaryLinesTest(unittest.TestCase):
    def facts(self, **over):
        agree = {"rows": [], "counts": {"=": 1, "~": 0, "-": 0, "+": 0, "synthetic": 0, "real": 0},
                 "agree": True, "report_agree": True}
        facts = {"rva": 0x1000, "name": "fn", "unit": "u", "pct": 100.0,
                 "census": {"blocks": (2, 2), "exact": 2, "size": 0, "shift": 0,
                            "flow": 0, "missing": 0, "rows": [], "first_flow": None,
                            "first_differs": None, "same": True},
                 "branches": {"status": "clean", "kind": None, "rows": [], "nbr": 1,
                              "nbr_t": 1, "rets": (1, 1)},
                 "calls": agree, "relocs": agree,
                 "asm": {"equal": True, "instructions": 0, "relocs": 0},
                 "divergence": None, "source": "(none - no divergent statement)",
                 "source_loaded": True, "why_bytes": False}
        facts.update(over)
        return facts

    def test_all_agree(self):
        lines, agree, nxt = diff._summary_lines(self.facts())
        self.assertTrue(agree)
        self.assertIsNone(nxt)
        self.assertIn("  next: (nothing - all views agree)", lines)
        self.assertIn("  objdiff       100.00%  (report; function_reloc_diffs=none)", lines)

    def test_next_rungs(self):
        differ = {"rows": [], "counts": {"=": 0, "~": 1, "-": 0, "+": 0, "synthetic": 1, "real": 0},
                  "agree": False, "report_agree": True}
        self.assertEqual(diff._summary_lines(self.facts(calls=differ, relocs=differ))[2], "--calls")
        self.assertEqual(diff._summary_lines(self.facts(
            branches={"status": "flips", "kind": "SIGNEDNESS", "rows": [], "nbr": 1, "nbr_t": 1, "rets": (1, 1)}))[2],
            "--branches")
        census = dict(self.facts()["census"], same=False, exact=1, size=1)
        self.assertEqual(diff._summary_lines(self.facts(census=census))[2], "--source")
        self.assertEqual(diff._summary_lines(self.facts(census=census, source_loaded=False))[2],
                         "--structure --verbose")
        self.assertEqual(diff._summary_lines(self.facts(relocs=differ))[2], "--relocs")
        asm = {"equal": False, "instructions": 2, "relocs": 0}
        self.assertEqual(diff._summary_lines(self.facts(asm=asm))[2], "--why-bytes")
        self.assertEqual(diff._summary_lines(self.facts(asm=asm, why_bytes=True))[2], "--source")
        lines, agree, _n = diff._summary_lines(self.facts(pct=None))
        self.assertIn("  objdiff       n/a (no report entry)", lines)

    def test_source_diff_keeps_two_values(self):
        from homm3.sema import source
        mapping = source.SourceMap("src/u.cpp", (source.Statement(0, 3, "x;"),))
        text = _listing(("mov eax, ebx", "8b c3"), RET)
        self.assertEqual(len(diff._source_diff(text, text, mapping)), 2)
        self.assertEqual(len(diff._source_diff_full(text, text, mapping)), 3)


if __name__ == "__main__":
    unittest.main()
