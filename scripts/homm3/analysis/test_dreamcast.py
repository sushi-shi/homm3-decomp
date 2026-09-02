"""Hermetic tests for `homm3 dreamcast` selector and corpus logic."""
from __future__ import annotations

import contextlib
import io
import struct
import unittest

from homm3.analysis import dc_asm, dreamcast
from homm3.core import undname


def _fn(offset: str, name: str, module: str = "unit.obj", cb: str = "12"):
    return {
        "offset": offset, "cb": cb, "kind": "global", "name": name,
        "module": module, "file": r"E:\gamedcs\unit.cpp", "line": "10",
        "debug_start": "0", "debug_end": cb, "params": "0", "locals": "0",
    }


def _bridge(rva: str, offset: str, name: str, module: str = "unit.obj"):
    return {
        "rva": rva, "size": "20", "role": "anchor-global", "name": name,
        "signature": f"void {name}()", "dc_module": module,
        "dc_offset": offset, "dc_cb": "12", "source": r"E:\gamedcs\unit.cpp:10",
    }


class CorpusResolutionTest(unittest.TestCase):
    def setUp(self):
        self.functions = [
            _fn("0x100", "Widget::Open"),
            _fn("0x120", "Widget::Close"),
            _fn("0x140", "Other::Open", "other.obj"),
        ]
        self.corpus = dreamcast.Corpus(
            functions=self.functions, variables=[],
            bridges=[_bridge("0x2000", "0x100", "Widget::Open")],
            claims=[dreamcast.Claim(0x403000, "unit.obj", 0x120,
                                    "src/unit.cpp", 44)])

    def test_explicit_dc_and_module_offsets(self):
        self.assertEqual(self.corpus.resolve("dc:0x100")["name"], "Widget::Open")
        self.assertEqual(self.corpus.resolve("other.obj:0x140")["name"],
                         "Other::Open")

    def test_retail_rva_va_bridge_and_source_claim(self):
        self.assertEqual(self.corpus.resolve("0x2000")["name"], "Widget::Open")
        self.assertEqual(self.corpus.resolve("0x402000")["name"], "Widget::Open")
        self.assertEqual(self.corpus.resolve("0x403000")["name"], "Widget::Close")

    def test_exact_name_wins_and_substring_ambiguity_is_explicit(self):
        self.assertEqual(self.corpus.resolve("Widget::Open")["module"], "unit.obj")
        with self.assertRaisesRegex(dreamcast.DreamcastError, "ambiguous"):
            self.corpus.resolve("Open")

    def test_short_number_without_retail_bridge_reads_as_dc_offset_with_note(self):
        with contextlib.redirect_stderr(io.StringIO()) as err:
            row = self.corpus.resolve("0x100")
        self.assertEqual(row["name"], "Widget::Open")
        self.assertIn("resolved as the Dreamcast offset dc:0x100", err.getvalue())
        # a bridged retail RVA still wins over the same number as a DC offset
        self.assertEqual(self.corpus.resolve("0x2000")["name"], "Widget::Open")

    def test_dc_offsets_are_hex_with_or_without_prefix(self):
        self.assertEqual(self.corpus.resolve("dc:100")["name"], "Widget::Open")
        self.assertEqual(self.corpus.resolve("dc:120")["name"], "Widget::Close")
        self.assertEqual(self.corpus.resolve("DC:0X140")["name"], "Other::Open")
        self.assertEqual(self.corpus.resolve("other.obj:140")["name"], "Other::Open")
        with self.assertRaisesRegex(dreamcast.DreamcastError, "invalid DC offset"):
            self.corpus.resolve("dc:0x12zz")

    def test_body_offset_snaps_to_its_procedure_with_note(self):
        with contextlib.redirect_stderr(io.StringIO()) as err:
            self.assertEqual(self.corpus.resolve("dc:0x104")["name"], "Widget::Open")
            self.assertEqual(self.corpus.resolve("unit.obj:0x126")["name"],
                             "Widget::Close")
        self.assertIn("dc:0x104 is +0x4 into unit.obj:0x100 Widget::Open", err.getvalue())
        with self.assertRaises(dreamcast.NoMatch):
            self.corpus.resolve("dc:0x10c")  # cb=12: first byte past the body

    def test_nothing_matching_is_an_answer_not_an_error(self):
        self.assertTrue(issubclass(dreamcast.NoMatch, dreamcast.DreamcastError))
        with self.assertRaises(dreamcast.NoMatch):
            self.corpus.resolve("dc:0xdead")
        with self.assertRaises(dreamcast.NoMatch):
            self.corpus.resolve("NoSuchName")
        with self.assertRaises(dreamcast.NoMatch):
            self.corpus.resolve("nothing.obj:0x100")

    def test_exact_name_with_several_procedures_resolves_all(self):
        functions = self.functions + [_fn("0x160", "Widget::Open", "other.obj")]
        corpus = dreamcast.Corpus(functions=functions, variables=[], bridges=[],
                                  claims=[])
        self.assertEqual([r["module"] for r in corpus.resolve_all("Widget::Open")],
                         ["unit.obj", "other.obj"])
        with self.assertRaisesRegex(dreamcast.DreamcastError, "ambiguous"):
            corpus.resolve("Widget::Open")
        with self.assertRaisesRegex(dreamcast.DreamcastError, "ambiguous"):
            corpus.resolve_all("Open")  # a substring still has to be unique

    def test_retail_bridge_to_two_procedures_resolves_all(self):
        corpus = dreamcast.Corpus(
            functions=self.functions, variables=[],
            bridges=[_bridge("0x2000", "0x100", "Widget::Open"),
                     _bridge("0x2000", "0x120", "Widget::Close")], claims=[])
        self.assertEqual([r["name"] for r in corpus.resolve_all("0x402000")],
                         ["Widget::Open", "Widget::Close"])

    def test_pasted_declaration_loses_its_signature(self):
        self.assertEqual(self.corpus.resolve("Widget::Close() const")["name"],
                         "Widget::Close")

    @unittest.skipUnless(undname.available(), "llvm-undname not on PATH")
    def test_retail_address_without_bridge_matches_by_demangled_name(self):
        corpus = dreamcast.Corpus(
            functions=self.functions, variables=[], bridges=[], claims=[],
            retail_names={0x1500: "?Close@Widget@@QAEXXZ",
                          0x1600: "?Nowhere@Widget@@QAEXXZ"})
        with contextlib.redirect_stderr(io.StringIO()) as err:
            self.assertEqual(corpus.resolve("0x401500")["name"], "Widget::Close")
        self.assertIn("BY NAME", err.getvalue())
        with self.assertRaisesRegex(dreamcast.NoMatch, "Widget::Nowhere"):
            corpus.resolve("0x401600")

    @unittest.skipUnless(undname.available(), "llvm-undname not on PATH")
    def test_retail_mangled_name_is_a_selector(self):
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(self.corpus.resolve("?Open@Widget@@QAEXXZ")["name"],
                             "Widget::Open")

    def test_find_normalizes_module_suffix(self):
        rows = self.corpus.find("open", "unit")
        self.assertEqual([row["name"] for row in rows], ["Widget::Open"])


