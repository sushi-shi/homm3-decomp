#!/usr/bin/env python3
"""homm3.carve.dcxref - the Dreamcast build's own xref graph, and the
caller-set cross-check against our retail call graph.

The Dreamcast H3.EXE (WinCE SH4 PE, the user's own GD-ROM rip, referenced in
place via $HOMM3_DC_EXE - never copied into the repo) is the very executable
the CodeView dump describes: its debug directory's type-2 entry points at the
`NB11` stream whose extent ends exactly at the file end, and the dump's
module lfo values are offsets into that stream. So every one of the dump's
procs has authoritative bytes here, and a reference graph over them is
symbol-accurate by construction - no name transfer involved.

SH4 codegen makes extraction almost declarative; two channels:

  pool  `MOV.L @(disp8*4, PC), Rn` (0xDnXX) loads a 32-bit literal from the
        function's constant pool; if the literal is the VA of a proc start,
        that is a reference (the following `JSR @Rn` consumes it). The edge
        is attributed to the function containing the INSTRUCTION, never the
        pool slot - pools cluster and cross proc boundaries.
  bsr   `BSR disp12` (0xBxxx), the +-4KB PC-relative direct call.

Output 1: evidence/dc-xref-graph.tsv - who references who, by name, in the
Dreamcast pressing.

Output 2: evidence/retail-dc-xref-check.csv - the cross-pressing caller-set
comparison. For every function tied to a Dreamcast proc (via the dcmap
correspondence, or unique-name match as fallback), compare WHO CALLS IT in
each build, in name space. Shared callers corroborate the identification;
a function whose comparable caller sets are non-empty and DISJOINT is a
likely misattribution (if `a` is called by `b`,`c` on Dreamcast but by
`d`,`e` in retail, one of the names is on the wrong function). Absence of a
single edge is weak evidence - virtual calls are invisible to both static
graphs and inlining differs between compilers - so only disjointness over
comparable callers raises the flag, never a single missing edge.
"""
from __future__ import annotations

import bisect
import csv
import hashlib
import os
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

from homm3.carve import common
from homm3.carve.names import DUMP, Dump

GRAPH_OUT = common.EVIDENCE_DIR / "dc-xref-graph.tsv"
CHECK_OUT = common.EVIDENCE_DIR / "retail-dc-xref-check.csv"
DC_MAP = common.EVIDENCE_DIR / "retail-dc-name-map.csv"
HD_MAP = common.EVIDENCE_DIR / "retail-hd-name-map.csv"
NAMES = common.EVIDENCE_DIR / "retail-function-names.csv"
XREFS = common.HOMM3_DIR / "build/dna/function_xrefs.tsv"


def dc_exe() -> Path:
    env = os.environ.get("HOMM3_DC_EXE")
    candidates = [Path(env)] if env else []
    candidates += [common.HOMM3_DIR.parent / "orig/dreamcast/H3.EXE"]
    for path in candidates:
        if path.is_file():
            return path
    common.die("Dreamcast H3.EXE not found. Extract it from the GD-ROM rip "
               "(track03 ISO root) and point $HOMM3_DC_EXE at it")


def text_section(path: Path):
    """(text bytes, VA of .text incl. image base, file size check data)."""
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    machine, nsec = struct.unpack_from("<HH", data, pe + 4)
    if machine != 0x1A6:
        common.die(f"{path}: machine 0x{machine:x}, expected SH4 (0x1a6)")
    osz = struct.unpack_from("<H", data, pe + 20)[0]
    image_base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    text = None
    for i in range(nsec):
        off = pe + 24 + osz + i * 40
        name = data[off:off + 8].rstrip(b"\0").decode("latin-1")
        vs, va, rs, ro = struct.unpack_from("<4I", data, off + 8)
        if name == ".text":
            text = (data[ro:ro + min(vs, rs)], image_base + va)
    if text is None:
        common.die(f"{path}: no .text section")

    # gate: the CodeView stream this exe carries must be the dump's - the
    # debug directory's type-2 entry ends exactly at the file end (NB11)
    ddir_rva = struct.unpack_from("<II", data, pe + 24 + 96 + 6 * 8)[0]
    for i in range(4):
        raw = _rva_to_raw(data, pe, nsec, osz, ddir_rva) + i * 28
        typ, size, _rva, ptr = struct.unpack_from("<IIII", data, raw + 12)
        if typ == 2:
            if data[ptr:ptr + 4] != b"NB11" or ptr + size != len(data):
                common.die(f"{path}: CodeView stream does not match the "
                           "dump layout (NB11 at tail expected)")
            return text
    common.die(f"{path}: no CodeView debug entry")


