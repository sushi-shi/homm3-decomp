#!/usr/bin/env python3
"""homm3.carve.gametree - materialize the game's source tree, keyed by
RETAIL addresses.

attempt-1 had this carcass too, but its VA() values were another pressing's
(NH3API/HD addresses) or cross-architecture Dreamcast offsets. Here every
address is either OUR pinned image's (via the dcmap correspondence - byte- or
order-evidenced, role recorded) or explicitly absent: a Dreamcast-only proc
keeps its DC coordinates in a comment and never fakes a VA() claim.

The Dreamcast dump supplies the tree itself - 100% of the DC build's real
functions are named (measured: 9,684 procs cover 99.7% of .text; the
remainder is import thunks) - so each module (compiland) becomes one carcass
file listing its functions in link order with original signatures and
FILE:LINE. The dcmap ties supply the retail VAs. Link order (proven
preserved across all three builds) then does one more job: the span between
a module's first and last tied rva is that TU's RETAIL EXTENT, and every
carved retail function inside it that is tied to no DC proc is listed as an
UNMATCHED RESIDENT - Complete-era additions to that TU, named where our
naming layer knows them. That block is the per-TU work list for the decomp.

Outputs (GENERATED, regenerable):
  evidence/game-tree/<source>.cpp.txt   one carcass per compiland
  evidence/game-tree/INDEX.md           module -> extent/coverage table
  evidence/retail-game-tree.csv         flat master: one row per DC proc
"""
from __future__ import annotations

import csv
import re
import shutil
import sys
from bisect import bisect_left, bisect_right
from collections import Counter, defaultdict

from homm3.carve import common
from homm3.carve.names import DUMP, Dump

TREE_DIR = common.EVIDENCE_DIR / "game-tree"
CSV_OUT = common.EVIDENCE_DIR / "retail-game-tree.csv"
DC_MAP = common.EVIDENCE_DIR / "retail-dc-name-map.csv"
SYMBOLS = common.EVIDENCE_DIR / "retail-symbols.csv"
IMAGE_BASE = 0x400000


def load_csv(path):
    if not path.is_file():
        return []
    with path.open() as fh:
        return list(csv.DictReader(
            line for line in fh if not line.startswith("#")))


