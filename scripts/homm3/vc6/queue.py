#!/usr/bin/env python3
"""homm3.vc6.queue - the tree-wide wall census (`homm3 vc6 queue`).

`diagnose` answers "what is blocking THIS function". This sweeps that same
routing over every unmatched function in the tree and ranks the answers by
RECOVERABLE BYTES, so a round of lanes can be aimed at a wall family rather
than at whatever unit happens to be next alphabetically.

Recoverable bytes = size * (1 - fuzzy/100): what closing the function would
add to the fuzzy-weighted total. It is the only ranking that does not lie in
both directions at once - a 90%-matched 9 KB function outranks a 64%-matched
580-byte one, while a stub's whole mass counts.

Writes evidence/wall-census.tsv (GENERATED - regenerate, never hand-edit) and
prints the ranked summary. Reads built objects only; compiles nothing, and
never edits source.
"""
from __future__ import annotations

import collections
import json

from homm3.vc6 import _common, diagnose

HEADER = ("class", "recoverable", "fuzzy", "size", "unit", "fn", "route",
          "knob")


def _size(fn):
    try:
        return int(fn.get("size", 0) or 0)
    except (TypeError, ValueError):
        return 0


def _targets():
    rep = _common.REPO / "build/objdiff/report.json"
    if not rep.is_file():
        _common.die("no build/objdiff/report.json - run `homm3 build` first")
    data = json.loads(rep.read_text())
    out, undiffable = [], []
    for u in data["units"]:
        unit = (u.get("name") or u.get("id") or "").split("/")[-1]
        for fn in u.get("functions", []):
            # A MISSING key is not a zero. objdiff declines to diff some
            # functions outright - typically when the object emits template
            # COMDATs with no delinked counterpart - and omits the field
            # entirely. Defaulting it to 0.0 counted 18 functions and 21 KB
            # as fully recoverable stub mass when they are not scoreable at
            # all, which is what ranked `army` first for several rounds.
            if "fuzzy_match_percent" not in fn:
                undiffable.append((unit, fn["name"], _size(fn)))
                continue
            p = fn.get("fuzzy_match_percent") or 0.0
            if p >= 99.999:
                continue
            try:
                size = int(fn.get("size", 0) or 0)
            except (TypeError, ValueError):
                size = 0
            out.append((unit, fn["name"], p, size))
    if undiffable:
        tot = sum(s for _u, _n, s in undiffable)
        print(f"[queue] {len(undiffable)} function(s), {tot/1024:.1f} KB, "
              "carry NO score at all - objdiff declines to diff them.\n"
              "        They are NOT recoverable mass and are excluded "
              "from the ranking:")
        for u, n, s in sorted(undiffable, key=lambda r: -r[2])[:8]:
            print(f"          {s:6d} B  {u}:{n[:52]}")
    return out


#: functions the router cannot see. NOT a diagnosis failure to shrug at: a
#: carve-name-only function has no source claim, so there is no symbol in the
#: base object to compare against. They are pure unreconstructed mass and they
#: are HALF of what is left, so dropping them from the ranking would
#: under-report the campaign by a factor of two.
UNCLAIMED = "unclaimed (no source binding)"


def _in_span_unclaimed():
    """(bytes, count) of functions inside an existing unit's inferred RVA span
    that no baseline row claims - and a per-unit TSV alongside the census.

    These are the cheapest unclaimed functions in the image: link-order has
    already attributed them to a unit that exists, so the source file, the
    headers and the compile profile are all in place.
    """
    import collections
    base = _common.REPO / "config/match_baseline.tsv"
    fns = _common.REPO / "evidence/link-order/functions.tsv"
    if not base.is_file() or not fns.is_file():
        return 0, 0
    claimed = set()
    for line in base.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        p = line.split("\t")
        if len(p) >= 6:
            try:
                claimed.add(int(p[5], 16))
            except ValueError:
                pass
    n_by, b_by = collections.Counter(), collections.Counter()
    hdr, total, count = None, 0, 0
    for line in fns.read_text().splitlines():
        if line.startswith("#"):
            continue
        p = line.split("\t")
        if hdr is None:
            hdr = p
            continue
        r = dict(zip(hdr, p))
        if r.get("relation") != "in-span":
            continue
        try:
            rva, size = int(r["rva"], 16), int(r["size"])
        except (ValueError, KeyError):
            continue
        if rva in claimed:
            continue
        owner = r.get("owner_or_bracket") or "(none)"
        n_by[owner] += 1
        b_by[owner] += size
        total += size
        count += 1
    out = _common.REPO / "evidence/unclaimed-in-span.tsv"
    with out.open("w") as fh:
        fh.write("# GENERATED: homm3 vc6 queue - regenerate, never hand-edit.\n")
        fh.write("# Functions inside an EXISTING unit's inferred link-order\n"
                 "# span that no match_baseline row claims.\n")
        fh.write("unit\tfunctions\tbytes\n")
        for unit, b in b_by.most_common():
            fh.write(f"{unit}\t{n_by[unit]}\t{b}\n")
    return total, count


