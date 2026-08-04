"""homm3.sema.rva - the address dossier: what is at this address?

Pure joins, no analysis - the canonical first command on any address.
Accepts an RVA or a full VA (values >= the image base reduce). Every
row degrades to an explanatory line when its input is absent rather
than erroring: the dossier answers with whatever is known.

rc: 0 = answered, 2 = error (unparseable address).
"""
from __future__ import annotations

import re
import struct

from homm3.core import common
from homm3.match import universe
from homm3.sema.context import get_context

_MACROS = r"(?:VA|VA_COMPGEN|DATA|DATA_COMPGEN|DATA_COMPGEN_GUARD|VTBL2?)"


def _src_locs(va: int, limit: int = 3):
    """file:line of every annotation macro claiming this VA in the
    hand-owned tree (padding-agnostic: 0x5de0 == 0x00005de0). The
    address may sit at any argument position - VTBL/VTBL2 carry it
    LAST (include/va.h), not first."""
    pattern = re.compile(
        rf"\b{_MACROS}\s*\([^)]*\b0x0*{va:x}\b", re.IGNORECASE)
    hits = []
    for root in (common.HOMM3_DIR / "src", common.HOMM3_DIR / "include"):
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".c", ".cpp", ".h", ".hpp"):
                continue
            try:
                lines = path.read_text(errors="replace").splitlines()
            except OSError:
                continue
            for lineno, line in enumerate(lines, 1):
                if pattern.search(line):
                    hits.append(f"{path.relative_to(common.HOMM3_DIR)}:{lineno}")
                    if len(hits) >= limit:
                        return hits
    return hits


def _vtable_slots_holding(ctx, fn_rva: int):
    """Every (vtable, slot) whose stored VA is this function's entry -
    complete, so the renderer can cap the display and still say how
    many more there are (a slot-0 dtor sits in dozens of vtables)."""
    want = ctx.image.image_base + fn_rva
    image = ctx.image
    hits = []
    for vt, count in ctx.vtables:
        section = image.section_of(vt)
        if section is None:
            continue
        offset = section.raw_offset + (vt - section.rva)
        for k in range(count):
            if struct.unpack_from("<I", image.data, offset + 4 * k)[0] == want:
                hits.append((vt, k))
    return hits


def run(args) -> None:
    ctx = get_context()
    rva = ctx.symbols.resolve(args.addr)
    va = ctx.image.image_base + rva
    section = ctx.image.section_of(rva)
    print(f"rva 0x{rva:08x}  va 0x{va:08x}  "
          f"[{section.name if section else 'no section'}]")

    frow = ctx.symbols.funcs.get(rva)
    fn = rva if frow else ctx.symbols.owner(rva)
    if frow:
        name, unit, size, prov = frow
        print(f"  function  {name} [{unit}]  size 0x{size:x}  ({prov})")
    elif fn is not None:
        name, unit, size, prov = ctx.symbols.funcs[fn]
        print(f"  inside    {name} [{unit}] +0x{rva - fn:x}  "
              f"(starts 0x{fn:08x}, size 0x{size:x}, {prov})")
    drow = ctx.symbols.datas.get(rva)
    if drow:
        print(f"  data      {drow[0]}  size 0x{drow[2]:x}")
    if frow is None and fn is None and drow is None:
        near = ctx.symbols.nearest_data(rva)
        if near:
            print(f"  near      ~{near[1]}+0x{near[2]:x}")
        else:
            print("  symbol    (no function or data symbol here)")

    if fn is not None:
        cat = ctx.classes.get(fn, "target")
        note = universe.EXCLUDED_NOTES.get(cat)
        print(f"  universe  {cat}" + (f"  ({note})" if note else ""))

    locs = _src_locs(va)
    if locs:
        print(f"  src       {'; '.join(locs)}")
    elif fn is not None and fn != rva:
        owner_locs = _src_locs(ctx.image.image_base + fn)
        print("  src       " + (f"(owner) {'; '.join(owner_locs)}"
                                if owner_locs
                                else "(no VA/DATA claim in src or include)"))
    else:
        print("  src       (no VA/DATA claim in src or include)")

    slot = ctx.vtable_of(rva)
    if slot:
        vt, k = slot
        vrow = ctx.symbols.datas.get(vt)
        print(f"  vtable    this address IS slot {k} of vtable 0x{vt:x}"
              + (f" {vrow[0]}" if vrow else ""))
    if fn is not None:
        held = _vtable_slots_holding(ctx, fn)
        for vt, k in held[:4]:
            vrow = ctx.symbols.datas.get(vt)
            print(f"  vtable    held by vtable 0x{vt:x}"
                  + (f" {vrow[0]}" if vrow else "") + f" slot {k}")
        if len(held) > 4:
            print(f"  vtable    ... (+{len(held) - 4} more vtable(s) hold "
                  "this entry - `homm3 sema xref` lists them all)")

    if frow and frow[1]:
        pct = ctx.fn_fuzzy(frow[1], frow[0])
        if pct is not None:
            exact = "  (EXACT)" if pct >= 100.0 else ""
            print(f"  match     {pct:.2f}%{exact}")
        elif ctx.report is None:
            print("  match     (no report.json - run `homm3 build`)")
        else:
            print("  match     (not in the report - unit not compared)")