def main(argv=None) -> int:
    print("[carve gametree] parsing Dreamcast CodeView dump ...", flush=True)
    dump = Dump(DUMP.read_text(errors="replace"))
    procs = sorted((p for p in dump.procs if p["cb"] > 0),
                   key=lambda p: p["offset"])

    tie = {}   # dc_offset -> (rva, role)
    for r in load_csv(DC_MAP):
        tie[int(r["dc_offset"], 16)] = (int(r["rva"], 16), r["role"])

    functions = {int(r["rva"], 16): int(r["size"]) for r in common.read_tsv(
        common.need(common.CARVE_DIR / "functions.tsv", "audit"))}
    entries = sorted(functions)
    named = {int(r["rva"], 16): r for r in load_csv(SYMBOLS)}

    # group procs by module, keep DC link order (address order)
    modules = []
    by_module = defaultdict(list)
    for p in procs:
        if p["module"] not in by_module:
            modules.append(p["module"])
        by_module[p["module"]].append(p)
    modules.sort(key=lambda m: by_module[m][0]["offset"])

    def main_source(plist):
        files = Counter(p["file"] for p in plist
                        if p["file"] and not p["file"].lower().endswith(".h"))
        return files.most_common(1)[0][0] if files else ""

    # per-module retail extents from the ties (monotone by construction)
    extents = {}
    for module in modules:
        rvas = [tie[p["offset"]][0] for p in by_module[module]
                if p["offset"] in tie]
        if rvas:
            if rvas != sorted(rvas):
                common.die(f"{module}: tied rvas not monotone - "
                           "correspondence defect")
            extents[module] = (rvas[0], rvas[-1])

    if TREE_DIR.exists():
        shutil.rmtree(TREE_DIR)
    TREE_DIR.mkdir(parents=True)

    used_names = {}
    csv_rows = []
    index_rows = []
    stats = Counter()
    claimed = set()  # rvas tied to some proc, for resident detection
    for _off, (rva, _role) in tie.items():
        claimed.add(rva)

    for module in modules:
        plist = by_module[module]
        source = main_source(plist)
        base = source.rsplit("\\", 1)[-1].rsplit("/", 1)[-1] or module
        stem = re.sub(r"[^A-Za-z0-9_.-]", "_", base)
        if stem.lower() in used_names:
            stem = f"{module.rsplit('.', 1)[0]}-{stem}"
        used_names[stem.lower()] = module

        tied = [(p, *tie[p["offset"]]) for p in plist if p["offset"] in tie]
        lines = [
            f"// {source or module} - materialized from the Dreamcast "
            "CodeView dump;",
            "// RVA()/VA() claims are for the PINNED RETAIL IMAGE "
            "(sha 057c9d88..., base 0x400000)",
            f"// via evidence/retail-dc-name-map.csv. module {module}, "
            f"{len(plist)} procs, {len(tied)} retail-tied.",
        ]
        if module in extents:
            lo, hi = extents[module]
            hi_end = hi + functions.get(hi, 0)
            lines.append(f"// retail extent 0x{lo:x}..0x{hi_end:x} "
                         f"(VA 0x{IMAGE_BASE + lo:x}..0x"
                         f"{IMAGE_BASE + hi_end:x})")
        lines.append("")

        for p in plist:
            sig = dump.signature(p) or p["name"]
            where = (f"{p['file']}:{p['line']}" if p["file"]
                     else f"{module}")
            if p["offset"] in tie:
                rva, role = tie[p["offset"]]
                size = functions.get(rva, 0)
                stats[role] += 1
                lines.append(f"// {where}")
                lines.append(f"VA(0x{IMAGE_BASE + rva:08x}, 0x{size:X})"
                             f"  // {role}, dc 0x{p['offset']:x}")
            else:
                rva, role, size = None, "dc-only", 0
                stats["dc-only"] += 1
                lines.append(f"// {where}")
                lines.append(f"// DC_ONLY(0x{p['offset']:x}, cb "
                             f"0x{p['cb']:X}) - no retail tie yet")
            lines.append(sig)
            lines.append("{")
            lines.append("    // @stub")
            lines.append("}")
            lines.append("")
            csv_rows.append({
                "module": module, "source": where,
                "name": p["name"], "signature": sig,
                "dc_offset": f"0x{p['offset']:x}", "dc_cb": p["cb"],
                "rva": f"0x{rva:x}" if rva is not None else "",
                "size": size or "", "tie": role})

        residents = []
        if module in extents:
            lo, hi = extents[module]
            for rva in entries[bisect_left(entries, lo):
                               bisect_right(entries, hi)]:
                if rva not in claimed:
                    residents.append(rva)
        if residents:
            stats["residents"] += len(residents)
            lines.append("// --- unmatched retail residents in this "
                         "extent (Complete-era or untied) ---")
            for rva in residents:
                row = named.get(rva, {})
                lines.append(f"//   0x{IMAGE_BASE + rva:08x} "
                             f"size {functions[rva]:5}  "
                             f"{row.get('name', '?')} "
                             f"[{row.get('tier', '?')}]")
            lines.append("")

        (TREE_DIR / f"{stem}.txt").write_text("\n".join(lines) + "\n")
        index_rows.append((module, stem, source, len(plist), len(tied),
                           len(residents),
                           f"0x{extents[module][0]:x}..0x{extents[module][1]:x}"
                           if module in extents else "-"))

    with CSV_OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.carve gametree - the game "
                 "source tree with retail ties.\n")
        for prov in common.provenance("homm3.carve.gametree"):
            fh.write(prov + "\n")
        fh.write("# One row per Dreamcast proc in link order; rva/size are "
                 "the PINNED image's where tied\n# (tie column = dcmap "
                 "role), empty for dc-only rows. ANALYSIS OUTPUT.\n")
        writer = csv.DictWriter(fh, fieldnames=list(csv_rows[0].keys()))
        writer.writeheader()
        writer.writerows(csv_rows)

    index = ["# The game tree (generated - `python3 -m homm3.carve "
             "gametree`)", "",
             "One carcass file per Dreamcast compiland, in link order; "
             "retail ties from", "`evidence/retail-dc-name-map.csv`. "
             "See `evidence/retail-game-tree.csv` for the flat table.", "",
             "| module | file | source | procs | tied | residents | "
             "retail extent |", "|---|---|---|---:|---:|---:|---|"]
    for module, stem, source, nprocs, ntied, nres, ext in index_rows:
        index.append(f"| {module} | {stem}.txt | {source} | {nprocs} | "
                     f"{ntied} | {nres} | {ext} |")
    (TREE_DIR / "INDEX.md").write_text("\n".join(index) + "\n")

    tied_total = sum(v for k, v in stats.items()
                     if k not in ("dc-only", "residents"))
    print(f"[carve gametree] {len(modules)} modules, {len(procs)} procs -> "
          f"{TREE_DIR}")
    print(f"  retail-tied: {tied_total} "
          f"({', '.join(f'{k}={v}' for k, v in sorted(stats.items()) if k not in ('dc-only', 'residents'))})")
    print(f"  dc-only: {stats['dc-only']}; unmatched retail residents "
          f"listed: {stats['residents']}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
