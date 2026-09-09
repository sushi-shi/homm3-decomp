"""Check recovered bank table owners and all 66 values against pinned inputs.

Read-only: never rewrites a compiler object, a retail image or scoring state.
NB11 proves the table type and loader-local storage. The actual VC6 symbols
must carry that local owner and enum type, with bytes in writable .data.
Every stored dword (including post-sentinel padding) must equal retail.
"""

import argparse
from pathlib import Path
import re
import struct

from homm3.analysis.dc_lines import load_symbols
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.core.common import load_image
from homm3.core.nb11_types import Types


TABLES = (("guardTypes", "guard_types", 0x6702a0, 55, "[11][5]"),
          ("rewardTypes", "reward_types", 0x67037c, 11, "[11]"))


def require_values(actual, expected, name):
    if actual != expected:
        raise ValueError(name + ": table bytes differ from pinned retail")


def verify(path):
    symbols = load_symbols()
    types = Types.from_symbols(symbols)
    owner = symbols.procedures[0x7112c]
    if owner.name != "initialize_creature_bank_traits":
        raise ValueError("Dreamcast loader identity changed")
    obj = CoffObject(path.read_bytes())
    retail, _ = load_image()
    for current, original, address, count, dimensions in TABLES:
        declarations = [row for rows in symbols.module_info.values() for row in rows
                        if row.get("name") == original and row.get("kind") == "static"]
        if len(declarations) != 1:
            raise ValueError("ambiguous NB11 table owner: " + original)
        declaration = declarations[0]
        if declaration.get("procedure") != owner.record_offset:
            raise ValueError("table is not a loader-local static: " + original)
        if types.declaration(declaration["type_index"], original) != "TCreatureType " + original + dimensions:
            raise ValueError("unexpected NB11 table type: " + original)

        # The encoded local-scope ordinal changes with preceding scopes;
        # require the actual owning function, not an assumed ordinal of 1.
        local_name = re.compile(r"^_?\?" + re.escape(current)
                                + r"@\?[0-9A-Z]+\?\?initializeCreatureBankTraits@@YIEXZ@")
        candidates = [symbol for symbol in obj.symbols.values()
                      if local_name.match(symbol.name)
                      and symbol.section > 0]
        if len(candidates) != 1 or "W4TCreatureType@@" not in candidates[0].name:
            raise ValueError("missing unique VC6 enum/local-static owner: " + current)
        symbol = candidates[0]
        section = obj.sections[symbol.section - 1]
        if section.name != ".data" or not section.characteristics & 0x80000000:
            raise ValueError("table must live in writable .data: " + current)
        candidate_bytes = obj.section_bytes(section)[symbol.value:symbol.value + count * 4]
        rva = address - retail.image_base
        retail_section = retail.section_of(rva)
        if retail_section.name != ".data":
            raise ValueError("retail table is not in .data: " + current)
        offset = rva - retail_section.rva
        expected = retail.blob(retail_section)[offset:offset + count * 4]
        if len(candidate_bytes) != count * 4 or len(expected) != count * 4:
            raise ValueError("truncated table: " + current)
        require_values(candidate_bytes, expected, current)
        # Non-mutating negative controls exercise content, ordering and extent.
        negative = (bytes([candidate_bytes[0] ^ 1]) + candidate_bytes[1:],
                    candidate_bytes[4:8] + candidate_bytes[:4] + candidate_bytes[8:],
                    candidate_bytes[:-4])
        if negative[1] == candidate_bytes:
            negative = (negative[0], candidate_bytes[-4:] + candidate_bytes[:-4], negative[2])
        for bad in negative:
            try:
                require_values(bad, expected, current)
            except ValueError:
                continue
            raise ValueError("table negative control was not rejected: " + current)
        print(f"PASS: {current}, loader-local mutable enum {dimensions}, {count} retail dwords")
        print("  ", struct.unpack("<" + str(count) + "i", candidate_bytes))
    print("PASS: both source owners, 66 dwords and six non-mutating negative controls")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("object", type=Path)
    verify(parser.parse_args().object)
