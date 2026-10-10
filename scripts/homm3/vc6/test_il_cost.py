"""The C1XX inline cost: tuple counting, record lookup and negative controls."""

import os
import struct
import tempfile
import unittest
from pathlib import Path

from homm3.vc6 import il_cost

# Operand formats copied from the pinned C1XX tables (0xbeaac / 0xbd9e8)
# for the opcodes the fixture bodies use; the live test re-reads them.
FORMATS = il_cost.Formats(
    ops={0x02: b"", 0x26: b"\x08", 0x28: b"\x08\x1b", 0x29: b"\x08\x0f",
         0x2D: b"\x08\x0f", 0x30: b"\x06", 0x32: b"\x06", 0x33: b"\x0b",
         0x3A: b"\x08\x0f", 0x46: b"\x0f", 0x47: b"\x0f", 0x4B: b"\x0f",
         0x4C: b"\x0f", 0x4D: b"", 0x53: b"\x0f", 0x54: b"\x02\x0f"},
    infos={0x01: b"\x01", 0x02: b"\x03", 0x1F: b"\x14", 0x20: b"\x15"})

# Two bodies captured from `void s0(int row) {}` and
# `void s1(int row) { gA[0] = gA[1] + row; }` (game profile): their gl
# records carry costs 13 and 25.
S0 = bytes.fromhex(
    "4f1f800000a4004f20084f02d9004f01044f23100e80ffff535326d800462dd7004c53"
    "4f01044f01053ada0054024f010529da004754015400")
S1 = bytes.fromhex(
    "4f1f800000a4004f20084f02d9004f01074f23100e80ffff535326dc00462ddb004c53"
    "4f01074f01084f010826d50033a0410028000026d50033a04104280000304126db0030"
    "410232414b4f01093add0054024f010929dd004754015400")


def stream(*bodies: bytes, tail: bytes = b"\x4d") -> tuple[bytes, list[int]]:
    first = 0x10
    ex = bytearray(b"\x5b\x80" + struct.pack("<I", first))
    ex += bytes(first - len(ex))
    starts = []
    for body in bodies:
        starts.append(len(ex))
        ex += body
    return bytes(ex + tail), starts


class TupleCountTests(unittest.TestCase):
    def test_counted_tuples_equal_the_recorded_costs(self):
        ex, starts = stream(S0, S1)
        found = il_cost.bodies(ex, FORMATS)
        self.assertEqual([b.start for b in found], starts)
        self.assertEqual([b.cb for b in found], [13, 25])
        self.assertEqual([b.handle for b in found], [0xD8, 0xDC])

    def test_info_tuples_do_not_count(self):
        body = il_cost.bodies(stream(S1)[0], FORMATS)[0]
        infos = [t for t in body.tuples if t.op == il_cost.INFO_OP]
        self.assertTrue(infos)
        self.assertFalse(any(t.counted for t in infos))
        warnings = [t for t in infos if t.operands[0] == il_cost.WARNING_STATE]
        self.assertEqual(warnings[0].operands, [0x23, (716, 0x0E)])

    def test_cost_is_attributed_to_source_lines(self):
        body = il_cost.bodies(stream(S1)[0], FORMATS)[0]
        rows = {(r["line"]): r["cost"] for r in il_cost.lines(body, {0xD9: "h1.cpp"})}
        self.assertEqual(rows, {7: 7, 8: 12, 9: 6})
        self.assertEqual(sum(rows.values()), 25)

    def test_inline_asm_blob_is_skipped_with_its_length_byte(self):
        formats = il_cost.Formats(dict(FORMATS.ops), {**FORMATS.infos, 0x16: b"\x0c"})
        ex = bytes.fromhex("4f1f800000a400" "5353" "26d800" "4f160404a90001" "4b")
        tuples = il_cost.parse(ex, formats, 0)
        self.assertEqual([t.op for t in tuples if t.counted], [0x53, 0x53, 0x26, 0x4B])

    def test_truncated_operand_desynchronizes(self):
        ex, _ = stream(S1[:-1], tail=b"")
        with self.assertRaises(il_cost.Desync):
            il_cost.bodies(ex, FORMATS)

    def test_unknown_opcode_desynchronizes(self):
        with self.assertRaisesRegex(il_cost.Desync, "unknown opcode"):
            il_cost.bodies(stream(S0 + b"\xfe")[0], FORMATS)

    def test_counted_tuple_before_a_body_is_rejected(self):
        with self.assertRaisesRegex(il_cost.Desync, "before the first body"):
            il_cost.bodies(stream(b"\x53" + S0)[0], FORMATS)


