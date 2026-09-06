#!/usr/bin/env python3
"""Rank existing compiled functions by their banked MAX, lowest first.

Banked-exact functions stay out even if their current score dips. Size breaks
score ties; recoverable bytes weight the remaining distance to 100%.
`--admission` explicitly lists functions without compiled source bodies.
`--diagnose` adds a full solver-routing census to the otherwise cheap ranking.
Both modes write generated evidence, compile nothing, and never edit source.
"""
from __future__ import annotations

import collections
import json

from homm3.vc6 import _common, diagnose

HEADER = ("class", "recoverable", "max_fuzzy", "current_fuzzy", "size",
          "unit", "fn", "route", "knob")
ADMISSION_HEADER = ("state", "size", "rva", "relation", "owner",
                    "candidates", "label", "action")
EXACT = 100.0 - 1e-6


def _size(fn):
    try:
        return int(fn.get("size", 0) or 0)
    except (TypeError, ValueError):
        return 0


def _load_maxima(path=None):
    from homm3.match.status import load_baseline
    path = path or (_common.REPO / "config/match_baseline.tsv")
    return {key: row.best for key, row in load_baseline(path).items()}


def _compiled_functions(data):
    """Require an emitted code definition, never infer one from its name."""
    from homm3.build.canonicalize_data_symbols import (
        CoffObject, FUNCTION_TYPE, MEM_EXECUTE)
    compiled = set()
    for unit in data.get("units", []):
        name = (unit.get("name") or unit.get("id") or "").split("/")[-1]
        obj = _common.REPO / f"build/objdiff/normalized/base/{name}.obj"
        if not obj.is_file():
            continue
        coff = CoffObject(obj.read_bytes())
        compiled.update(
            (name, sym.name) for sym in coff.symbols.values()
            if sym.section > 0 and sym.typ == FUNCTION_TYPE and
            coff.sections[sym.section - 1].characteristics & MEM_EXECUTE)
    return compiled


def _partition_targets(data, maxima, compiled):
    """Return (unit, name, banked_max, current, size) for unfinished bodies."""
    out = []
    for u in data.get("units", []):
        unit = (u.get("name") or u.get("id") or "").split("/")[-1]
        for fn in u.get("functions", []):
            name = fn["name"]
            key = (unit, name)
            if key not in compiled:
                continue
            maximum = maxima.get(key, 0.0)
            if maximum >= EXACT:
                continue
            current = (float(fn["fuzzy_match_percent"])
                       if fn.get("fuzzy_match_percent") is not None else None)
            out.append((unit, name, maximum, current, _size(fn)))
    out.sort(key=lambda row: (row[2], -row[4], row[0], row[1]))
    return out


def _targets(maxima):
    rep = _common.REPO / "build/objdiff/report.json"
    if not rep.is_file():
        _common.die("no build/objdiff/report.json - run `homm3 build` first")
    data = json.loads(rep.read_text())
    return _partition_targets(data, maxima, _compiled_functions(data))


def _admission_rows_from_text(data, baseline_text, link_order_text,
                              category, sizes):
    """Return every target RVA without a diffable compiled source body.

    A report row is admitted when objdiff scores it. A scoreless mangled row
    is also admitted source (objdiff merely declines that comparison). The
    measured unadmitted case is a scoreless flat carve label: its VA claim is
    still disabled or otherwise emits no public text symbol.

    Inputs are explicit so the admission-vs-polish distinction has a
    hermetic negative control.
    """
    report_state = {}
    for u in data.get("units", []):
        unit = (u.get("name") or u.get("id") or "").split("/")[-1]
        for fn in u.get("functions", []):
            name = fn.get("name", "")
            admitted = "fuzzy_match_percent" in fn or name.startswith("?")
            report_state[(unit, name)] = admitted

    rva_states = {}
    rva_report = {}
    for line in baseline_text.splitlines():
        if line.startswith("#") or not line.strip():
            continue
        p = line.split("\t")
        if len(p) < 6:
            continue
        identity = (p[0], p[1])
        if identity not in report_state:
            continue
        try:
            rva = int(p[5], 16)
        except ValueError:
            continue
        rva_states[rva] = rva_states.get(rva, False) or report_state[identity]
        rva_report.setdefault(rva, (p[0], p[1]))

    link_rows = {}
    header = None
    for line in link_order_text.splitlines():
        if line.startswith("#") or not line.strip():
            continue
        p = line.split("\t")
        if header is None:
            header = p
            continue
        row = dict(zip(header, p))
        try:
            link_rows[int(row["rva"], 16)] = row
        except (KeyError, ValueError):
            continue

    rows = []
    for rva, size in sizes.items():
        if category.get(rva) != "target" or rva_states.get(rva, False):
            continue
        link = link_rows.get(rva, {})
        relation = link.get("relation", "unmapped")
        owner = link.get("owner_or_bracket", "")
        candidates = link.get("candidates", "")
        report_unit, report_name = rva_report.get(rva, ("", ""))
        if rva in rva_report:
            state = "carcass"
            owner = report_unit
            candidates = report_unit
            label = report_name
            action = "enable the existing VA claim and reconstruct its body"
        elif relation == "in-span":
            state = "unclaimed-in-span"
            label = link.get("label", "")
            action = f"add a VA claim and body to {owner}"
        elif relation == "bracketed":
            state = "bracketed"
            label = link.get("label", "")
            action = "resolve the owner, then add its VA claim and body"
        else:
            state = "unmapped"
            label = link.get("label", "")
            action = "locate the owning TU, then add its VA claim and body"
        rows.append({
            "state": state, "size": size, "rva": rva,
            "relation": relation, "owner": owner,
            "candidates": candidates, "label": label,
            "action": action,
        })
    rows.sort(key=lambda r: (-r["size"], r["rva"]))
    return rows


