#!/usr/bin/env python3
"""homm3.carve.dcmap - transfer Dreamcast CodeView names by link order.

The Dreamcast port's CodeView dump proves names, signatures, and FILE:LINE
for 9,684 procs - but for a *different pressing* (RoE sources, SH4 code), so
no byte identity can carry them over. What does carry across pressings is
LINK ORDER: measured here, 95% of the functions NH3API already names for our
image appear in the SAME relative order in the Dreamcast build (LIS over 588
unique-name anchor pairs). Both builds evidently linked their objects in the
same project order, exactly as the HD sibling build did.

That order is the transfer channel, in three passes:

  pass 1  anchor every qualified name that is UNIQUE on both sides
          (our hd-crossbuild/nh3api-named entries vs Dreamcast procs), then
          keep only the longest monotone subset (LIS) - off-backbone pairs
          are tiny COMDAT-style dtors that genuinely move between builds.
  pass 2  names that are ambiguous globally (overloads, shared dtor names)
          are re-anchored INSIDE the bracket between their resolved
          neighbors, where a single Dreamcast proc of that name remains -
          unique-in-bracket, monotonicity re-asserted. (The same idea as
          hdmap's bracketed pass.)
  pass 3  between two adjacent anchors, if our carve holds EXACTLY as many
          entries as the Dreamcast build holds procs, the bracket aligns
          1:1 in address order ("incremental rva"). An agreement gate
          rejects the whole bracket if any already-named entry inside would
          align with a differently-named proc - insertions or reorders
          (Complete added sources RoE lacks) then fail the count instead of
          shifting names silently.

The result is `evidence/retail-dc-name-map.csv`. Rows carry a `role`:
  anchor-global / anchor-bracket   the correlation skeleton (already named)
  corroborates                     pass-3 alignment agrees with the known name
  linkorder                        a NEW name for a function nothing else
                                   names - confidence `linkorder-candidate`,
                                   the weakest tier we admit: order evidence
                                   only, from another pressing.

Sizes are reported (SH4 Cb vs our x86 bytes) but never gate: cross-ISA
codegen makes them a sanity signal, not evidence.
"""
from __future__ import annotations

import bisect
import csv
import re
import sys
from collections import Counter, defaultdict

from homm3.carve import common
from homm3.carve.names import DUMP, Dump

OUT = common.EVIDENCE_DIR / "retail-dc-name-map.csv"
HD_MAP = common.EVIDENCE_DIR / "retail-hd-name-map.csv"
NAMES = common.EVIDENCE_DIR / "retail-function-names.csv"


def load_csv(path):
    if not path.is_file():
        return []
    with path.open() as fh:
        return list(csv.DictReader(
            line for line in fh if not line.startswith("#")))


def lis_pairs(pairs):
    """The longest subset of (a, b) pairs monotone in both coordinates."""
    pairs = sorted(pairs)
    links, index = [None] * len(pairs), []
    for i, (_a, b) in enumerate(pairs):
        j = bisect.bisect_left([pairs[k][1] for k in index], b)
        links[i] = index[j - 1] if j else None
        if j == len(index):
            index.append(i)
        else:
            index[j] = i
    keep = set()
    node = index[-1] if index else None
    while node is not None:
        keep.add(pairs[node])
        node = links[node]
    return keep


