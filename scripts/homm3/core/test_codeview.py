"""Hermetic tests for the VC6 classic COFF line reader."""
from __future__ import annotations

import struct
import tempfile
from pathlib import Path
import unittest

from homm3.core import codeview


def _symbol(name: str, value: int, section: int, typ: int,
            storage: int, aux: int) -> bytes:
    return struct.pack("<8sIhHBB", name.encode().ljust(8, b"\0"), value,
                       section, typ, storage, aux)


def _fixture(functions: int = 1) -> bytes:
    """Minimal i386 COFF with one /Z7-style contribution per function."""
    header_size = 20 + functions * 40
    chunks = []
    sections = []
    cursor = header_size
    symbol_indices = []
    for index in range(functions):
        code = bytes((0x90 + index, 0xC3))
        raw_offset = cursor
        chunks.append(code)
        cursor += len(code)
        symbol_index = index * 4
        symbol_indices.append(symbol_index)
        records = (
            struct.pack("<IH", symbol_index, 0)
            + struct.pack("<IH", 0, 1)
            + struct.pack("<IH", 0, 2)
            + struct.pack("<IH", 1, 3)
        )
        line_offset = cursor
        chunks.append(records)
        cursor += len(records)
        sections.append(struct.pack(
            "<8sIIIIIIHHI", b".text\0\0\0", 0, 0, len(code), raw_offset,
            0, line_offset, 0, 4, 0x60000020))

    sym_offset = cursor
    symbols = []
    for index, symbol_index in enumerate(symbol_indices):
        begin = 10 + index * 10
        symbols.append(_symbol("func", 0, index + 1, 0x20, 2, 1))
        function_aux = bytearray(18)
        struct.pack_into("<II", function_aux, 0, symbol_index + 2, 2)
        symbols.append(bytes(function_aux))
        symbols.append(_symbol(".bf", 0, index + 1, 0, 101, 1))
        bf_aux = bytearray(18)
        struct.pack_into("<H", bf_aux, 4, begin)
        symbols.append(bytes(bf_aux))
    header = struct.pack("<HHIIIHH", 0x14C, functions, 0, sym_offset,
                         functions * 4, 0, 0)
    return header + b"".join(sections) + b"".join(chunks) \
        + b"".join(symbols) + struct.pack("<I", 4)


class CodeViewLinesTest(unittest.TestCase):
    def _write(self, data: bytes, directory: str) -> Path:
        path = Path(directory) / "fixture.obj"
        path.write_bytes(data)
        return path

    def test_relative_lines_repeated_offsets_and_code(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(_fixture(), directory)
            result = codeview.parse_lines(path)[("func", 0)]
        self.assertEqual(result.begin_line, 10)
        self.assertEqual([(row.offset, row.line) for row in result.lines],
                         [(0, 11), (0, 12), (1, 13)])
        self.assertEqual(result.code, b"\x90\xc3")

    def test_duplicate_function_names_are_ordinal(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(_fixture(2), directory)
            result = codeview.parse_lines(path)
            second = codeview.function_bytes(path, "func", 1)
        self.assertEqual(set(result), {("func", 0), ("func", 1)})
        self.assertEqual(result[("func", 1)].begin_line, 20)
        self.assertEqual(second, b"\x91\xc3")

    def test_truncated_line_table_is_rejected(self):
        payload = bytearray(_fixture())
        struct.pack_into("<I", payload, 20 + 28, len(payload) + 0x100)
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(bytes(payload), directory)
            with self.assertRaisesRegex(codeview.CodeViewError,
                                        "truncated COFF section 1 line table"):
                codeview.parse_lines(path)

    def test_missing_function_is_explicit(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(_fixture(), directory)
            with self.assertRaisesRegex(codeview.CodeViewError, "not found"):
                codeview.function_bytes(path, "absent", 0)


if __name__ == "__main__":
    unittest.main()