def _admission_rows():
    report = _common.REPO / "build/objdiff/report.json"
    baseline = _common.REPO / "config/match_baseline.tsv"
    links = _common.REPO / "evidence/link-order/functions.tsv"
    if not report.is_file():
        _common.die("no build/objdiff/report.json - run `homm3 build` first")
    if not baseline.is_file():
        _common.die("no config/match_baseline.tsv - run `homm3 build` first")
    from homm3.match import universe
    category, sizes = universe.classify()
    return _admission_rows_from_text(
        json.loads(report.read_text()), baseline.read_text(),
        links.read_text() if links.is_file() else "", category, sizes)


def _run_admission(args) -> int:
    only = set(filter(None, (args.unit or "").split(",")))
    rows = _admission_rows()
    if only:
        rows = [r for r in rows if r["owner"] in only or
                any(c in only for c in r["candidates"].split(",") if c)]

    out = _common.REPO / "evidence/admission-queue.tsv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as fh:
        fh.write("# GENERATED: homm3 vc6 queue --admission - regenerate, never hand-edit.\n")
        fh.write("# Admission campaign only; sorted by retail size descending.\n")
        fh.write("\t".join(ADMISSION_HEADER) + "\n")
        for r in rows:
            fh.write("\t".join((
                r["state"], str(r["size"]), f"0x{r['rva']:x}",
                r["relation"], r["owner"], r["candidates"], r["label"],
                r["action"].replace("\t", " "))) + "\n")

    by_state = collections.Counter(r["state"] for r in rows)
    total = sum(r["size"] for r in rows)
    print(f"[queue] admission-first: {len(rows)} function(s), "
          f"{total / 1024:.1f} KB not yet in the diffable build")
    for state, count in by_state.most_common():
        code = sum(r["size"] for r in rows if r["state"] == state)
        print(f"  {code / 1024:8.1f} KB  {count:4d} fn  {state}")
    if rows:
        print("\nnext largest admissions:")
        for r in rows[:getattr(args, "limit", 20) or None]:
            label = r["label"] or "(unnamed)"
            print(f"  {r['size']:6d} B  0x{r['rva'] + 0x400000:08x}  "
                  f"{r['state']:<19} {r['owner'] or r['candidates']:<28} "
                  f"{label[:64]}")
    print(f"\nwrote {out.relative_to(_common.REPO)}")
    return 0


def _run_polish(args) -> int:
    only = set(filter(None, (args.unit or "").split(",")))
    rows, failed = [], []
    maxima = _load_maxima()
    targets = _targets(maxima)
    for i, (unit, fn, pct, current, size) in enumerate(targets, 1):
        if only and unit not in only:
            continue
        row = {
            "class": "not diagnosed", "recoverable": size * (1 - pct / 100),
            "max_fuzzy": pct, "current_fuzzy": current, "size": size,
            "unit": unit, "fn": fn, "route": "diagnose", "knob": "",
        }
        rows.append(row)
        if not getattr(args, "diagnose", False):
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
            reason = err or "no built objects"
            failed.append((unit, fn, reason))
            row.update({
                "class": "unclassified",
                "knob": "diagnosis failed: " + reason,
            })
            continue
        d, _eh_div, _inl, routes = routed
        primary = next((s for s, _ in routes if s), "(none)")
        row.update({
            "class": d["class"],
            "route": primary, "knob": d.get("knob", ""),
        })

    # Hardest first: ascending banked MAX. Size only breaks score ties.
    rows.sort(key=lambda r: (r["max_fuzzy"], -r["size"], r["unit"], r["fn"]))
    out = _common.REPO / "evidence/wall-census.tsv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as fh:
        command = "homm3 vc6 queue" + (" --diagnose" if getattr(args, "diagnose", False) else "")
        fh.write(f"# GENERATED: {command} - regenerate, never hand-edit.\n")
        fh.write("# Existing compiled bodies, sorted by banked MAX/history; "
                 "retail size breaks ties.\n")
        fh.write("# Banked-exact current dips are observational and excluded.\n")
        fh.write("# recoverable = size * (1 - max_fuzzy/100).\n")
        fh.write("\t".join(HEADER) + "\n")
        for r in rows:
            fh.write("\t".join((
                r["class"], f"{r['recoverable']:.0f}",
                f"{r['max_fuzzy']:.4f}",
                (f"{r['current_fuzzy']:.4f}" if r['current_fuzzy'] is not None else "-"),
                str(r["size"]), r["unit"], r["fn"],
                r["route"], r["knob"].replace("\t", " ") or "-")) + "\n")

    print("\nnext polish functions (ascending banked MAX):")
    for r in rows[:getattr(args, "limit", 20) or None]:
        print(f"  {r['max_fuzzy']:6.2f}%  {r['unit']}:{r['fn']}  {r['route']}")

    by_class = collections.defaultdict(list)
    by_unit = collections.defaultdict(float)
    for r in rows:
        by_class[r["class"]].append(r)
        by_unit[r["unit"]] += r["recoverable"]

    in_unit = sum(r["recoverable"] for r in rows)
    print(f"\n[queue] {len(rows)} never-exact function(s), "
          f"{in_unit / 1024:.1f} KB recoverable")
    if failed:
        print(f"[queue] {len(failed)} diagnosis failure(s) retained as "
              "inspectable unclassified rows")
    if getattr(args, "diagnose", False):
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


def run(args) -> int:
    if getattr(args, "limit", 20) < 0:
        _common.die("--limit must be >= 0")
    if getattr(args, "admission", False):
        return _run_admission(args)
    return _run_polish(args)
