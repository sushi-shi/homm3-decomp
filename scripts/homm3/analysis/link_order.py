#!/usr/bin/env python3
"""homm3.analysis.link_order - carve each TU's retail extent from the
src/ claims, and attribute the unclaimed functions inside it.

A compiland's functions are emitted in source order and the linker
keeps its contribution contiguous, so a TU's proven `VA()` claims
bracket a retail SPAN [first, last+size). Sorting the spans recovers
retail LINK ORDER, and every carved function inside a span that no
claim owns is attributable to that TU - which is the point: ~750
claims turn into module ownership for thousands of unnamed functions.

HEADER-ORIGIN FUNCTIONS DO NOT ANCHOR. An inline defined in a header
(`E:\\gamedcs\\Button.h:78 button::SetText`) is emitted as a COMDAT by
every TU that uses it; the linker folds the copies and keeps whichever
it likes - or drops it. In the Dreamcast build such a copy sits inside
the using TU's contribution (SetText lands mid-run in button.obj), but
that placement is a property of THAT link, not of the source order, so
counting it would stretch a span into a neighbour. Only claims whose
source file is the unit's own .cpp anchor; header-origin claims are
reported separately as span RESIDENTS.

FINDING (2026-08-04): within the game-object group the retail link
order is EXACTLY ALPHABETICAL by object name - 63 spanned units, zero
inversions. That turns every gap into a named prediction: the TUs that
sort between two spanned neighbours are the only candidates for the
bytes between them.

Outputs (evidence/link-order/):
  units.tsv        per unit: span, anchors, residents, coverage
  order.tsv        units sorted by first_rva, with inter-span gaps
  gaps.tsv         each gap + the alphabetically admissible owners
  attribution.tsv  unclaimed carved functions -> owning unit
  functions.tsv    EVERY game-band function -> its owner or its bracket
  README.md        method, the header-origin rule, and the findings

Every function in the game band is relatable: one inside a span is
owned by that TU; one between spans is owned by the previous TU's tail,
the next TU's head, or an unspanned compiland sorting between them -
a bounded, named candidate set. Ownership here means "the linker placed
it in that TU's contribution", NOT "the .cpp defines it": a header
inline is emitted by every using TU and folded, so it can be resident
in a TU that merely used it.

Run: python3 -m homm3.analysis.link_order
"""
from __future__ import annotations

import bisect
import csv
import re
import sys
from pathlib import Path

from homm3.core import common

SRC = common.HOMM3_DIR / "src"
FUNCTIONS = common.HOMM3_DIR / "config/retail-functions.tsv"
SYMBOLS = common.HOMM3_DIR / "build/gen/symbol_names.csv"
OUT = common.EVIDENCE_DIR / "link-order"

_ORIGIN = re.compile(r"^//\s+(\S+?):(\d+)\s*$")
_VA = re.compile(r"^VA\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)")
_DC_ONLY = re.compile(r"^DC_ONLY\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)")


def parse_unit(path: Path):
    """[(origin_file, lineno, kind, addr, size)] for one carcass TU."""
    entries = []
    origin = None
    for line in path.read_text(errors="replace").splitlines():
        m = _ORIGIN.match(line)
        if m:
            origin = (m.group(1).replace("\\", "/").rsplit("/", 1)[-1],
                      int(m.group(2)))
            continue
        m = _VA.match(line)
        if m and origin:
            entries.append((origin[0], origin[1], "VA",
                            int(m.group(1), 16), int(m.group(2), 0)))
            origin = None
            continue
        m = _DC_ONLY.match(line)
        if m and origin:
            entries.append((origin[0], origin[1], "DC_ONLY",
                            int(m.group(1), 16), int(m.group(2), 0)))
            origin = None
    return entries


def load_functions():
    rows = {}
    for line in FUNCTIONS.open():
        if line.startswith("#") or line.startswith("rva\t"):
            continue
        rva, size = line.split("\t")
        rows[int(rva, 16)] = int(size)
    return rows


def load_labels():
    out = {}
    if not SYMBOLS.is_file():
        return out
    with SYMBOLS.open() as fh:
        for r in csv.DictReader(ln for ln in fh if not ln.startswith("#")):
            if r.get("kind") == "func":
                out[int(r["rva"], 16)] = (r["name"], r.get("provenance", ""))
    return out


