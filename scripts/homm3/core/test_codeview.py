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


def _fixture(functions: int = 1, files: tuple[str, ...] | None = None,
             bss_size: int = 0) -> bytes:
    """Minimal i386 COFF with one /Z7-style contribution per function."""
    section_count = functions + bool(bss_size)
    header_size = 20 + section_count * 40
    chunks = []
    sections = []
    cursor = header_size
    symbol_indices = []
    file_records = []
    symbol_cursor = 0
    for index in range(functions):
        file_record = b""
        if files is not None:
            filename = files[index].encode("latin1") + b"\0"
            count = (len(filename) + 17) // 18
            file_record = (_symbol(".file", 0, -2, 0, 103, count)
                           + filename.ljust(count * 18, b"\0"))
            symbol_cursor += 1 + count
        file_records.append(file_record)
        code = bytes((0x90 + index, 0xC3))
        raw_offset = cursor
        chunks.append(code)
        cursor += len(code)
        symbol_index = symbol_cursor
        symbol_cursor += 4
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

    if bss_size:
        sections.append(struct.pack(
            "<8sIIIIIIHHI", b".bss\0\0\0\0", 0, 0, bss_size, 0,
            0, 0, 0, 0, 0xC0000080))

    sym_offset = cursor
    symbols = []
    for index, symbol_index in enumerate(symbol_indices):
        symbols.append(file_records[index])
        begin = 10 + index * 10
        symbols.append(_symbol("func", 0, index + 1, 0x20, 2, 1))
        function_aux = bytearray(18)
        struct.pack_into("<II", function_aux, 0, symbol_index + 2, 2)
        symbols.append(bytes(function_aux))
        symbols.append(_symbol(".bf", 0, index + 1, 0, 101, 1))
        bf_aux = bytearray(18)
        struct.pack_into("<H", bf_aux, 4, begin)
        symbols.append(bytes(bf_aux))
    header = struct.pack("<HHIIIHH", 0x14C, section_count, 0, sym_offset,
                         symbol_cursor, 0, 0)
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
        self.assertIsNone(result.source_file)

    def test_large_uninitialized_section_has_no_file_payload(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(_fixture(bss_size=0x10000), directory)
            result = codeview.parse_lines(path)[("func", 0)]
            code = codeview.function_bytes(path, "func")
        self.assertEqual(result.code, b"\x90\xc3")
        self.assertEqual(code, result.code)
        self.assertEqual(result.begin_line, 10)

    def test_truncated_initialized_section_is_still_rejected(self):
        payload = bytearray(_fixture(bss_size=0x10000))
        struct.pack_into("<I", payload, 60 + 36, 0xC0000040)
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(bytes(payload), directory)
            with self.assertRaisesRegex(codeview.CodeViewError,
                                        "truncated COFF section 2 data"):
                codeview.parse_lines(path)

    def test_long_file_records_switch_between_tu_and_header(self):
        files = (r"Z:\repo\src\unit.cpp", r"Z:\repo\include\retained_header.h",
                 r"Z:\repo\src\unit.cpp")
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(_fixture(3, files), directory)
            result = codeview.parse_lines(path)
        self.assertEqual([result[("func", index)].source_file for index in range(3)],
                         list(files))

    def test_file_ownership_follows_symbols_not_section_order(self):
        payload = bytearray(_fixture(2, ("source.cpp", "header.h")))
        # Reverse the code-section order, updating section references but
        # leaving symbol/.file order intact. Duplicate-name ordinals reverse.
        payload[20:60], payload[60:100] = payload[60:100], payload[20:60]
        sym_offset, count = struct.unpack_from("<II", payload, 8)
        index = 0
        while index < count:
            offset = sym_offset + index * 18
            section = struct.unpack_from("<h", payload, offset + 12)[0]
            if section > 0:
                struct.pack_into("<h", payload, offset + 12, 3 - section)
            index += 1 + payload[offset + 17]
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(bytes(payload), directory)
            result = codeview.parse_lines(path)
        self.assertEqual(result[("func", 0)].source_file, "header.h")
        self.assertEqual(result[("func", 1)].source_file, "source.cpp")

    def test_empty_file_record_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(_fixture(files=("",)), directory)
            with self.assertRaisesRegex(codeview.CodeViewError, "empty COFF .file"):
                codeview.parse_lines(path)

    def test_duplicate_function_names_are_ordinal(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(_fixture(2), directory)
            result = codeview.parse_lines(path)
            second = codeview.function_bytes(path, "func", 1)
        self.assertEqual(set(result), {("func", 0), ("func", 1)})
        self.assertEqual(result[("func", 1)].begin_line, 20)
        self.assertEqual(second, b"\x91\xc3")

    def test_return_to_begin_line_uses_7fff_without_reanchoring(self):
        payload = bytearray(_fixture())
        line_offset = struct.unpack_from("<I", payload, 20 + 28)[0]
        struct.pack_into("<H", payload, line_offset + 3 * 6 + 4, 0x7fff)
        with tempfile.TemporaryDirectory() as directory:
            path = self._write(bytes(payload), directory)
            result = codeview.parse_lines(path)[("func", 0)]
        self.assertEqual([(row.offset, row.line) for row in result.lines],
                         [(0, 11), (0, 12), (1, 10)])
        self.assertEqual(result.code, b"\x90\xc3")

    def test_large_relative_lines_are_not_masked_or_clamped(self):
        for stored in (0x7ffe, 0x8000, 0xffff):
            with self.subTest(stored=stored):
                payload = bytearray(_fixture())
                line_offset = struct.unpack_from("<I", payload, 20 + 28)[0]
                struct.pack_into("<H", payload, line_offset + 3 * 6 + 4, stored)
                with tempfile.TemporaryDirectory() as directory:
                    path = self._write(bytes(payload), directory)
                    result = codeview.parse_lines(path)[("func", 0)]
                self.assertEqual(result.lines[-1].line, 10 + stored)

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