class ClassificationTest(unittest.TestCase):
    def test_lifetime_and_tiny_helper_classification(self):
        self.assertEqual(dreamcast._call_kind("Widget::Widget", 40), "constructor")
        self.assertEqual(dreamcast._call_kind("Widget::~Widget", 40), "destructor")
        self.assertEqual(dreamcast._call_kind("??0Widget@@QAA@XZ", 40),
                         "constructor")
        self.assertEqual(dreamcast._call_kind("??1Widget@@QAA@XZ", 40),
                         "destructor")
        self.assertEqual(dreamcast._call_kind(
            "std::basic_string<char,std::allocator<char> >::basic_string<char>",
            40), "constructor")
        self.assertEqual(dreamcast._call_kind("Widget::size", 4), "tiny-helper")
        self.assertEqual(dreamcast._call_kind("Widget::Draw", 80), "call")


class SourceLineGapTest(unittest.TestCase):
    def test_leading_gap_uses_procedure_frame_and_first_body_line(self):
        row = _fn("0x100", "Widget::Open", cb="32")
        rows = [
            (r"E:\gamedcs\unit.cpp", 20, 0x100),
            (r"E:\gamedcs\unit.cpp", 22, 0x108),
            (r"E:\gamedcs\unit.cpp", 27, 0x114),
        ]
        row["line"] = "20"
        shape = dreamcast._source_line_shape(row, rows)
        self.assertEqual(shape["first_body_line"], 22)
        self.assertEqual(shape["leading_gap_lines"], 1)
        self.assertEqual(shape["gaps"], [
            {"after_line": 20, "before_line": 22, "missing_lines": 1,
             "first_missing_line": 21, "last_missing_line": 21,
             "leading": True},
            {"after_line": 22, "before_line": 27, "missing_lines": 4,
             "first_missing_line": 23, "last_missing_line": 26,
             "leading": False},
        ])

    def test_same_line_body_has_no_leading_gap(self):
        row = _fn("0x100", "Widget::Open", cb="12")
        rows = [
            (r"E:\gamedcs\unit.cpp", 10, 0x100),
            (r"E:\gamedcs\unit.cpp", 10, 0x104),
            (r"E:\gamedcs\unit.cpp", 11, 0x108),
        ]
        shape = dreamcast._source_line_shape(row, rows)
        self.assertEqual(shape["first_body_line"], 10)
        self.assertEqual(shape["leading_gap_lines"], 0)

    def test_minimal_sh4_body_does_not_turn_close_line_into_body(self):
        row = _fn("0x100", "Widget::Open", cb="4")
        rows = [
            (r"E:\gamedcs\unit.cpp", 10, 0x100),
            (r"E:\gamedcs\unit.cpp", 90, 0x100),
        ]
        shape = dreamcast._source_line_shape(row, rows)
        self.assertTrue(shape["bodyless"])
        self.assertIsNone(shape["first_body_line"])
        self.assertIsNone(shape["leading_gap_lines"])

    def test_previous_closing_row_at_boundary_is_not_a_leading_gap(self):
        previous = _fn("0xe0", "Widget::Previous", cb="30")
        previous["line"] = "10"
        row = _fn("0x100", "Widget::Open", cb="32")
        row["line"] = "20"
        rows = [
            (r"E:\gamedcs\unit.cpp", 10, 0xe0),
            (r"E:\gamedcs\unit.cpp", 18, 0xfc),
            (r"E:\gamedcs\unit.cpp", 20, 0x100),
            (r"E:\gamedcs\unit.cpp", 70, 0x108),
            (r"E:\gamedcs\unit.cpp", 71, 0x114),
        ]
        shape = dreamcast._source_line_shape(row, rows, previous)
        self.assertTrue(shape["borrowed_boundary_line"])
        self.assertFalse(shape["procedure_line_reliable"])
        self.assertIsNone(shape["leading_gap_lines"])


