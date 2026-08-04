#!/usr/bin/env python3
"""homm3.carve.relate - join names <-> functions <-> vtables for the pinned exe.

Ties the three carve deliverables together for OUR retail image only (no
Dreamcast/HD addresses ever leak in - those are other pressings). Two joins:

  retail-function-symbols.csv   every function in retail-functions.tsv, with
                                the name/signature we have for its ENTRY (from
                                retail-function-names.csv, entry rows only -
                                interior NH3API addresses describe a different
                                pressing and are excluded), plus whether the
                                entry is a vtable slot target and which
                                library the DNA pass proved it in.
  retail-vtable-symbols.csv     every vtable in retail-vtables.tsv, per slot:
                                the slot's target function and that function's
                                method name, plus the owning CLASS where
                                NH3API's `NH3API_SPECIALIZE_TYPE_VFTABLE(addr,
                                class)` addr lands on (or inside) the vtable.

The vtable->class channel is NH3API's vftable-address specializations. They
are external-unverified, but unlike the call-macro addresses they name
`.rdata` vtable STARTS, which we can check: 54 of 96 land exactly on our
vtable starts; 34 land on an interior slot (our cut evidence - a ctor's vptr
store - starts the piece a few slots earlier, the documented NH3API boundary
gap), recorded with the offset; 8 miss our pieces entirely. Class labels are
candidates, the slot->function->name topology is retail-derived.
"""
from __future__ import annotations

import csv
import re
import sys
from collections import defaultdict

from homm3.carve import common

NAMES = common.HOMM3_DIR / "config/retail-function-names.csv"
FUNCTIONS = common.CARVE_DIR / "functions.tsv"
VTABLES = common.CARVE_DIR / "vtables.tsv"
VTABLE_SLOTS = common.CARVE_DIR / "vtable_slots.tsv"
LIBRARIES = common.HOMM3_DIR / "config/retail-function-libraries.tsv"
NH3API_ROOT = common.HOMM3_DIR.parent / "homm3-symbols/NH3API/nh3api"

OUT_SYMBOLS = common.HOMM3_DIR / "config/retail-function-symbols.csv"
OUT_VTABLES = common.HOMM3_DIR / "config/retail-vtable-symbols.csv"

VFTABLE = re.compile(r"NH3API_SPECIALIZE_TYPE_VFTABLE\s*\(\s*(0x[0-9A-Fa-f]+)"
                     r"\s*,\s*([^)]+?)\s*\)")


def read_names_by_entry():
    """retail-function-names entry rows, keyed by rva (best row per rva)."""
    if not NAMES.is_file():
        common.die(f"{NAMES} missing - run `python3 -m homm3.carve names`")
    rows = list(csv.DictReader(
        line for line in NAMES.open() if not line.startswith("#")))
    by_rva = {}
    # prefer retail-proven sources, then dreamcast-joined, then plain nh3api
    rank = {"masked-archive": 0, "masked-zlib-obj": 0, "stock-fid": 1}

    def score(row):
        base = min((v for k, v in rank.items() if k in row["sources"]),
                   default=2)
        return (base, 0 if "dreamcast" in row["sources"] else 1)

    for row in rows:
        if row["carve_state"] != "entry":
            continue
        rva = int(row["rva"], 16)
        if rva not in by_rva or score(row) < score(by_rva[rva]):
            by_rva[rva] = row
    return by_rva


def read_vtable_classes():
    """NH3API vftable-address -> class."""
    classes = {}
    if NH3API_ROOT.is_dir():
        for path in NH3API_ROOT.rglob("*.hpp"):
            for m in VFTABLE.finditer(path.read_text(errors="replace")):
                classes[int(m.group(1), 16) - common.IMAGE_BASE] = m.group(2)
    return classes


def label_vtable(piece_rva, slot_count, classes):
    """The class whose vftable address == this piece's start, else one that
    falls inside its slots (record the interior offset)."""
    if piece_rva in classes:
        return classes[piece_rva], 0
    hi = piece_rva + slot_count * 4
    interior = sorted(a for a in classes if piece_rva < a < hi)
    if interior:
        return classes[interior[0]], interior[0] - piece_rva
    return "", -1


