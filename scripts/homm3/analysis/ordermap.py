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
#: cost of leaving an x86 row with no DC counterpart. Higher than SKIP_COST:
#: an unexplained retail function is a worse outcome than an unused roster
#: row, so the alignment should prefer a weak pairing to giving up.
UNPAIR_COST = 2.5


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
        # leave x86[j] unpaired. Without this the DP is INFEASIBLE whenever a
        # segment holds more x86 rows than DC rows - which happens for real
        # (statics and COMDATs the DC roster does not carry), and made `game`
        # crash outright rather than degrade.
        for i in range(m + 1):
            if best[j][i] == inf or anchors.get(j) is not None:
                continue
            c = best[j][i] + UNPAIR_COST
            if c < best[j + 1][i]:
                best[j + 1][i] = c
                back[j + 1][i] = ("drop", j, i)
    # finish: trailing DC rows may be skipped
    end_i, end_cost = None, inf
    for i in range(m + 1):
        c = best[n][i] + SKIP_COST * (m - i)
        if c < end_cost:
            end_cost, end_i = c, i
    if end_i is None:              # nothing feasible - report, do not crash
        return [(None, j) for j in range(n)]
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
        elif kind == "drop":
            out.append((None, pj))
            j, i = pj, pi
        else:
            i = pi
    out.reverse()
    return out


def _anchors(dc, x86, claims, anchor_log=False):
    """{x86_index: dc_index} for claims that identify their DC row UNAMBIGUOUSLY.

    An anchor is only worth having if it is certainly right, so this demands
    three things a naive substring match does not:

      * whole-token equality on the method name, not `in`. `game::game`
        appears inside a dozen unrelated labels, and matching loosely pinned
        94 of game's 175 rows to nonsense.
      * one-to-one. Overloads share a method name, so a name matching several
        DC rows (or several claims) identifies none of them.
      * monotonicity. Anchors that cross each other cannot all be right and
        make the alignment infeasible; the longest increasing run is kept.
    """
    import re
    tok = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")
    by_name = {}
    for i, d in enumerate(dc):
        by_name.setdefault(d["name"].split("::")[-1], []).append(i)
    cand = []
    for j, x in enumerate(x86):
        label = claims.get(x["rva"])
        if not label:
            continue
        # A MANGLED label names its method FIRST - `?Method@Class@@...` -
        # so read it there rather than tokenizing the whole decoration.
        # Tokenizing makes every `army::X` label answer to BOTH `X` and
        # the class token `army`, which the roster's own `army::army`
        # constructor row also answers to; the one-to-one test below then
        # throws the anchor away. Measured on army 2026-08-20: 53 valid
        # anchors collapsed to 2, and the verdict with them (79% MIXED
        # against a hand-anchored map that agrees with every claim).
        # Any TU whose constructor is in its own roster has this.
        m = re.match(r"\?([A-Za-z_][A-Za-z_0-9]*)@", label)
        if m and m.group(1) in by_name:
            names = {m.group(1)}
        else:
            # A CLAIMED-but-not-yet-compiled row carries the delinker's
            # working label instead, and that spells the class as a plain
            # `army_Method` prefix - one token to the regex below, so it
            # matches nothing at all. Offer the text after the first
            # underscore as well. (Only claimed rvas reach here, so the
            # suffix is our own claim's name, not a stale guess.)
            names = set(tok.findall(label))
            head, sep, tail = label.partition("_")
            if sep and tail:
                names.add(tail)
        hits = [(n, idxs) for n, idxs in by_name.items() if n in names]
        if len(hits) != 1:
            continue           # names nothing, or names several DC rows
        name, idxs = hits[0]
        if len(idxs) != 1:
            continue           # overloaded - the name does not pick one
        cand.append((j, idxs[0], x["rva"], dc[idxs[0]]["name"]))
    # longest strictly-increasing run in the dc index (patience/LIS, O(n^2) is
    # ample here) - crossing anchors are dropped, not trusted.
    best = []
    for k in range(len(cand)):
        run = max((r for r in best if r and r[-1][1] < cand[k][1]),
                  key=len, default=[])
        best.append(list(run) + [cand[k]])
    keep = max(best, key=len, default=[])
    anchors = {j: i for j, i, _r, _n in keep}
    note = [f"0x{r:06x} = {n}" for _j, _i, r, n in keep] if anchor_log else []
    dropped = len(cand) - len(keep)
    if dropped and anchor_log:
        note.append(f"({dropped} ambiguous or crossing anchor(s) dropped)")
    return anchors, note


_RET = None


def _epilogue_ret(rva: int, size: int):
    """Bytes popped by the function's terminal `ret N`, or None.

    In-process against ONE loaded image. Shelling out to `sema disasm` per
    function re-loads and re-hashes the whole executable each time, which is
    fine for a 40-row unit and times out on a 175-row one - the units this
    tool exists for.
    """
    global _RET
    if _RET is None:
        import re
        # image_text lines are "  <va>: <bytes>\tret\t<imm>" - the mnemonic is
        # tab-delimited and the line STARTS with an address, so this cannot be
        # anchored at ^.
        _RET = re.compile(r"\tret\t\s*(0x[0-9a-f]+|\d+)?\s*$", re.M | re.I)
    try:
        from homm3.sema import _asm
        from homm3.sema.context import get_context
        text = _asm.image_text(get_context(), rva, size, f"fn_{rva:x}")
    except (Exception, SystemExit):
        return None
    last = None
    for m in _RET.finditer(text):
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

    anchors, anchor_note = _anchors(dc, x86, claims, anchor_log=True)

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
        got = None if no_arity else _epilogue_ret(x["rva"], x["size"])
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
        rate = 100.0 * agree / (agree + disagree)
        # COVERAGE MATTERS AS MUCH AS THE RATE. A unit where the aligner
        # explained 5 of 27 rows can still agree on all five; that is a map of
        # almost nothing, not a good map. resourcemanager scored 100% on five
        # pairings while leaving 22 unexplained.
        covered = 100.0 * (agree + disagree) / len(x86) if x86 else 0.0
        if covered < 60:
            verdict = (f"THIN - only {covered:.0f}% of the span's functions "
                       "got a pairing at all, so this rate is computed on a "
                       "handful of rows. The unpaired ones are the work, and "
                       "the map does not address them.")
        elif rate >= 90:
            verdict = ("USABLE - a run this clean is the exhaustive order-map "
                       "the skill accepts as proof.")
        elif rate >= 70:
            verdict = ("MIXED - the spine is probably right but individual "
                       "rows are not. Claim only ret-ok rows.")
        else:
            verdict = ("DO NOT CLAIM FROM THIS MAP. Agreement this low is "
                       "near chance for common ret values, so the alignment "
                       "is not carrying real information. Usual causes: many "
                       "overloads (which no size score can separate), and "
                       "x86 statics the DC roster never had. Anchor it with "
                       "more claims first.")
        print(f"\nVERDICT {rate:.0f}% arity agreement over {covered:.0f}% of the span - {verdict}")
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
