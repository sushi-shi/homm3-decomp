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


def _targets():
    rep = _common.REPO / "build/objdiff/report.json"
    if not rep.is_file():
        _common.die("no build/objdiff/report.json - run `homm3 build` first")
    data = json.loads(rep.read_text())
    out = []
    for u in data["units"]:
        unit = (u.get("name") or u.get("id") or "").split("/")[-1]
        for fn in u.get("functions", []):
            p = fn.get("fuzzy_match_percent", 0.0) or 0.0
            if p >= 99.999:
                continue
            try:
                size = int(fn.get("size", 0) or 0)
            except (TypeError, ValueError):
                size = 0
            out.append((unit, fn["name"], p, size))
    return out


#: functions the router cannot see. NOT a diagnosis failure to shrug at: a
#: carve-name-only function has no source claim, so there is no symbol in the
#: base object to compare against. They are pure unreconstructed mass and they
#: are HALF of what is left, so dropping them from the ranking would
#: under-report the campaign by a factor of two.
UNCLAIMED = "unclaimed (no source binding)"


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

    print(f"\n[queue] {len(rows)} unmatched function(s), "
          f"{sum(r['recoverable'] for r in rows) / 1024:.1f} KB recoverable")
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