def _horizon(in_unit_bytes: float) -> None:
    """Print what this census can and cannot see.

    Everything above ranks work INSIDE the units the build compiles. That is
    a small fraction of the engine, and a ranking that does not say so invites
    reading `queue`'s total as the distance to 100%. Uses the same filtered
    universe the README scores against, so the two never disagree.
    """
    try:
        from homm3.match import universe
        _cat, sizes, tally = universe.summary()
    except Exception as e:
        print(f"[queue] (horizon unavailable: {type(e).__name__}: {e})")
        return
    engine = tally.get("target", (0, 0))[1] + tally.get("zlib", (0, 0))[1]
    try:
        rep = json.loads(
            (_common.REPO / "build/objdiff/report.json").read_text())
    except Exception:
        return
    unit_bytes = matched = 0.0
    for u in rep["units"]:
        for f in u.get("functions", []):
            try:
                s = int(f.get("size", 0) or 0)
            except (TypeError, ValueError):
                s = 0
            unit_bytes += s
            matched += s * (f.get("fuzzy_match_percent", 0) or 0) / 100
    outside = engine - unit_bytes
    remaining = engine - matched
    in_span, in_span_n = _in_span_unclaimed()
    print(f"\nhorizon (filtered engine, the README's denominator: "
          f"{engine / 1024:.0f} KB):")
    print(f"  {matched / 1024:8.1f} KB  matched so far "
          f"({100 * matched / engine:.2f}%)")
    print(f"  {in_unit_bytes / 1024:8.1f} KB  recoverable INSIDE compiled "
          f"units - everything this census ranks")
    print(f"  {outside / 1024:8.1f} KB  in functions NO unit claims yet - "
          "invisible to every solver")
    if in_span:
        print(f"    of which {in_span / 1024:.1f} KB ({in_span_n} fn) lies in "
              "the RVA span of a unit that ALREADY EXISTS -")
        print("    the tractable tier: the unit, its headers and its build "
              "profile are all in place,")
        print("    so a lane only has to claim the addresses and reconstruct "
              "the bodies")
    if remaining:
        print(f"  -> closing the whole ranking above moves the total to "
              f"{100 * (matched + in_unit_bytes) / engine:.2f}%; the other "
              f"{100 * outside / remaining:.0f}% of the work left is claiming "
              "functions, not better matching")


def run(args) -> int:
    only = set(filter(None, (args.unit or "").split(",")))
    rows, failed = [], []
    targets = _targets()
    for i, (unit, fn, pct, size) in enumerate(targets, 1):
        if only and unit not in only:
            continue
        if not args.quiet and i % 25 == 0:
            print(f"[queue] {i}/{len(targets)}", flush=True)
        err = None
        try:
            routed = diagnose.route(unit, fn)
        except (ValueError, SystemExit) as e:
            routed, err = None, str(e) or "diagnosis failed"
        except Exception as e:  # a solver blowing up must not kill the sweep
            routed, err = None, f"{type(e).__name__}: {e}"
        if routed is None:
            failed.append((unit, fn, err or "no built objects"))
            rows.append({
                "class": UNCLAIMED, "recoverable": size * (1 - pct / 100),
                "fuzzy": pct, "size": size, "unit": unit, "fn": fn,
                "route": "reconstruct", "knob": "no source claim owns this "
                "address - carve a VA() claim and reconstruct the body",
            })
            continue
        d, _eh_div, _inl, routes = routed
        primary = next((s for s, _ in routes if s), "(none)")
        rows.append({
            "class": d["class"], "recoverable": size * (1 - pct / 100),
            "fuzzy": pct, "size": size, "unit": unit, "fn": fn,
            "route": primary, "knob": d.get("knob", ""),
        })

    rows.sort(key=lambda r: -r["recoverable"])
    out = _common.REPO / "evidence/wall-census.tsv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as fh:
        fh.write("# GENERATED: homm3 vc6 queue - regenerate, never hand-edit.\n")
        fh.write("# recoverable = size * (1 - fuzzy/100), the fuzzy-weighted\n")
        fh.write("# bytes closing this function would add.\n")
        fh.write("\t".join(HEADER) + "\n")
        for r in rows:
            fh.write("\t".join((
                r["class"], f"{r['recoverable']:.0f}", f"{r['fuzzy']:.4f}",
                str(r["size"]), r["unit"], r["fn"],
                r["route"], r["knob"].replace("\t", " "))) + "\n")

    by_class = collections.defaultdict(list)
    by_unit = collections.defaultdict(float)
    for r in rows:
        by_class[r["class"]].append(r)
        by_unit[r["unit"]] += r["recoverable"]

    in_unit = sum(r["recoverable"] for r in rows)
    print(f"\n[queue] {len(rows)} unmatched function(s), "
          f"{in_unit / 1024:.1f} KB recoverable")
    _horizon(in_unit)
    if failed:
        print(f"[queue] {len(failed)} of those have no source claim, so the "
              f"router cannot see them; they are counted as {UNCLAIMED!r} "
              "rather than dropped")
    print("\nby wall class:")
    for cls, v in sorted(by_class.items(),
                         key=lambda x: -sum(r["recoverable"] for r in x[1])):
        print(f"  {sum(r['recoverable'] for r in v) / 1024:7.1f} KB  "
              f"{len(v):3d} fn  {cls}")
    print("\nby unit (top 12):")
    for unit, kb in sorted(by_unit.items(), key=lambda x: -x[1])[:12]:
        print(f"  {kb / 1024:7.1f} KB  {unit}")
    print(f"\nwrote {out.relative_to(_common.REPO)}")
    return 0
