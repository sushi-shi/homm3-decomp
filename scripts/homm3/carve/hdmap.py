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

Matches are accepted only when the search finds exactly ONE site in our .text
and that site is a carved function ENTRY; everything else is reported, not
silently used. Output: config/retail-hd-name-map.csv.

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

    rows = []
    stats = {"candidates": 0, "too-small": 0, "too-masked": 0, "no-match": 0,
             "ambiguous": 0, "matched": 0, "non-entry": 0}
    for va in sorted(wrappers):
        rva = va - IMAGE_LO
        if not hd_lo <= rva < hd_lo + len(hd):
            continue
        stats["candidates"] += 1
        i = bisect.bisect_right(starts, rva)
        end = starts[i] if i < len(starts) else rva + MAX_BODY
        size = min(end - rva, MAX_BODY)
        if size < MIN_BODY:
            stats["too-small"] += 1
            continue
        blob = hd[rva - hd_lo:rva - hd_lo + size]
        mask = body_mask(blob)
        if size - sum(mask) < MIN_FIXED:
            stats["too-masked"] += 1
            continue
        hits = masked_find(ours, blob, mask)
        if not hits:
            stats["no-match"] += 1
            continue
        if len(hits) > 1:
            stats["ambiguous"] += 1
            continue
        our_rva = our_lo + hits[0]
        entry = wrappers[va][0]
        qualified = (f"{entry['class']}::{entry['name']}"
                     if entry["class"] else entry["name"])
        st = state(our_rva)
        if st != "entry":
            stats["non-entry"] += 1
        else:
            stats["matched"] += 1
        rows.append({
            "rva": f"0x{our_rva:x}", "hd_va": f"0x{va:x}",
            "name": qualified, "signature":
                f"{entry['ret']} {qualified}({entry['args']})".strip(),
            "convention": entry["cc"], "match_bytes": size,
            "fixed_bytes": size - sum(mask), "our_state": st,
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

    print(f"[carve hdmap] {stats['candidates']} NH3API addresses in HD .text; "
          f"{len(rows)} transferred ({stats['matched']} onto carved entries, "
          f"{stats['non-entry']} interior/outside)")
    for key in ("too-small", "too-masked", "no-match", "ambiguous"):
        print(f"  {key}: {stats[key]}")
    print(f"  -> {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