class InlineClueTest(unittest.TestCase):
    def test_foreign_and_earlier_named_source_rows_are_grouped(self):
        caller = _fn("0x100", "Caller", cb="32")
        caller["line"] = "100"
        earlier = _fn("0x200", "EarlierHelper")
        earlier["line"] = "40"
        header = _fn("0x300", "HeaderHelper", "header.obj")
        header["file"] = r"E:\gamedcs\helper.h"
        header["line"] = "20"
        corpus = dreamcast.Corpus(
            functions=[caller, earlier, header], variables=[], bridges=[], claims=[])
        rows = [
            (r"E:\gamedcs\unit.cpp", 100, 0x100),
            (r"E:\gamedcs\helper.h", 20, 0x104),
            (r"E:\gamedcs\helper.h", 21, 0x108),
            (r"E:\gamedcs\unit.cpp", 40, 0x10c),
            (r"E:\gamedcs\unit.cpp", 101, 0x110),
        ]
        clues = dreamcast._inline_clue_rows(corpus, caller, rows)
        self.assertEqual([item["kind"] for item in clues], [
            "foreign-source", "foreign-source", "earlier-source-function"])
        self.assertEqual(clues[0]["definitions"], ["HeaderHelper"])
        self.assertEqual(clues[2]["definitions"], ["EarlierHelper"])
        self.assertTrue(all(item["confidence"] == "positive" for item in clues))
        groups = dreamcast._inline_clue_groups(clues)
        self.assertEqual([item["statement_rows"] for item in groups], [2, 1])
        self.assertEqual([item["emitted_size"] for item in groups], [8, 4])

    def test_ordinary_owning_source_rows_do_not_invent_inline_clues(self):
        caller = _fn("0x100", "Caller", cb="16")
        corpus = dreamcast.Corpus(
            functions=[caller], variables=[], bridges=[], claims=[])
        rows = [
            (r"E:\gamedcs\unit.cpp", 10, 0x100),
            (r"E:\gamedcs\unit.cpp", 11, 0x108),
        ]
        self.assertEqual(dreamcast._inline_clue_rows(corpus, caller, rows), [])

