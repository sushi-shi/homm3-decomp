#!/usr/bin/env python3
"""homm3.analysis.ordermap - align a TU's Dreamcast roster onto its x86 carve.

The skill's Locate step is an order-map: VC6 emits a TU's functions in source
order, the linker lays the .obj down contiguously, so the DC roster sorted by
source line and the x86 carve sorted by RVA are the SAME SEQUENCE - except that
the two builds inline different callees, so the DC side has extra rows the x86
side never emitted (and, rarely, the reverse).

Done by hand that is index-counting with a ruler, and it breaks at the first
skipped row. This does it as a sequence alignment: a monotonic matching that
may skip DC rows, scored on size plausibility (SH4 -> x86 lands in ~0.3-2.5x),
pinned wherever an existing claim already proves a pairing.

IT PROVES NOTHING ON ITS OWN. The skill requires body evidence per pairing -
strings, imports, claimed-callee names, address-takes, arity. This narrows 40
candidates to one hypothesis each so that evidence has something to confirm or
refute, and it says how confident each pairing is so the weak ones get checked
first. Read-only; writes no source.
"""
from __future__ import annotations

import argparse
import csv
import math
import sys

from homm3.core import common

#: SH4 -> x86 size ratio the skill records as plausible. Outside this a
#: pairing is not impossible, just unsupported by size alone.
RATIO_LO, RATIO_HI = 0.3, 2.5
#: cost charged for leaving a DC row unpaired (inlined away, or absent from
#: this build). Low enough that skipping beats a wildly implausible pairing.
SKIP_COST = 1.15


def _repo():
    return common.HOMM3_DIR


def dc_roster(unit: str):
    """DC rows for <unit>.obj that come from <unit>.cpp itself, source order.

    The `file` column is what separates real TU rows from header and template
    attributions - a row compiled out of a .h belongs to whatever included it,
    not to this TU's emission sequence.
    """
    path = _repo() / "evidence/dreamcast/functions.csv"
    out = []
    with path.open() as fh:
        for r in csv.reader(fh):
            if len(r) < 11 or r[4] != f"{unit}.obj":
                continue
            if not r[5].lower().replace("\\", "/").endswith(f"/{unit}.cpp"):
                continue
            try:
                out.append({"off": int(r[0], 16), "size": int(r[1]),
                            "kind": r[2], "name": r[3], "line": int(r[6]),
                            "args": int(r[9])})
            except ValueError:
                continue
    out.sort(key=lambda d: d["off"])
    return out


def claimed_rvas():
    """{rva: label} for every RVA the baseline already claims."""
    path = _repo() / "config/match_baseline.tsv"
    out = {}
    if not path.is_file():
        return out
    for line in path.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        p = line.split("\t")
        if len(p) >= 6:
            try:
                out[int(p[5], 16)] = p[1]
            except ValueError:
                pass
    return out


def carve_span(unit: str):
    """x86 in-span carve rows for `unit`, address order."""
    path = _repo() / "evidence/link-order/functions.tsv"
    out, hdr = [], None
    for line in path.read_text().splitlines():
        if line.startswith("#"):
            continue
        p = line.split("\t")
        if hdr is None:
            hdr = p
            continue
        r = dict(zip(hdr, p))
        if r.get("owner_or_bracket") != unit or r.get("relation") != "in-span":
            continue
        try:
            out.append({"rva": int(r["rva"], 16), "size": int(r["size"]),
                        "label": r.get("label", "")})
        except ValueError:
            continue
    out.sort(key=lambda d: d["rva"])
    return out


def pair_cost(dc, x86):
    """Cost of pairing one DC row with one x86 row. Lower is better."""
    if not dc["size"] or not x86["size"]:
        return SKIP_COST * 2
    ratio = x86["size"] / dc["size"]
    # |log ratio| is symmetric in over- and under-shoot, which is what we want:
    # 2x too big and 2x too small are equally weak evidence.
    cost = abs(math.log(ratio / math.sqrt(RATIO_LO * RATIO_HI)))
    if not RATIO_LO <= ratio <= RATIO_HI:
        cost += 1.0
    return cost


