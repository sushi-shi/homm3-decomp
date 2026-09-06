"""Synthetic NB11 fixtures: no game bytes or external symbol dump required."""
from __future__ import annotations

import struct
import unittest

from homm3.core import nb11


def _name(text):
    data = text.encode("ascii")
    return bytes([len(data)]) + data


def _symbol(kind, body=b""):
    return struct.pack("<HH", len(body) + 2, kind) + body


def fixture(*, bad_record=False, bad_lines=False, global_types=None) -> bytes:
    proc = _symbol(0x100b, struct.pack("<8IHB", 0, 0, 0, 32, 0, 32,
                                    0x1000, 0x100, 1, 0) + _name("Function"))
    block = _symbol(0x0207, struct.pack("<4IH", 4, 0, 8, 0x108, 1) + _name(""))
    local = _symbol(0x100d, struct.pack("<IIH", 12, 0x74, 25) + _name("value"))
    records = bytearray(proc + block + _symbol(6) + local)
    # pEnd is relative to the aligned-symbol subsection, including its signature.
    struct.pack_into("<I", records, 8, 4 + len(records))
    records += _symbol(6)
    if bad_record:
        struct.pack_into("<H", records, 0, 1)
    symbols = struct.pack("<I", 2) + records
    module = struct.pack("<4H2H2I", 0, 0, 1, 0x5643, 1, 0, 0x100, 32) + _name("C:\\unit.obj")

    source_name = _name("C:\\unit.cpp")
    file_offset = 20
    line_offset = file_offset + 16 + len(source_name)
    header = struct.pack("<HHIIIHH", 1, 1, file_offset, 0x100, 0x11f, 1, 0)
    file = struct.pack("<HHIII", 1, 0, 0xffffffff if bad_lines else line_offset,
                       0x100, 0x11f) + source_name
    lines = struct.pack("<HH3I3H", 1, 3, 0x100, 0x100, 0x108, 10, 11, 12)
    public = _symbol(0x1009, struct.pack("<IIH", 0, 0x100, 1) + _name("?Function@@YAXXZ"))
    public = struct.pack("<HHIII", 10, 12, len(public), 0, 0) + public

    stream = bytearray(b"NB11" + b"\0" * 4)
    entries = []
    sections = [(0x120, 1, module), (0x125, 1, symbols),
                (0x127, 1, header + file + lines), (0x12a, 65535, public)]
    if global_types is not None:
        sections.append((0x12b, 65535, global_types))
    for kind, index, data in sections:
        entries.append(struct.pack("<HHII", kind, index, len(stream), len(data)))
        stream += data
    struct.pack_into("<I", stream, 4, len(stream))
    stream += struct.pack("<HHIII", 16, 12, len(entries), 0, 0) + b"".join(entries)

    image = bytearray(0x400)
    image[:2] = b"MZ"
    struct.pack_into("<I", image, 0x3c, 0x80)
    image[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HH", image, 0x84, 0x1a6, 1)
    struct.pack_into("<H", image, 0x94, 0xe0)
    optional = 0x98
    struct.pack_into("<H", image, optional, 0x10b)
    struct.pack_into("<I", image, optional + 28, 0x10000)
    struct.pack_into("<II", image, optional + 144, 0x1000, 28)
    struct.pack_into("<8s4I", image, optional + 0xe0, b".text", 0x200, 0x1000, 0x200, 0x200)
    struct.pack_into("<4I", image, 0x200 + 12, 2, len(stream), 0, len(image))
    return bytes(image + stream)


class NB11Test(unittest.TestCase):
    def test_global_type_offsets_are_relative_to_the_record_area(self):
        modifier = _symbol(0x1001, struct.pack("<IH", 0x74, 1))
        pointer = _symbol(0x1002, struct.pack("<II", 0x1000, 10))
        types = struct.pack("<4I", 2, 2, 0, len(modifier)) + modifier + pointer
        symbols = nb11.parse(fixture(global_types=types))
        self.assertEqual(symbols.type_records, {0x1000: modifier, 0x1001: pointer})
        with self.assertRaisesRegex(nb11.NB11Error, "truncated"):
            nb11.parse(fixture(global_types=types[:-1]))

    def test_variables_keep_type_argument_boundary_and_coincident_scope_parents(self):
        proc = _symbol(0x100b, struct.pack("<8IHB", 0, 0, 0, 32, 4, 28,
                                         0x1000, 0x100, 1, 0) + _name("Overloaded"))
        param = _symbol(0x100d, struct.pack("<IIH", 4, 0x74, 25) + _name("value"))
        endarg = _symbol(0xa)
        outer_offset = 4 + len(proc + param + endarg)
        inner_offset = outer_offset + 24
        outer = _symbol(0x207, struct.pack("<4IH", 4, inner_offset + 24 + 20 + 4,
                                          8, 0x108, 1) + _name(""))
        inner = _symbol(0x207, struct.pack("<4IH", outer_offset, inner_offset + 24 + 20,
                                          8, 0x108, 1) + _name(""))
        # Align the synthetic records like sstAlignSym, where an empty block is 24 B.
        outer = struct.pack("<H", 22) + outer[2:] + b"\0"
        inner = struct.pack("<H", 22) + inner[2:] + b"\0"
        local = _symbol(0x100d, struct.pack("<IIH", 0xfffffffc, 0x75, 24) + _name("value"))
        body = bytearray(proc + param + endarg + outer + inner + local + _symbol(6) + _symbol(6))
        struct.pack_into("<I", body, 8, 4 + len(body))
        body += _symbol(6)
        symbols = nb11.Symbols()
        nb11._symbols(nb11._View(struct.pack("<I", 2) + body), 4, symbols, {1: 0x11000}, "unit.obj")
        result = symbols.procedures[0x100]
        self.assertEqual((result.type_index, result.debug_start, result.debug_end), (0x1000, 4, 28))
        self.assertEqual([(v.name, v.kind, v.type_index) for v in result.variables],
                         [("value", "param", 0x74), ("value", "local", 0x75)])
        self.assertEqual(result.variables[1].scope, inner_offset)
        self.assertEqual(result.variables[1].storage, "fp-0x4")
        self.assertEqual(result.lexical_scopes[1].parent, outer_offset)

    def test_embedded_procedures_nested_scopes_names_and_duplicate_lines(self):
        symbols = nb11.parse(fixture())
        proc = symbols.procedures[0x100]
        self.assertEqual((proc.name, proc.size), ("Function", 32))
        self.assertEqual(proc.scopes, [(0x108, 8)])
        self.assertEqual(proc.locals, [("sp", 12, "value")])
        self.assertEqual(symbols.names[0x11100], "?Function@@YAXXZ")
        self.assertEqual(symbols.source_lines["unit.obj"], [
            ("C:\\unit.cpp", 10, 0x100), ("C:\\unit.cpp", 11, 0x100),
            ("C:\\unit.cpp", 12, 0x108)])
        self.assertEqual(symbols.line_table(0x100, 8), [
            (0x100, 10, "C:\\unit.cpp"), (0x100, 11, "C:\\unit.cpp")])

    def test_truncated_stream_is_rejected(self):
        with self.assertRaisesRegex(nb11.NB11Error, "truncated"):
            nb11.parse(fixture()[:-1])

    def test_wrong_debug_signature_is_rejected(self):
        image = bytearray(fixture())
        image[0x400:0x404] = b"RSDS"
        with self.assertRaisesRegex(nb11.NB11Error, "NB11"):
            nb11.parse(bytes(image))

    def test_invalid_symbol_length_is_rejected(self):
        with self.assertRaisesRegex(nb11.NB11Error, "record length"):
            nb11.parse(fixture(bad_record=True))

    def test_source_line_offset_outside_subsection_is_rejected(self):
        with self.assertRaisesRegex(nb11.NB11Error, "truncated"):
            nb11.parse(fixture(bad_lines=True))


if __name__ == "__main__":
    unittest.main()
