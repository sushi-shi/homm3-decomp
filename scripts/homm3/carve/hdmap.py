#!/usr/bin/env python3
"""homm3.carve.hdmap - transfer NH3API names via the HD Mod sibling build.

NH3API's embedded addresses do not fit our pinned image: measured here, only
11.1% of its 920 call-macro addresses are `E8` call targets in our exe (chance
level), and probes land mid-instruction. Its README says why - the library
targets "the Complete edition with HD Mod by baratorch", and HD Mod ships its
own `_HD3_Data/Heroes3.exe`. In THAT binary 98.3% of the same addresses are
call targets. So NH3API is correct; it just describes a sibling build.

The two builds share source but not layout (same .text virtual size, yet the
bytes diverge from 0x1025 on). What they do share is per-function code: with
absolute operands and rel32 branch displacements masked out, an HD function
body is byte-identical to its counterpart in our image. That makes the
transfer evidence-bearing rather than a guess - the name comes from NH3API,
but the IDENTIFICATION is our own bytes matching theirs, uniquely.

    HD function @ NH3API address --(masked byte identity, unique)--> our rva

Two passes. Pass 1 accepts only a globally unique masked hit. The resulting
map is then order-checked: the builds preserve link order (measured: 816/817
pass-1 pairs monotonic, median neighbor-gap difference 0 bytes), so any pair
breaking monotonicity is a false match to a byte-twin and is demoted. Pass 2
retries every unresolved address INSIDE the bracket between its resolved
neighbors, with shrinking match windows - within a bracket a few fixed bytes
suffice, which recovers small bodies and bodies whose pass-1 window overshot
into a diverging neighbor. Bracketed matches must be unique in-bracket and
keep the whole map monotonic. Output: config/retail-hd-name-map.csv with a
`pass` column (global-unique | bracketed).

The HD executable is the user's own HD Mod download, referenced in place via
$HOMM3_HD_EXE (never copied into the repo).
"""
from __future__ import annotations

import bisect
import csv
import os
import struct
import sys
from pathlib import Path

from homm3.carve import common
from homm3.carve.dna import masked_find

OUT = common.HOMM3_DIR / "config/retail-hd-name-map.csv"
MIN_BODY = 24
MIN_FIXED = 8
MAX_BODY = 256
IMAGE_LO, IMAGE_HI = 0x400000, 0x700000


def hd_exe() -> Path:
    env = os.environ.get("HOMM3_HD_EXE")
    candidates = [Path(env)] if env else []
    candidates += [
        Path.home() / ".cache/homm3-hd/Heroes3.exe",
        common.HOMM3_DIR / "build/carve/hd/Heroes3.exe",
    ]
    for path in candidates:
        if path.is_file():
            return path
    common.die("HD Mod executable not found. Extract it from the HD Mod "
               "installer and point $HOMM3_HD_EXE at "
               "app/_HD3_Data/Heroes3.exe")


def text_section(path: Path):
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    osz = struct.unpack_from("<H", data, pe + 20)[0]
    for i in range(nsec):
        off = pe + 24 + osz + i * 40
        name = data[off:off + 8].rstrip(b"\0").decode("latin-1")
        _vs, va, rs, ro = struct.unpack_from("<4I", data, off + 8)
        if name == ".text":
            return data[ro:ro + rs], va
    common.die(f"{path}: no .text section")


def call_targets(blob: bytes, rva_lo: int) -> set:
    """`E8 rel32` destinations - build-independent function-start evidence."""
    out = set()
    limit = rva_lo + len(blob)
    for i in range(len(blob) - 5):
        if blob[i] == 0xE8:
            target = rva_lo + i + 5 + struct.unpack_from("<i", blob, i + 1)[0]
            if rva_lo <= target < limit:
                out.add(target)
    return out


def body_mask(blob: bytes) -> bytes:
    """Mask what LAYOUT changes between builds: in-image absolute operands and
    rel32 branch/call displacements. What remains is the instruction skeleton."""
    mask = bytearray(len(blob))
    for k in range(len(blob) - 3):
        if IMAGE_LO <= struct.unpack_from("<I", blob, k)[0] < IMAGE_HI:
            mask[k:k + 4] = b"\1\1\1\1"
    k = 0
    while k < len(blob) - 4:
        if blob[k] in (0xE8, 0xE9):
            mask[k + 1:k + 5] = b"\1\1\1\1"
            k += 5
        elif blob[k] == 0x0F and k + 5 < len(blob) and 0x80 <= blob[k + 1] <= 0x8F:
            mask[k + 2:k + 6] = b"\1\1\1\1"
            k += 6
        else:
            k += 1
    return bytes(mask)


def longest_increasing(pairs):
    """Maximal monotonic subset of (hd_rva, our_rva) pairs (patience LIS)."""
    pairs = sorted(pairs)
    tails, links, index = [], [None] * len(pairs), []
    for i, (_hd, our) in enumerate(pairs):
        j = bisect.bisect_left([pairs[k][1] for k in index], our)
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


def bracket_find(ours, blob, mask, lo_off, hi_off):
    """All offsets in [lo_off, hi_off) matching blob under mask (brute)."""
    fixed = [i for i in range(len(blob)) if not mask[i]]
    hits = []
    for start in range(max(lo_off, 0),
                       min(hi_off, len(ours) - len(blob) + 1)):
        if all(ours[start + i] == blob[i] for i in fixed):
            hits.append(start)
            if len(hits) > 2:
                break
    return hits