def align(dc, x86, anchors):
    """Monotonic alignment of x86 rows onto DC rows, DC rows skippable.

    anchors: {x86_index: dc_index} pairings held fixed (from existing claims).
    Returns [(dc_index or None, x86_index)] in x86 order.
    """
    n, m = len(x86), len(dc)
    if not n or not m:
        return [(None, j) for j in range(n)]
    inf = float("inf")
    # best[j][i] = cost of placing x86[0..j-1] within dc[0..i-1]
    best = [[inf] * (m + 1) for _ in range(n + 1)]
    back = [[None] * (m + 1) for _ in range(n + 1)]
    for i in range(m + 1):
        best[0][i] = SKIP_COST * i  # leading DC rows skipped
    for j in range(n):
        for i in range(m):
            if best[j][i] == inf:
                continue
            # skip dc[i]
            if best[j][i] + SKIP_COST < best[j][i + 1]:
                best[j][i + 1] = best[j][i] + SKIP_COST
                back[j][i + 1] = ("skip", j, i)
            # pair x86[j] with dc[i]
            forced = anchors.get(j)
            if forced is not None and forced != i:
                continue
            if forced is None and i in anchors.values():
                continue  # an anchored DC row is reserved
            c = best[j][i] + pair_cost(dc[i], x86[j])
            if c < best[j + 1][i + 1]:
                best[j + 1][i + 1] = c
                back[j + 1][i + 1] = ("pair", j, i)
    # finish: trailing DC rows may be skipped
    end_i, end_cost = None, inf
    for i in range(m + 1):
        c = best[n][i] + SKIP_COST * (m - i)
        if c < end_cost:
            end_cost, end_i = c, i
    out, j, i = [], n, end_i
    while j > 0:
        step = back[j][i]
        if step is None:
            out.append((None, j - 1))
            j -= 1
            continue
        kind, pj, pi = step
        if kind == "pair":
            out.append((pi, pj))
            j, i = pj, pi
        else:
            i = pi
    out.reverse()
    return out


def _epilogue_ret(rva: int):
    """Bytes popped by the function's terminal `ret N`, or None.

    Reads the disassembly the sema layer already produces; no compile.
    """
    import re
    import subprocess
    try:
        out = subprocess.run(
            [sys.executable, "-m", "homm3.sema", "disasm", f"0x{rva:x}"],
            capture_output=True, text=True, timeout=120,
            cwd=str(_repo() / "scripts")).stdout
    except (subprocess.SubprocessError, OSError):
        return None
    last = None
    for m in re.finditer(r"^\s*ret(?:\s+(0x[0-9a-f]+|\d+))?\s*$",
                         out, re.M | re.I):
        last = int(m.group(1), 0) if m.group(1) else 0
    return last


def _predict_ret(dc):
    """Stack bytes a VC6 callee pops, from the DC argument count.

    thiscall members take `this` in ecx and the rest on the stack; /Gr makes
    free functions fastcall, so their first two integer args go in ecx/edx.
    A SCREEN, not a proof: it assumes every argument is one 4-byte slot, so a
    by-value struct, an __int64, or a hidden return-UDT pointer will make a
    correct pairing look wrong. Disagreement means look, not reject.
    """
    args = dc["args"]
    if "::" in dc["name"]:
        return 4 * max(0, args - 1)
    return 4 * max(0, args - 2)