def _rva_to_raw(data, pe, nsec, osz, rva):
    for i in range(nsec):
        off = pe + 24 + osz + i * 40
        vs, va, rs, ro = struct.unpack_from("<4I", data, off + 8)
        if va <= rva < va + max(vs, rs):
            return ro + (rva - va)
    common.die(f"rva 0x{rva:x} in no section")


def load_csv(path):
    if not path.is_file():
        return []
    with path.open() as fh:
        return list(csv.DictReader(
            line for line in fh if not line.startswith("#")))


def build_graph(text, text_va, procs):
    starts = [p["offset"] for p in procs]
    start_of = {}
    for p in procs:
        start_of.setdefault(p["offset"], p)

    def owner(off):
        i = bisect.bisect_right(starts, off) - 1
        if i >= 0 and starts[i] <= off < starts[i] + procs[i]["cb"]:
            return procs[i]
        return None

    edges = {}
    stats = defaultdict(int)
    for p in procs:
        lo, hi = p["offset"], min(p["offset"] + p["cb"], len(text))
        for at in range(lo, hi - 1, 2):
            w = struct.unpack_from("<H", text, at)[0]
            op = w >> 12
            if op == 0xD:  # MOV.L @(disp,PC),Rn
                pool = ((at + 4) & ~3) + (w & 0xFF) * 4
                if pool + 4 > len(text):
                    continue
                val = struct.unpack_from("<I", text, pool)[0]
                stats["pool-loads"] += 1
                if text_va <= val < text_va + len(text):
                    dst = val - text_va
                    if dst in start_of:
                        stats["pool-edges"] += 1
                        edges.setdefault(
                            (p["offset"], dst), [0, 0])[0] += 1
                    elif owner(dst):
                        stats["pool-interior"] += 1
            elif op == 0xB:  # BSR disp12
                disp = w & 0xFFF
                if disp >= 0x800:
                    disp -= 0x1000
                dst = at + 4 + disp * 2
                if dst in start_of:
                    stats["bsr-edges"] += 1
                    edges.setdefault((p["offset"], dst), [0, 0])[1] += 1
    return edges, start_of, stats