def main(argv=None) -> int:
    from homm3.carve.names import parse_nh3api

    ours, our_lo = text_section(Path(common.resolve_exe()))
    hd_path = hd_exe()
    hd, hd_lo = text_section(hd_path)
    print(f"[carve hdmap] HD build: {hd_path} ({hd_path.stat().st_size} B)")

    functions = {int(r["rva"], 16): int(r["size"]) for r in common.read_tsv(
        common.need(common.CARVE_DIR / "functions.tsv", "audit"))}
    entries = sorted(functions)

    def state(rva):
        if rva in functions:
            return "entry"
        i = bisect.bisect_right(entries, rva) - 1
        if i >= 0 and entries[i] <= rva < entries[i] + functions[entries[i]]:
            return "interior"
        return "outside"

    wrappers = parse_nh3api()
    hd_calls = call_targets(hd, hd_lo)
    starts = sorted(hd_calls | {va - IMAGE_LO for va in wrappers})

    def windows(rva):
        i = bisect.bisect_right(starts, rva)
        end = starts[i] if i < len(starts) else rva + MAX_BODY
        size = min(end - rva, MAX_BODY)
        sizes = []
        while size >= 12:
            sizes.append(size)
            size = size * 2 // 3
        return sizes

    candidates = [va - IMAGE_LO for va in sorted(wrappers)
                  if hd_lo <= va - IMAGE_LO < hd_lo + len(hd)]
    stats = {"candidates": len(candidates), "pass1": 0, "lis-demoted": 0,
             "bracketed": 0, "unresolved": 0, "non-entry": 0}

    # pass 1: globally unique masked identity
    resolved = {}
    for rva in candidates:
        for size in windows(rva):
            if size < MIN_BODY:
                break
            blob = hd[rva - hd_lo:rva - hd_lo + size]
            mask = body_mask(blob)
            if size - sum(mask) < MIN_FIXED:
                continue
            hits = masked_find(ours, blob, mask)
            if hits and len(hits) == 1:
                resolved[rva] = (our_lo + hits[0], size,
                                 size - sum(mask), "global-unique")
                break
            if hits is not None and len(hits) > 1:
                break  # shrinking only makes it more ambiguous
    stats["pass1"] = len(resolved)

    # order gate: demote pairs that break link-order monotonicity
    keep = longest_increasing([(rva, resolved[rva][0]) for rva in resolved])
    for rva in [r for r in resolved if (r, resolved[r][0]) not in keep]:
        del resolved[rva]
        stats["lis-demoted"] += 1

    # pass 2: bracket search between resolved neighbors, shrinking windows
    changed = True
    while changed:
        changed = False
        anchor_hd = sorted(resolved)
        for rva in candidates:
            if rva in resolved:
                continue
            i = bisect.bisect_left(anchor_hd, rva)
            prev_hd = anchor_hd[i - 1] if i else None
            next_hd = anchor_hd[i] if i < len(anchor_hd) else None
            lo_off = (resolved[prev_hd][0] - our_lo + 1) if prev_hd else 0
            hi_off = (resolved[next_hd][0] - our_lo) if next_hd else len(ours)
            if hi_off <= lo_off:
                continue
            for size in windows(rva):
                blob = hd[rva - hd_lo:rva - hd_lo + size]
                mask = body_mask(blob)
                if size - sum(mask) < 6:
                    continue
                hits = bracket_find(ours, blob, mask, lo_off, hi_off)
                if len(hits) == 1:
                    resolved[rva] = (our_lo + hits[0], size,
                                     size - sum(mask), "bracketed")
                    stats["bracketed"] += 1
                    changed = True
                    break
                if len(hits) > 1:
                    break
            anchor_hd = sorted(resolved)

    stats["unresolved"] = len(candidates) - len(resolved)
    keep = longest_increasing([(rva, resolved[rva][0]) for rva in resolved])
    if len(keep) != len(resolved):
        common.die("bracketed pass broke monotonicity - matcher defect")

    rows = []
    for rva in sorted(resolved):
        our_rva, size, fixed, how = resolved[rva]
        entry = wrappers[IMAGE_LO + rva][0]
        qualified = (f"{entry['class']}::{entry['name']}"
                     if entry["class"] else entry["name"])
        st = state(our_rva)
        if st != "entry":
            stats["non-entry"] += 1
        rows.append({
            "rva": f"0x{our_rva:x}", "hd_va": f"0x{IMAGE_LO + rva:x}",
            "name": qualified, "signature":
                f"{entry['ret']} {qualified}({entry['args']})".strip(),
            "convention": entry["cc"], "match_bytes": size,
            "fixed_bytes": fixed, "pass": how, "our_state": st,
            "evidence": entry["where"]})

    rows.sort(key=lambda r: int(r["rva"], 16))
    with OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.carve hdmap - NH3API names "
                 "transferred from HD Mod's Heroes3.exe by masked byte "
                 "identity.\n")
        for prov in common.provenance("homm3.carve.hdmap"):
            fh.write(prov + "\n")
        fh.write(f"# HD build: {hd_path.name}; match = unique masked hit of "
                 "the HD function body in OUR .text\n"
                 "# (absolute operands + rel32 displacements masked). The "
                 "NAME is NH3API's (external);\n# the IDENTIFICATION is our "
                 "own bytes. our_state=entry means it also lands on a carved "
                 "entry.\n")
        writer = csv.DictWriter(fh, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    on_entry = len(rows) - stats["non-entry"]
    print(f"[carve hdmap] {stats['candidates']} NH3API addresses in HD .text; "
          f"{len(rows)} transferred ({on_entry} onto carved entries, "
          f"{stats['non-entry']} interior/outside)")
    for key in ("pass1", "lis-demoted", "bracketed", "unresolved"):
        print(f"  {key}: {stats[key]}")
    print(f"  -> {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