def run(unit: str, show_skipped: bool, no_arity: bool = False) -> int:
    dc, x86 = dc_roster(unit), carve_span(unit)
    if not dc:
        print(f"[ordermap] no DC roster rows for {unit}.obj from {unit}.cpp")
        return 1
    if not x86:
        print(f"[ordermap] no in-span carve rows for {unit} - is the "
              "link-order bracket stale? regenerate with "
              "`python -m homm3.analysis.link_order`")
        return 1
    claims = claimed_rvas()

    # Anchors: an x86 row the baseline already claims, whose DC counterpart is
    # identifiable by name. These pin the alignment and are why it survives
    # long runs of skipped rows.
    anchors, anchor_note = {}, []
    for j, x in enumerate(x86):
        label = claims.get(x["rva"])
        if not label:
            continue
        for i, d in enumerate(dc):
            tail = d["name"].split("::")[-1]
            if tail and tail in label:
                anchors[j] = i
                anchor_note.append(f"0x{x['rva']:06x} = {d['name']}")
                break

    print(f"[ordermap] {unit}: {len(dc)} DC row(s) from {unit}.cpp, "
          f"{len(x86)} x86 carve row(s) in span, {len(anchors)} anchor(s)")
    for a in anchor_note:
        print(f"           anchor {a}")
    print()

    pairs = align(dc, x86, anchors)
    used = {i for i, _ in pairs if i is not None}
    agree = disagree = 0
    print(f"{'x86 rva':>10}{'B':>7}  {'DC name':<42}{'dcB':>6}{'x/d':>6}"
          f"{'ar':>4}  {'ret':>10}  confidence")
    for i, j in pairs:
        x = x86[j]
        mark = "*" if x["rva"] in claims else " "
        if i is None:
            print(f"{mark}0x{x['rva']:06x}{x['size']:>7}  "
                  f"{'(no DC row)':<42}{'':>6}{'':>6}{'':>4}  {'':>10}  "
                  f"UNPAIRED - {x['label'][:26]}")
            continue
        d = dc[i]
        ratio = x["size"] / d["size"] if d["size"] else 0
        want = _predict_ret(d)
        got = None if no_arity else _epilogue_ret(x["rva"])
        if got is None:
            ret_col, ret_tag = "-", ""
        elif got == want:
            ret_col, ret_tag = f"{got}=={want}", " ret-ok"
            agree += 1
        else:
            ret_col, ret_tag = f"{got}!={want}", " RET-MISMATCH"
            disagree += 1
            # The usual cause is two adjacent rows sharing a name - overloads
            # the size score cannot tell apart. Name the ones whose arity DOES
            # fit, so the resolution is a lookup rather than a re-derivation.
            alt = [f"{k['name'].split('::')[-1]}/{k['size']}B@{k['line']}"
                   for k in dc if k is not d and _predict_ret(k) == got
                   and abs(dc.index(k) - i) <= 3]
            if alt:
                ret_tag += "  arity fits: " + ", ".join(alt[:3])
        if j in anchors:
            conf = "ANCHOR (claimed)"
        elif RATIO_LO <= ratio <= RATIO_HI:
            conf = "size-plausible"
        else:
            conf = "WEAK - size implausible"
        print(f"{mark}0x{x['rva']:06x}{x['size']:>7}  {d['name'][:42]:<42}"
              f"{d['size']:>6}{ratio:>6.2f}{d['args']:>4}  {ret_col:>10}  "
              f"{conf}{ret_tag}")
    if agree or disagree:
        print(f"\narity screen: {agree} agree, {disagree} disagree. Each "
              "agreement is an independent numeric\nprediction (bytes the "
              "callee pops, from the DC argument count) that came true; a\n"
              "long run of them over a contiguous segment is what the skill "
              "calls an exhaustive\norder-map. Disagreements are not "
              "refutations on their own - a by-value struct, an\n__int64 or a "
              "hidden return-UDT pointer all break the one-arg-one-slot "
              "assumption.")

    skipped = [d for i, d in enumerate(dc) if i not in used]
    print(f"\n{len(skipped)} DC row(s) unpaired - inlined away on x86, or "
          "outside this bracket:")
    if show_skipped:
        for d in skipped:
            print(f"    {d['name'][:56]:<56}{d['size']:>6} B  line "
                  f"{d['line']}")
    else:
        print("    (pass --skipped to list them)")
    print("\nSize agreement is a SCREEN, not proof. Confirm every pairing "
          "with body evidence\nbefore claiming: arity (`ret N` vs the DC "
          "argument count above) is the highest-\nyield check there is; then "
          "strings, imports, claimed-callee names, address-takes.")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="python3 -m homm3.analysis.ordermap")
    ap.add_argument("unit", help="TU name, e.g. spells")
    ap.add_argument("--skipped", action="store_true",
                    help="list the DC rows left unpaired")
    ap.add_argument("--no-arity", action="store_true",
                    help="skip the ret-immediate screen (faster)")
    a = ap.parse_args(argv)
    return run(a.unit, a.skipped, a.no_arity)


if __name__ == "__main__":
    sys.exit(main())
