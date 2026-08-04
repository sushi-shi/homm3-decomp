#!/usr/bin/env python3
"""ONE-OFF carve script (not a pipeline actor; already ran, 2026-08-04) -
recovered real names for the zlib map's working labels from OUR OWN
compiled base objects. Kept for provenance; rerunning is only meaningful
after a boundary correction in the map.

A label like `zlib_206ec0_sub00_208870` is a zlib static internal the DNA
archive channel could not pin to a symbol. But the base objects we compile
from vendor/zlib-1.1.3 with the pinned toolchain contain every function -
statics included - with its exact symbol and bytes. Per member (unit):

  pass 1  each base-obj function's bytes (trailing 0x90/0xCC padding
          trimmed, relocation dwords masked) is searched in the member's
          RETAIL extent; a globally-unique hit that lands on a map rva
          claims that rva's name. Existing proven names must AGREE - a
          disagreement is fatal, never arbitrated.
  pass 2  hdmap's bracket trick: still-unmatched obj functions retry with
          shrinking windows INSIDE the bracket between their resolved
          link-order neighbours, unique-in-bracket only.
  gate    the (obj order -> rva) map must stay monotone - contributions
          preserve section order.

Matches are recompiled-identity evidence (the same channel that proved the
original 35) and are written back into config/retail-zlib-map.tsv in
place, preserving its header. Run after `homm3 build` has produced
build/objdiff/base/.
"""
from __future__ import annotations

import sys
from collections import defaultdict

from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.carve import common
from homm3.carve.dna import masked_find

ZLIB_MAP = common.HOMM3_DIR / "config/retail-zlib-map.tsv"
BASE_DIR = common.HOMM3_DIR / "build/objdiff/base"
FUNCTION_TYPE = 0x20
PAD = (0x90, 0xCC)


def obj_functions(path):
    """[(symbol name, masked-ready bytes, mask)] in section order."""
    coff = CoffObject(path.read_bytes())
    text = [s for s in coff.sections if s.name.startswith(".text")]
    reloc_sites = defaultdict(list)
    for reloc in coff.relocations:
        reloc_sites[reloc.section].append(reloc.site)
    out = []
    for section in text:
        payload = coff.section_bytes(section)
        funcs = sorted(
            (s for s in coff.symbols.values()
             if s.section == section.index and s.typ == FUNCTION_TYPE
             and s.storage_class in (2, 3)),
            key=lambda s: s.value)
        for sym, nxt in zip(funcs, funcs[1:] + [None]):
            end = nxt.value if nxt else len(payload)
            while end > sym.value + 1 and payload[end - 1] in PAD:
                end -= 1
            blob = payload[sym.value:end]
            mask = bytearray(len(blob))
            for site in reloc_sites[section.index]:
                lo = site - sym.value
                for k in range(lo, lo + 4):
                    if 0 <= k < len(blob):
                        mask[k] = 1
            out.append((sym.name, bytes(blob), bytes(mask)))
    return out


def bracket_find(window, blob, mask, lo, hi):
    fixed = [i for i in range(len(blob)) if not mask[i]]
    hits = []
    for start in range(max(lo, 0), min(hi, len(window) - len(blob) + 1)):
        if all(window[start + i] == blob[i] for i in fixed):
            hits.append(start)
            if len(hits) > 1:
                break
    return hits


def main(argv=None) -> int:
    lines = ZLIB_MAP.read_text().splitlines()
    header_end = next(i for i, l in enumerate(lines)
                      if l.startswith("rva\t"))
    rows = [l.split("\t") for l in lines[header_end + 1:] if l]
    by_unit = defaultdict(list)
    for r in rows:
        by_unit[r[3]].append(r)

    image, _info = common.load_image()
    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)

    renamed = corroborated = unmatched = 0
    for unit, unit_rows in sorted(by_unit.items()):
        obj_path = BASE_DIR / f"{unit}.obj"
        if not obj_path.is_file():
            print(f"[zlib names] {unit}: no base obj - skipped")
            continue
        rvas = [int(r[0], 16) for r in unit_rows]
        sizes = [int(r[1]) for r in unit_rows]
        lo = min(rvas) - text.rva
        hi = max(r + s for r, s in zip(rvas, sizes)) - text.rva
        window = blob[lo:hi]
        row_at = {rva: r for rva, r in zip(rvas, unit_rows)}

        functions = obj_functions(obj_path)
        resolved = {}  # obj index -> map rva
        for index, (name, body, mask) in enumerate(functions):
            hits = masked_find(window, body, bytearray(mask))
            if hits and len(hits) == 1:
                rva = text.rva + lo + hits[0]
                if rva in row_at:
                    resolved[index] = rva

        # pass 2: bracket retries with shrinking windows
        changed = True
        while changed:
            changed = False
            for index, (name, body, mask) in enumerate(functions):
                if index in resolved or len(body) < 8:
                    continue
                prev = max((i for i in resolved if i < index), default=None)
                nxt = min((i for i in resolved if i > index), default=None)
                blo = (resolved[prev] - text.rva - lo + 1) if prev is not None else 0
                bhi = (resolved[nxt] - text.rva - lo) if nxt is not None \
                    else len(window)
                size = len(body)
                while size >= 8:
                    hits = bracket_find(window, body[:size], mask[:size],
                                        blo, bhi)
                    if len(hits) == 1:
                        rva = text.rva + lo + hits[0]
                        if rva in row_at:
                            resolved[index] = rva
                            changed = True
                        break
                    if len(hits) > 1:
                        break
                    size = size * 2 // 3

        ordered = sorted(resolved.items())
        if [rva for _i, rva in ordered] != sorted(
                rva for _i, rva in ordered):
            common.die(f"{unit}: matches break link-order monotonicity")
        claimed = set()
        for index, rva in ordered:
            name = functions[index][0]
            if rva in claimed:
                continue
            claimed.add(rva)
            row = row_at[rva]
            if row[2].startswith(("?", "_", "@")):
                if row[2] != name:
                    common.die(f"{unit}: 0x{rva:x} proven {row[2]} but obj "
                               f"match says {name} - review")
                corroborated += 1
            else:
                row[2] = name
                renamed += 1
        unmatched += len(unit_rows) - len(claimed)

    names = [r[2] for r in rows]
    if len(set(names)) != len(names):
        common.die("rename produced duplicate names")
    ZLIB_MAP.write_text("\n".join(lines[:header_end + 1]
                                  + ["\t".join(r) for r in rows]) + "\n")
    print(f"[zlib names] {renamed} working labels -> real symbols, "
          f"{corroborated} proven names corroborated, {unmatched} rows "
          f"still unmatched -> {ZLIB_MAP.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
