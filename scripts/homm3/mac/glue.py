"""Identify MWLink CFM import glue using its instructions and loader provenance.

The six-instruction form was checked against MWLinkPPC 2.4 with an imported
function control. An instruction pattern alone does not identify an import:
the r2-relative cell must have a loader relocation to a transition-vector
import, with no addend. Repeated/ambiguous symbol names stay unresolved.
"""
from __future__ import annotations

from collections import defaultdict

from homm3.mac.loader import ImportedAddress, Loader
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, CallTarget


def imports(pef: PEF) -> dict[str, CallTarget]:
    loader = Loader(pef)
    anchor = loader.toc()
    candidates = defaultdict(list)
    suffix = bytes.fromhex("90410014 800c0000 804c0004 7c0903a6 4e800420")
    for section in pef.sections:
        if section.kind != 0:
            continue
        data = pef.contents(section.index)
        cursor = 0
        while True:
            found = data.find(suffix, cursor)
            if found < 0:
                break
            cursor = found + 1
            offset = found - 4
            if offset < 0 or offset % 4:
                continue
            word = int.from_bytes(data[offset:found], "big")
            if word & 0xffff0000 != 0x81820000:  # lwz r12,disp(r2)
                continue
            displacement = (word & 0xffff) - (0x10000 if word & 0x8000 else 0)
            cell = Address(anchor.section, anchor.offset + displacement)
            imported = loader.pointers.get(cell)
            if (not isinstance(imported, ImportedAddress) or imported.addend
                    or imported.symbol.symbol_class != 2):
                continue
            symbol = imported.symbol
            candidates["." + symbol.name].append(CallTarget(
                Address(section.index, offset), "import",
                f"import:{symbol.library}:{symbol.name}"))
    return {name: values[0] for name, values in candidates.items() if len(values) == 1}
