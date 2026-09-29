"""Zero literal sections and appended symbols in paired normalization."""
import struct
import unittest

from homm3.build import canonicalize_data_symbols as canon
from homm3.build.normalize_objs import _canonicalize_zero_literal_sections

LITERAL = "??_C@_00A@?$AA@"


def coff(section_name: bytes, characteristics: int, raw: bytes | None, size: int,
         symbol: str = LITERAL, extra: tuple[str, ...] = ()) -> bytes:
    """One section, its section symbol, and one external symbol at offset 0."""
    header_end = 20 + 40
    raw_offset = header_end if raw is not None else 0
    body = raw or b""
    symbol_offset = header_end + len(body)
    strings = bytearray(struct.pack("<I", 4))
    def name_field(name: str) -> bytes:
        encoded = name.encode("latin-1")
        if len(encoded) <= 8:
            return encoded.ljust(8, b"\0")
        offset = len(strings)
        strings.extend(encoded + b"\0")
        return struct.pack("<II", 0, offset)
    symbols = (name_field(section_name.decode()) + struct.pack("<IhHBB", 0, 1, 0, 3, 0)
               + name_field(symbol) + struct.pack("<IhHBB", 0, 1, 0, 2, 0))
    for other in extra:
        symbols += name_field(other) + struct.pack("<IhHBB", 4, 1, 0, 2, 0)
    struct.pack_into("<I", strings, 0, len(strings))
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, symbol_offset, 2 + len(extra), 0, 0)
    section = struct.pack("<8sIIIIIIHHI", section_name, 0, 0, size, raw_offset,
                          0, 0, 0, 0, characteristics)
    return header + section + body + symbols + bytes(strings)


class ZeroLiteralSectionsTest(unittest.TestCase):
    def test_bss_literal_moves_to_zero_initialized_data(self):
        base = coff(b".bss", 0xC0301080, None, 1)
        target = coff(b".data", 0xC0300040, b"\0", 1)
        result, _target, count = _canonicalize_zero_literal_sections(base, target)
        self.assertEqual(count, 1)
        parsed = canon.CoffObject(result)
        literal = next(s for s in parsed.symbols.values() if s.name == LITERAL)
        section = parsed.sections[literal.section - 1]
        self.assertEqual((section.name, literal.value), (".data", 0))
        self.assertEqual(parsed.section_bytes(section), b"\0")
        self.assertFalse(section.characteristics & 0x80)
        self.assertEqual(parsed.sections[0].name, ".bss")

    def test_mixed_bss_keeps_its_other_symbols(self):
        base = coff(b".bss", 0xC0300080, None, 8, extra=("??_B?1guard",))
        target = coff(b".data", 0xC0300040, b"\0", 1)
        result, _target, count = _canonicalize_zero_literal_sections(base, target)
        self.assertEqual(count, 1)
        parsed = canon.CoffObject(result)
        guard = next(s for s in parsed.symbols.values() if s.name == "??_B?1guard")
        self.assertEqual((guard.section, guard.value), (1, 4))
        literal = next(s for s in parsed.symbols.values() if s.name == LITERAL)
        self.assertEqual(parsed.sections[literal.section - 1].raw_size, 1)

    def test_other_bss_symbols_stay_uninitialized(self):
        base = coff(b".bss", 0xC0301080, None, 4, symbol="_g_counter")
        target = coff(b".data", 0xC0300040, b"\0\0\0\0", 4, symbol="_g_counter")
        result, _target, count = _canonicalize_zero_literal_sections(base, target)
        self.assertEqual((result, count), (base, 0))


class AppendUndefinedSymbolsTest(unittest.TestCase):
    def test_appended_externals_keep_existing_indices(self):
        from homm3.build.normalize_objs import _append_undefined_symbols
        payload = coff(b".data", 0xC0300040, b"\0", 1)
        result, indices = _append_undefined_symbols(
            payload, ["?g_statNames@@3PAPBDA", "_short"])
        parsed = canon.CoffObject(result)
        self.assertEqual(indices, {"?g_statNames@@3PAPBDA": 2, "_short": 3})
        self.assertEqual(parsed.symbols[1].name, LITERAL)
        self.assertEqual(parsed.symbols[2].name, "?g_statNames@@3PAPBDA")
        self.assertEqual(parsed.symbols[3].section, 0)