def main(argv=None) -> int:
    # our side: every entry with an rva-anchored external name
    ours = {}
    for r in load_csv(HD_MAP):
        if r["our_state"] == "entry":
            ours[int(r["rva"], 16)] = r["name"]
    for r in load_csv(NAMES):
        if r["carve_state"] == "entry" and "nh3api" in r["sources"]:
            ours.setdefault(int(r["rva"], 16), r["name"])
    if not ours:
        common.die("no named entries - run hdmap/names first")

    functions = {int(r["rva"], 16): int(r["size"]) for r in common.read_tsv(
        common.need(common.CARVE_DIR / "functions.tsv", "audit"))}
    entries = sorted(functions)

    print("[carve dcmap] parsing Dreamcast CodeView dump ...", flush=True)
    dump = Dump(DUMP.read_text(errors="replace"))
    procs = sorted((p for p in dump.procs if p["cb"] > 0),
                   key=lambda p: p["offset"])
    offsets = [p["offset"] for p in procs]
    base = {id(p): re.sub(r"@\d+$", "", p["name"]) for p in procs}
    by_name = defaultdict(list)
    for p in procs:
        by_name[base[id(p)]].append(p)

    stats = {"our-named": len(ours), "dc-procs": len(procs)}

    # pass 1: globally unique name pairs, order-gated
    our_count = Counter(ours.values())
    anchors = {}
    for rva, name in ours.items():
        if our_count[name] == 1 and len(by_name[name]) == 1:
            anchors[rva] = by_name[name][0]
    stats["pass1-unique"] = len(anchors)
    keep = lis_pairs([(rva, p["offset"]) for rva, p in anchors.items()])
    anchors = {rva: p for rva, p in anchors.items()
               if (rva, p["offset"]) in keep}
    stats["pass1-anchors"] = len(anchors)
    stats["lis-demoted"] = stats["pass1-unique"] - len(anchors)

    # pass 2: ambiguous names, unique inside their bracket, to fixpoint
    added = True
    while added:
        added = False
        anchor_rvas = sorted(anchors)
        for rva in sorted(set(ours) - set(anchors)):
            i = bisect.bisect_left(anchor_rvas, rva)
            lo = anchors[anchor_rvas[i - 1]]["offset"] if i else -1
            hi = (anchors[anchor_rvas[i]]["offset"]
                  if i < len(anchor_rvas) else 1 << 60)
            if hi <= lo:
                continue
            hits = [p for p in by_name[ours[rva]] if lo < p["offset"] < hi]
            if len(hits) == 1:
                anchors[rva] = hits[0]
                anchor_rvas = sorted(anchors)
                added = True
    stats["pass2-anchors"] = len(anchors) - stats["pass1-anchors"]
    pairs = [(rva, p["offset"]) for rva, p in anchors.items()]
    if len(lis_pairs(pairs)) != len(pairs):
        common.die("pass 2 broke monotonicity - matcher defect")

    # pass 3: equal-count brackets, 1:1 in order, agreement-gated
    backbone = sorted((rva, anchors[rva]) for rva in anchors)
    rows = []
    for rva, p in backbone:
        rows.append((rva, p, "anchor-global"
                     if our_count[ours[rva]] == 1
                     and len(by_name[ours[rva]]) == 1 else "anchor-bracket"))
    stats.update({"brackets-accepted": 0, "brackets-gated": 0,
                  "brackets-mismatched": 0})
    for (r1, p1), (r2, p2) in zip(backbone, backbone[1:]):
        our_gap = entries[bisect.bisect_right(entries, r1):
                          bisect.bisect_left(entries, r2)]
        dc_gap = procs[bisect.bisect_right(offsets, p1["offset"]):
                       bisect.bisect_left(offsets, p2["offset"])]
        if not our_gap or len(our_gap) != len(dc_gap):
            if our_gap or dc_gap:
                stats["brackets-mismatched"] += 1
            continue
        if any(ours.get(orva) and ours[orva] != base[id(p)]
               for orva, p in zip(our_gap, dc_gap)):
            stats["brackets-gated"] += 1
            continue
        stats["brackets-accepted"] += 1
        for orva, p in zip(our_gap, dc_gap):
            rows.append((orva, p, "corroborates" if orva in ours
                         else "linkorder"))

    roles = Counter(role for _r, _p, role in rows)
    rows.sort(key=lambda t: t[0])
    out_rows = []
    for rva, p, role in rows:
        out_rows.append({
            "rva": f"0x{rva:x}", "size": functions[rva], "role": role,
            "name": base[id(p)], "signature": dump.signature(p),
            "dc_module": p["module"], "dc_offset": f"0x{p['offset']:x}",
            "dc_cb": p["cb"],
            "source": f"{p['file']}:{p['line']}" if p["file"] else ""})

    with OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.carve dcmap - Dreamcast "
                 "CodeView names transferred by link order.\n")
        for prov in common.provenance("homm3.carve.dcmap"):
            fh.write(prov + "\n")
        fh.write("# The Dreamcast build is ANOTHER PRESSING (RoE sources, "
                 "SH4 code): no address or byte\n"
                 "# evidence crosses over - only link order does. "
                 "role=linkorder rows are NEW names,\n"
                 "# confidence linkorder-candidate (order evidence only); "
                 "anchor/corroborates rows are the\n"
                 "# correlation skeleton over already-named entries.\n")
        writer = csv.DictWriter(fh, fieldnames=list(out_rows[0].keys()))
        writer.writeheader()
        writer.writerows(out_rows)

    print(f"[carve dcmap] {stats['our-named']} named entries vs "
          f"{stats['dc-procs']} Dreamcast procs")
    for key in ("pass1-unique", "lis-demoted", "pass1-anchors",
                "pass2-anchors", "brackets-accepted", "brackets-gated",
                "brackets-mismatched"):
        print(f"  {key}: {stats[key]}")
    print(f"  roles: " + ", ".join(f"{r}={n}" for r, n in roles.most_common()))
    print(f"  -> {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