def main(argv=None) -> int:
    exe_path = dc_exe()
    sha = hashlib.sha256(exe_path.read_bytes()).hexdigest()
    print(f"[carve dcxref] DC exe: {exe_path} sha256={sha[:12]}...")
    text, text_va = text_section(exe_path)

    print("[carve dcxref] parsing Dreamcast CodeView dump ...", flush=True)
    dump = Dump(DUMP.read_text(errors="replace"))
    procs = sorted((p for p in dump.procs if p["cb"] > 0),
                   key=lambda p: p["offset"])
    inside = sum(1 for p in procs if p["offset"] + p["cb"] <= len(text))
    if inside < len(procs) * 99 // 100:
        common.die(f"only {inside}/{len(procs)} procs fit .text - "
                   "wrong executable for this dump")

    edges, start_of, stats = build_graph(text, text_va, procs)
    base = {off: re.sub(r"@\d+$", "", p["name"])
            for off, p in start_of.items()}

    dc_callers = defaultdict(set)
    for (src, dst), _counts in edges.items():
        if src != dst:
            dc_callers[dst].add(src)

    rows = []
    for (src, dst), (pool, bsr) in sorted(edges.items()):
        rows.append((f"0x{src:x}", start_of[src]["name"],
                     start_of[src]["module"], f"0x{dst:x}",
                     start_of[dst]["name"], start_of[dst]["module"],
                     pool, bsr))
    common.write_tsv(GRAPH_OUT, "homm3.carve.dcxref",
                     ["src_offset", "src_name", "src_module", "dst_offset",
                      "dst_name", "dst_module", "pool_refs", "bsr_calls"],
                     rows)
    with_callers = len(dc_callers)
    print(f"[carve dcxref] {len(edges)} edges "
          f"({stats['pool-edges']} pool refs, {stats['bsr-edges']} bsr; "
          f"{stats['pool-interior']} interior hits ignored); "
          f"{with_callers}/{len(procs)} procs have callers "
          f"-> {GRAPH_OUT.name}")

    # ---- cross-check: caller sets by name, retail vs Dreamcast ----------
    # our rva <-> dc proc: the dcmap correspondence first, unique-name second
    matched = {}  # our_rva -> dc_offset
    for r in load_csv(DC_MAP):
        matched[int(r["rva"], 16)] = (int(r["dc_offset"], 16), r["role"])
    our_name = {}
    for r in load_csv(HD_MAP):
        if r["our_state"] == "entry":
            our_name[int(r["rva"], 16)] = r["name"]
    for r in load_csv(NAMES):
        if r["carve_state"] == "entry" and "nh3api" in r["sources"]:
            our_name.setdefault(int(r["rva"], 16), r["name"])
    for r in load_csv(DC_MAP):
        our_name.setdefault(int(r["rva"], 16), r["name"])
    name_rvas = defaultdict(list)
    for rva, name in our_name.items():
        name_rvas[name].append(rva)
    dc_by_name = defaultdict(list)
    for off, nm in base.items():
        dc_by_name[nm].append(off)
    for name, rvas in name_rvas.items():
        if len(rvas) == 1 and len(dc_by_name[name]) == 1 \
                and rvas[0] not in matched:
            matched[rvas[0]] = (dc_by_name[name][0], "unique-name")

    retail_callers = defaultdict(set)
    for r in common.read_tsv(XREFS) if XREFS.is_file() else []:
        if r["callers"] == "-":
            continue
        callee = int(r["entry_rva"], 16)
        for c in r["callers"].split(";"):
            caller = int(c, 16)
            if caller != callee:
                retail_callers[callee].add(caller)
    if not retail_callers:
        print("[carve dcxref] no retail xrefs "
              "(build/dna/function_xrefs.tsv) - check skipped")
        return 0

    # comparable = a caller name known on BOTH sides
    dc_names_present = set(dc_by_name)
    check_rows = []
    verdicts = defaultdict(int)
    for rva, (dc_off, via) in sorted(matched.items()):
        name = our_name.get(rva, "")
        dc_set = {base[c] for c in dc_callers.get(dc_off, ())}
        our_set = {our_name[c] for c in retail_callers.get(rva, ())
                   if c in our_name}
        shared = dc_set & our_set
        # discrepancies only count where the other side COULD have seen it
        dc_only = {n for n in dc_set - shared if n in name_rvas}
        our_only = {n for n in our_set - shared if n in dc_names_present}
        if shared:
            verdict = "corroborated"
        elif dc_only and our_only:
            verdict = "disjoint-callers"
        elif dc_only or our_only:
            verdict = "one-sided"
        else:
            verdict = "no-signal"
        verdicts[verdict] += 1
        check_rows.append({
            "rva": f"0x{rva:x}", "name": name, "dc_offset": f"0x{dc_off:x}",
            "match_via": via, "verdict": verdict,
            "shared_callers": len(shared),
            "dc_only": ";".join(sorted(dc_only)[:6]),
            "retail_only": ";".join(sorted(our_only)[:6]),
            "dc_caller_count": len(dc_set),
            "retail_named_caller_count": len(our_set)})

    with CHECK_OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.carve dcxref - caller-set "
                 "comparison, retail vs Dreamcast.\n")
        for prov in common.provenance("homm3.carve.dcxref"):
            fh.write(prov + "\n")
        fh.write("# corroborated: >=1 caller shared by name across the "
                 "pressings. disjoint-callers: both\n# builds show named "
                 "comparable callers and they DISAGREE completely - likely "
                 "name\n# misattribution on one side. one-sided/no-signal: "
                 "not enough overlap to judge\n# (virtual calls and inlining "
                 "hide edges from both static graphs).\n")
        writer = csv.DictWriter(fh, fieldnames=list(check_rows[0].keys()))
        writer.writeheader()
        writer.writerows(check_rows)

    print(f"[carve dcxref] {len(check_rows)} matched functions checked "
          f"-> {CHECK_OUT.name}")
    for verdict, count in sorted(verdicts.items(), key=lambda kv: -kv[1]):
        print(f"  {verdict}: {count}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