class CfgTest(unittest.TestCase):
    @staticmethod
    def _decoder(rows):
        def decode(address):
            mnemonic, operands = rows[address]
            return dc_asm.Instruction(address, 2, b"\0\0", mnemonic, operands)
        return decode

    def test_conditional_and_delayed_jump_form_blocks(self):
        rows = {
            0: ("bt", "0x8"),
            2: ("mov", "r0,r1"),
            4: ("bra", "0x10"),
            6: ("nop", ""),
            8: ("mov", "r2,r3"),
            10: ("rts", ""),
            12: ("nop", ""),
            16: ("rts", ""),
            18: ("nop", ""),
        }
        blocks = dc_asm.build_cfg(0, 20, self._decoder(rows))
        by_start = {block.start: block for block in blocks}
        self.assertEqual(sorted(by_start), [0, 2, 8, 16])
        self.assertEqual(by_start[0].successors, [2, 8])
        self.assertEqual(by_start[2].successors, [16])
        self.assertEqual([ins.address for ins in by_start[2].instructions],
                         [2, 4, 6])

    def test_uncovered_breakpoint_recovers_indirect_jump_arm(self):
        rows = {
            0: ("jmp", "@r1"),
            2: ("nop", ""),
            8: ("rts", ""),
            10: ("nop", ""),
        }
        blocks = dc_asm.build_cfg(0, 12, self._decoder(rows), extra_roots=[8])
        self.assertEqual([block.start for block in blocks], [0, 8])

    def test_dreamcast_fpu_words_are_decoded_as_sh4_instructions(self):
        # 0xfe17 is one of the common SH4 FPU words Capstone rejects unless
        # CS_MODE_SHFPU is explicitly combined with CS_MODE_SH4.
        data = bytearray(dc_asm.dc_lines.TEXT_RAW + 2)
        data[dc_asm.dc_lines.TEXT_RAW:] = bytes.fromhex("17fe")
        ins = dc_asm._decode_capstone(bytes(data))(0)
        self.assertNotEqual(ins.mnemonic, ".word")

    def test_control_events_resolve_pool_call_without_scanning_pool_as_code(self):
        target = dc_asm.dc_lines.POOL_BASE + 0x100
        data = bytearray(dc_asm.dc_lines.TEXT_RAW + 8)
        data[dc_asm.dc_lines.TEXT_RAW:dc_asm.dc_lines.TEXT_RAW + 4] = \
            bytes.fromhex("00d00b40")  # mov.l @(0,pc),r0; jsr @r0
        data[dc_asm.dc_lines.TEXT_RAW + 4:] = struct.pack("<I", target)
        view = {"blocks": [{"instructions": [
            {"address": 0, "bytes": "00d0", "mnemonic": "mov.l"},
            {"address": 2, "bytes": "0b40", "mnemonic": "jsr"},
        ]}]}
        events = dc_asm.control_events(view, bytes(data))
        self.assertEqual(events, {2: {"call_target_va": target}})



class WrongNamespaceTest(unittest.TestCase):
    def test_guessed_subcommands_name_their_home(self):
        import os
        from pathlib import Path
        saved = dreamcast.LOG
        dreamcast.LOG = Path(os.devnull)
        try:
            for argv, hint in ((["init", "--help"], "homm3 init"),
                               (["verify", "--changed"], "homm3 build"),
                               (["raw", "-h"], "--no-breakpoints")):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    self.assertEqual(dreamcast.main(argv), 2)
                self.assertIn(hint, err.getvalue())
        finally:
            dreamcast.LOG = saved


if __name__ == "__main__":
    unittest.main()