class RecordTests(unittest.TestCase):
    def test_record_fields_follow_c2_reader_order(self):
        gl = (b"\x0e\xd8\x00\x00?s0@@YIXH@Z\x00\x07\x05\x00\x04\x00\x00"
              b"\x80\xf4\x03\x00\x00" b"\x1e" b"\x80\xf0\x00" b"\x68\x00" b"\x01")
        self.assertEqual(il_cost.gl_record(gl, 0x3F4),
                         dict(offset=gl.index(b"\x80\xf4"), sy=0x1E, cb=0xF0, flags=0x68))

    def test_sy_field_of_another_record_is_not_an_ex_match(self):
        decoy = b"\x80\x00\x10\x00\x00" + b"\x80\xf4\x03\x00\x00" + b"\x10\x68\x00\x01"
        self.assertIsNone(il_cost.gl_record(decoy, 0x3F4))

    def test_wide_handles_and_saturated_costs(self):
        gl = b"\x07\x04\xa4\x87\x02\x00\x00?destroyMsg@@YIXPAVCNetMsg@@@Z\x00"
        self.assertEqual(il_cost.gl_names(gl), {0x107A4: "?destroyMsg@@YIXPAVCNetMsg@@@Z"})
        self.assertEqual(il_cost._Reader(b"\xa4\x87\x02\x00").handle(), 0x107A4)
        self.assertEqual(il_cost.stored_cost(32767), 32767)
        self.assertEqual(il_cost.stored_cost(40000), 40000 - 0x10000)
        self.assertEqual(il_cost.stored_cost(70000), -1)

    def test_file_records_use_both_handle_forms(self):
        gl = (b"\x00\x12\xd6\x00Z:\\src\\town.cpp\x00"
              b"\x12\x27\x88\x02\x00Z:\\include\\game.h\x00")
        names = il_cost.gl_names(gl)
        self.assertEqual(names[0xD6], "Z:\\src\\town.cpp")
        self.assertEqual(names[0x10827], "Z:\\include\\game.h")

    def test_check_records_reports_disagreement(self):
        body = il_cost.bodies(stream(S0)[0], FORMATS)[0]
        body.record = dict(cb=14, flags=0x68)
        self.assertEqual(il_cost.check_records([body])["differ"], [(None, 14, 13)])


HARNESS = """
int gA[64];
int gB;
void ext(int);
void s0(int row) {}
void s1(int row) { gA[0] = gA[1] + row; }
void s2(int row) { gA[0] = gA[1] + row; gA[1] = gA[2] + row; }
void braced(int a) { if (a) { gB = 1; } }
void plain(int a) { if (a) gB = 1; }
void calls() { ext(1); }
"""


def _sized(name: str, cost: int, prefix: str = "") -> str:
    """A body of exactly `cost` tuples: 13 + 12k + 4a + 5b."""
    remain = cost - 13
    for twelves in range(remain // 12, -1, -1):
        rest = remain - 12 * twelves
        for fours in range(rest // 4 + 1):
            if (rest - 4 * fours) % 5 == 0:
                body = ([f"gA[{i}] = gA[{i + 1}] + row;" for i in range(twelves)]
                        + ["gB = 1;"] * fours + ["gB = gB;"] * ((rest - 4 * fours) // 5))
                return f"{prefix}void {name}(int row) {{ {' '.join(body)} }}\n"
    raise ValueError(cost)


@unittest.skipUnless(os.environ.get("HOMM3_TEST_VC6_CB") == "1",
                     "requires the pinned VC6 toolchain and Wine")
class LiveCostTests(unittest.TestCase):
    def test_counts_save_gate_and_pinned_formats(self):
        from homm3.vc6 import _unit
        formats = il_cost.pinned_formats()
        for op, fmt in FORMATS.ops.items():
            self.assertEqual(formats.ops[op], fmt, hex(op))
        for subtype, fmt in FORMATS.infos.items():
            self.assertEqual(formats.infos[subtype], fmt, hex(subtype))
        self.assertEqual(formats.save_limit, 175)
        text = HARNESS + "".join(
            _sized(f"auto{c}", c) + _sized(f"inl{c}", c, "inline ")
            + _sized(f"stat{c}", c, "static ") for c in (174, 175, 200))
        text += "void use() { " + " ".join(
            f"{k}{c}(1);" for c in (174, 175, 200) for k in ("auto", "inl", "stat")) + " }\n"
        with tempfile.TemporaryDirectory(prefix="vc6-cb-") as temporary:
            source = Path(temporary) / "cb_harness.cpp"
            source.write_text(text)
            streams = il_cost.capture(source, _unit.flags_for_unit("game"),
                                      Path(temporary) / "il")
        found = il_cost.functions(streams, formats)
        gate = il_cost.check_records(found)
        self.assertEqual(gate["differ"], [])
        costs = {b.name: b.cb for b in found}
        saved = {b.name: bool(b.record["flags"] & il_cost.SAVED) for b in found if b.record}
        self.assertEqual([costs[f"?s{n}@@YIXH@Z"] for n in range(3)], [13, 25, 37])
        self.assertEqual(costs["?braced@@YIXH@Z"] - costs["?plain@@YIXH@Z"], 2)
        self.assertEqual(costs["?calls@@YIXXZ"], 18)
        for cost in (174, 175, 200):
            self.assertEqual(costs[f"?auto{cost}@@YIXH@Z"], cost)
            self.assertEqual(saved[f"?auto{cost}@@YIXH@Z"], cost < 175)
            self.assertTrue(saved[f"?inl{cost}@@YIXH@Z"])
        self.assertTrue(any(name.startswith("?stat200") and flag
                            for name, flag in saved.items()))


if __name__ == "__main__":
    unittest.main()