def main(argv=None) -> int:
    functions = [(int(r["rva"], 16), int(r["size"])) for r in
                 common.read_tsv(common.need(FUNCTIONS, "audit"))]
    functions.sort()
    names = read_names_by_entry()
    classes = read_vtable_classes()

    libraries = {}
    if LIBRARIES.is_file():
        for r in common.read_tsv(LIBRARIES):
            libraries[int(r["rva"], 16)] = (r["library"], r["symbol"])

    slot_rows = common.read_tsv(common.need(VTABLE_SLOTS, "vtables"))
    vtables = common.read_tsv(common.need(VTABLES, "vtables"))
    vt_count = {int(r["rva"], 16): int(r["function_count"]) for r in vtables}

    slot_of_target = defaultdict(list)
    slots_by_vt = defaultdict(list)
    for r in slot_rows:
        piece = int(r["piece_rva"], 16)
        if piece not in vt_count:
            continue  # only vtable-classified pieces
        target = int(r["target_rva"], 16)
        slot = int(r["slot"])
        slot_of_target[target].append((piece, slot))
        slots_by_vt[piece].append((slot, target, r["state"]))

    # --- join 1: function symbols -------------------------------------
    sym_rows = []
    named = with_lib = in_vtable = 0
    for rva, size in functions:
        row = names.get(rva)
        name = row["name"] if row else ""
        memberships = slot_of_target.get(rva, [])
        vt_field = ";".join(f"0x{v:x}#{s}" for v, s in memberships[:6])
        lib, lib_symbol = libraries.get(rva, ("", ""))
        if name:
            named += 1
        if lib:
            with_lib += 1
        if memberships:
            in_vtable += 1
        sym_rows.append({
            "rva": f"0x{rva:x}", "size": size, "name": name,
            "signature": row["signature"] if row else "", "library": lib,
            "library_symbol": lib_symbol, "vtable_slots": vt_field,
            "source": row["sources"] if row else "",
            "source_file": row["source_file"] if row else "",
            "line": row["line"] if row else ""})

    with OUT_SYMBOLS.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.carve relate - join of "
                 "retail-functions + retail-function-names + "
                 "retail-function-libraries.\n")
        for prov in common.provenance("homm3.carve.relate"):
            fh.write(prov + "\n")
        fh.write("# name/signature: ENTRY-only NH3API+Dreamcast candidates "
                 "(unverified); library_symbol is retail-byte-proven.\n")
        writer = csv.DictWriter(fh, fieldnames=list(sym_rows[0].keys()))
        writer.writeheader()
        writer.writerows(sym_rows)

    # --- join 2: vtable symbols ---------------------------------------
    vt_rows = []
    labeled = slot_named = slot_total = 0
    for piece in sorted(vt_count):
        cls, offset = label_vtable(piece, vt_count[piece], classes)
        if cls:
            labeled += 1
        for slot, target, state in sorted(slots_by_vt[piece]):
            slot_total += 1
            row = names.get(target)
            if row and row["name"]:
                slot_named += 1
            vt_rows.append({
                "vtable_rva": f"0x{piece:x}", "class": cls,
                "class_addr_offset": offset if cls else "",
                "slot": slot, "target_rva": f"0x{target:x}",
                "target_state": state, "method": row["name"] if row else "",
                "signature": row["signature"] if row else "",
                "library": libraries.get(target, ("", ""))[0]})

    with OUT_VTABLES.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.carve relate - per-slot join "
                 "of retail-vtables + vtable_slots + names + NH3API classes.\n")
        for prov in common.provenance("homm3.carve.relate"):
            fh.write(prov + "\n")
        fh.write("# class: NH3API vftable-address label (unverified; "
                 "class_addr_offset>0 means NH3API's address lands that many\n"
                 "# bytes INTO our piece - our cut evidence starts it earlier;"
                 " -1/blank = no NH3API label). method/target are retail.\n")
        writer = csv.DictWriter(fh, fieldnames=list(vt_rows[0].keys()))
        writer.writeheader()
        writer.writerows(vt_rows)

    print(f"[carve relate] {len(sym_rows)} functions -> {OUT_SYMBOLS.name}: "
          f"{named} named, {with_lib} library-proven, {in_vtable} are vtable "
          "slot targets")
    print(f"[carve relate] {len(vt_count)} vtables, {slot_total} slots -> "
          f"{OUT_VTABLES.name}: {labeled} vtables class-labeled, "
          f"{slot_named} slots carry a method name")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