def main(argv=None) -> int:
    functions = load_functions()
    labels = load_labels()
    starts = sorted(functions)

    units = {}
    for path in sorted(SRC.glob("*.cpp")):
        unit = path.stem
        entries = parse_unit(path)
        own_cpp = f"{unit}.cpp".lower()
        anchors, residents = [], []
        for origin, _line, kind, addr, size in entries:
            if kind != "VA":
                continue
            (anchors if origin.lower() == own_cpp else residents).append(
                (addr, size, origin))
        if not anchors:
            continue
        first = min(a for a, _s, _o in anchors)
        last_end = max(a + s for a, s, _o in anchors)
        units[unit] = {
            "first": first - common.IMAGE_BASE,
            "end": last_end - common.IMAGE_BASE,
            "anchors": len(anchors),
            "residents": residents,
            "claimed": {a - common.IMAGE_BASE for a, _s, _o in anchors},
        }

    ordered = sorted(units.items(), key=lambda kv: kv[1]["first"])

    # overlap check: a TU's span may not contain another's anchors
    overlaps = []
    for (ua, a), (ub, b) in zip(ordered, ordered[1:]):
        if b["first"] < a["end"]:
            overlaps.append((ua, ub, a["end"] - b["first"]))

    OUT.mkdir(parents=True, exist_ok=True)
    prov = common.provenance("homm3.analysis.link_order")

    attribution = []
    with (OUT / "units.tsv").open("w") as fh:
        fh.write("\n".join(prov) + "\n")
        fh.write("unit\tfirst_rva\tend_rva\tspan\tanchors\tresidents\t"
                 "fns_in_span\tclaimed\tunclaimed\n")
        for unit, info in ordered:
            lo = bisect.bisect_left(starts, info["first"])
            hi = bisect.bisect_left(starts, info["end"])
            inside = starts[lo:hi]
            unclaimed = [r for r in inside if r not in info["claimed"]]
            for rva in unclaimed:
                name, provenance = labels.get(rva, ("", ""))
                attribution.append((rva, functions[rva], unit, name, provenance))
            fh.write(f"{unit}\t0x{info['first']:x}\t0x{info['end']:x}\t"
                     f"{info['end'] - info['first']}\t{info['anchors']}\t"
                     f"{len(info['residents'])}\t{len(inside)}\t"
                     f"{len(info['claimed'])}\t{len(unclaimed)}\n")

    with (OUT / "order.tsv").open("w") as fh:
        fh.write("\n".join(prov) + "\n")
        fh.write("position\tunit\tfirst_rva\tend_rva\tgap_to_next\n")
        for i, (unit, info) in enumerate(ordered):
            gap = (ordered[i + 1][1]["first"] - info["end"]
                   if i + 1 < len(ordered) else 0)
            fh.write(f"{i}\t{unit}\t0x{info['first']:x}\t0x{info['end']:x}\t"
                     f"{gap}\n")

    with (OUT / "attribution.tsv").open("w") as fh:
        fh.write("\n".join(prov) + "\n")
        fh.write("rva\tsize\tunit\tcurrent_label\tprovenance\n")
        for rva, size, unit, name, provenance in sorted(attribution):
            fh.write(f"0x{rva:x}\t{size}\t{unit}\t{name}\t{provenance}\n")

    # gaps: the TUs that sort between two spanned neighbours are the
    # only admissible owners of the bytes between them (link order is
    # alphabetical - asserted below).
    dc_modules = set()
    dc_compile = common.EVIDENCE_DIR / "dreamcast/compile.csv"
    if dc_compile.is_file():
        for line in dc_compile.open():
            if line.startswith("#") or line.startswith("module"):
                continue
            stem = line.split(",")[0].strip()
            if stem.endswith(".obj"):
                dc_modules.add(stem[:-4].lower())
    vendor = set()
    zlib_map = common.HOMM3_DIR / "config/retail-zlib-map.tsv"
    if zlib_map.is_file():
        for line in zlib_map.open():
            if not line.startswith("#") and "\t" in line:
                parts = line.rstrip("\n").split("\t")
                if len(parts) >= 4:
                    vendor.add(parts[3].lower())
    spanned = {u for u, _i in ordered}
    unspanned = sorted(dc_modules - spanned - vendor)
    names = [u for u, _i in ordered]
    alphabetical = names == sorted(names)

    with (OUT / "gaps.tsv").open("w") as fh:
        fh.write("\n".join(prov) + "\n")
        fh.write("after_unit\tbefore_unit\tgap_start\tgap_end\tbytes\t"
                 "fns\tcandidate_owners\n")
        for i, (unit, info) in enumerate(ordered[:-1]):
            nxt_unit, nxt = ordered[i + 1]
            if nxt["first"] <= info["end"]:
                continue
            lo = bisect.bisect_left(starts, info["end"])
            hi = bisect.bisect_left(starts, nxt["first"])
            cands = [m for m in unspanned if unit < m < nxt_unit]
            fh.write(f"{unit}\t{nxt_unit}\t0x{info['end']:x}\t"
                     f"0x{nxt['first']:x}\t{nxt['first'] - info['end']}\t"
                     f"{hi - lo}\t{','.join(cands)}\n")

    # every game-band function: owned (inside a span) or bracketed
    from homm3.match import universe
    category, _sizes = universe.classify()
    span_list = [(i["first"], i["end"], u) for u, i in ordered]
    span_starts = [s0 for s0, _e, _u in span_list]
    band_lo, band_hi = span_list[0][0], span_list[-1][1]
    counts = {"in-span": 0, "bracketed": 0}
    cand_sizes = {}
    with (OUT / "functions.tsv").open("w") as fh:
        fh.write("\n".join(prov) + "\n")
        fh.write("rva\tsize\trelation\towner_or_bracket\tcandidates\t"
                 "label\n")
        for rva in starts:
            if category.get(rva) != "target" or not (band_lo <= rva < band_hi):
                continue
            name = labels.get(rva, ("", ""))[0]
            k = bisect.bisect_right(span_starts, rva) - 1
            if k >= 0 and rva < span_list[k][1]:
                unit = span_list[k][2]
                counts["in-span"] += 1
                fh.write(f"0x{rva:x}\t{functions[rva]}\tin-span\t{unit}\t"
                         f"{unit}\t{name}\n")
                continue
            prev_unit = span_list[k][2] if k >= 0 else ""
            nxt = span_list[k + 1][2] if k + 1 < len(span_list) else ""
            cands = ([prev_unit] if prev_unit else []) \
                + [m for m in unspanned if prev_unit < m < nxt] \
                + ([nxt] if nxt else [])
            counts["bracketed"] += 1
            cand_sizes[len(cands)] = cand_sizes.get(len(cands), 0) + 1
            fh.write(f"0x{rva:x}\t{functions[rva]}\tbracketed\t"
                     f"{prev_unit}..{nxt}\t{','.join(cands)}\t{name}\n")

    covered = sum(i["end"] - i["first"] for _u, i in ordered)
    text_bytes = sum(functions.values())
    print(f"[link_order] {len(ordered)} units spanned, "
          f"{sum(i['anchors'] for _u, i in ordered)} own-cpp anchors, "
          f"{sum(len(i['residents']) for _u, i in ordered)} header-origin "
          "residents (not anchoring)")
    print(f"[link_order] spans cover {covered:,} B of "
          f"{text_bytes:,} carved function bytes "
          f"({100.0 * covered / text_bytes:.1f}%); "
          f"{len(attribution):,} unclaimed functions attributed")
    total = counts["in-span"] + counts["bracketed"]
    print(f"[link_order] game band 0x{band_lo:x}..0x{band_hi:x}: "
          f"{total:,} target functions ALL relatable - "
          f"{counts['in-span']:,} owned by a span, "
          f"{counts['bracketed']:,} bracketed")
    print("[link_order] bracketed candidate-set sizes: "
          + ", ".join(f"{n} cand -> {c:,} fns"
                      for n, c in sorted(cand_sizes.items())[:6]))
    print(f"[link_order] link order alphabetical: {alphabetical}"
          + ("" if alphabetical else " - INVERSIONS PRESENT"))
    if overlaps:
        print(f"[link_order] {len(overlaps)} SPAN OVERLAPS "
              "(a claim is misattributed):")
        for ua, ub, by in overlaps[:10]:
            print(f"    {ua} end overruns {ub} start by {by} B")
    else:
        print("[link_order] no span overlaps - every TU's extent is "
              "disjoint (link order consistent)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
