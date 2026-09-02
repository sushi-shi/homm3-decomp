#!/usr/bin/env python3
"""homm3.vc6.queue - admission-first campaign routing.

The default `homm3 vc6 queue` lists target functions that do not yet have a
diffable compiled source body. It covers disabled carcass claims, unclaimed
functions inside a known TU span, bracketed functions whose owner still needs
resolution, and the remaining unmapped target rows. They are sorted largest
first: admit the hardest functions before polishing code that is already in
the build.

`homm3 vc6 queue --polish` retains the old tree-wide wall census. It sweeps
`diagnose` over every admitted never-exact function and ranks by ascending
effective MAX, while excluding banked-exact current dips. That campaign is
parked until admission is complete.

Writes evidence/admission-queue.tsv by default or evidence/wall-census.tsv in
polish mode. Both are generated analysis outputs. The command compiles
nothing and never edits source.
"""
from __future__ import annotations

import collections
import json

from homm3.vc6 import _common, diagnose

HEADER = ("class", "recoverable", "max_fuzzy", "current_fuzzy", "size",
          "unit", "fn", "route", "knob")
ADMISSION_HEADER = ("state", "size", "rva", "relation", "owner",
                    "candidates", "label", "action")
EXACT = 99.999


def _size(fn):
    try:
        return int(fn.get("size", 0) or 0)
    except (TypeError, ValueError):
        return 0


def _load_maxima(path=None):
    """Return the ratchet's banked MAX score for each report identity."""
    path = path or (_common.REPO / "config/match_baseline.tsv")
    out = {}
    if not path.is_file():
        return out
    for line in path.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        cols = line.split("\t")
        if len(cols) < 4:
            continue
        try:
            out[(cols[0], cols[1])] = float(cols[3])
        except ValueError:
            continue
    return out


def _partition_targets(data, maxima):
    """Split report rows into actionable, banked dips, and undiffable rows.

    Actionable tuples are (unit, name, effective_max, current, size). A row
    whose effective max is exact is never actionable. This is intentionally a
    pure helper so the MAX-routing defect has a hermetic negative control.
    """
    out, banked_dips, undiffable = [], [], []
    for u in data["units"]:
        unit = (u.get("name") or u.get("id") or "").split("/")[-1]
        for fn in u.get("functions", []):
            name = fn["name"]
            size = _size(fn)
            maximum = maxima.get((unit, name), 0.0)
            has_score = "fuzzy_match_percent" in fn
            current = float(fn.get("fuzzy_match_percent") or 0.0)
            effective = max(current, maximum)

            if effective >= EXACT:
                if not has_score or current < EXACT:
                    banked_dips.append(
                        (unit, name, maximum, current if has_score else None,
                         size))
                continue

            # A MISSING key has TWO causes and they are opposites, which the
            # NAME FORM tells apart:
            #   * a FLAT CARVE NAME (`army_do_post_attack`) means the body
            #     still sits in `#if 0` - sec-0 undefined, so the delinker
            #     labels it with the flat src-VA name. It is unclaimed
            #     carcass and its whole size IS recoverable. Once claimed and
            #     re-delinked it scores normally: an army lane took thirteen
            #     such rows from "no key" to 67-100% in one session.
            #   * a MANGLED name (`?LootDeadHero@...`) means objdiff genuinely
            #     declines to diff a claimed function - the object emits
            #     template COMDATs with no delinked counterpart. That one is
            #     not recoverable mass.
            # Excluding both (2026-08-20) wrongly dropped 20 KB of real work
            # and kept the 723 B that actually is undiffable - backwards.
            if not has_score and name.startswith("?"):
                undiffable.append((unit, name, size))
                continue
            out.append((unit, name, effective, current, size))
    out.sort(key=lambda row: (row[2], -row[4], row[0], row[1]))
    return out, banked_dips, undiffable


def _targets(maxima):
    rep = _common.REPO / "build/objdiff/report.json"
    if not rep.is_file():
        _common.die("no build/objdiff/report.json - run `homm3 build` first")
    data = json.loads(rep.read_text())
    out, banked_dips, undiffable = _partition_targets(data, maxima)
    if undiffable:
        tot = sum(s for _u, _n, s in undiffable)
        print(f"[queue] {len(undiffable)} function(s), {tot/1024:.1f} KB, "
              "are CLAIMED but carry no score - objdiff declines to\n"
              "        diff them. Not recoverable mass; excluded from the ranking:")
        for u, n, s in sorted(undiffable, key=lambda r: -r[2])[:8]:
            print(f"          {s:6d} B  {u}:{n[:52]}")
    if banked_dips:
        print(f"[queue] {len(banked_dips)} current dip(s) are already banked "
              "exact and are excluded from actionable ranking.")
        print("        Inspect them as collateral, but do not rewrite source "
              "unless an evidence/source gate fails:")
        for u, n, mx, cur, s in sorted(
                banked_dips,
                key=lambda r: (r[3] is None, r[3] if r[3] is not None else 0,
                               -r[4]))[:8]:
            shown = "no current score" if cur is None else f"current {cur:.4f}%"
            print(f"          {s:6d} B  {u}:{n[:44]}  {shown}; "
                  f"MAX {mx:.4f}%")
    return out


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
        fh.write("# GENERATED: homm3 vc6 queue - regenerate, never hand-edit.\n")
        fh.write("# Admission campaign only; sorted by retail size descending.\n")
        fh.write("# Use `homm3 vc6 queue --polish` only after admission.\n")
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
        for r in rows[:20]:
            label = r["label"] or "(unnamed)"
            print(f"  {r['size']:6d} B  0x{r['rva'] + 0x400000:08x}  "
                  f"{r['state']:<19} {r['owner'] or r['candidates']:<28} "
                  f"{label[:64]}")
    print(f"\nwrote {out.relative_to(_common.REPO)}")
    return 0


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
    # This side census must use the same authoritative universe as the main
    # queue. Otherwise an admitted EH funclet/runtime thunk inside a unit's
    # inferred span is falsely advertised as source-reconstructable work.
    from homm3.match import universe
    category, _sizes = universe.classify()
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
        if category.get(rva) not in ("target", "zlib"):
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


def _lost_peaks():
    """Rows whose `hist` peak exceeds their current `max`.

    THE RATCHET CANNOT SEE THESE. It compares against `max`, so once a max has
    been accepted downward the row sits below a value it once reached and the
    build stays green forever. `hist` is the only record that it was ever
    higher, and recovering it is ordinary work - two such rows were recovered
    in one lane (20.7 and 11.5 points) after a retired view gate had left an
    invariant unenforced.
    """
    path = _common.REPO / "config/match_baseline.tsv"
    if not path.is_file():
        return []
    out = []
    for line in path.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        p = line.split("\t")
        if len(p) < 6:
            continue
        try:
            mx, hist = float(p[3]), float(p[4])
        except ValueError:
            continue
        if hist > mx + 1e-9:
            out.append((hist - mx, p[0], p[1], mx, hist))
    out.sort(reverse=True)
    return out


def _horizon(in_unit_bytes: float, maxima) -> None:
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
        unit = (u.get("name") or u.get("id") or "").split("/")[-1]
        for f in u.get("functions", []):
            try:
                s = int(f.get("size", 0) or 0)
            except (TypeError, ValueError):
                s = 0
            unit_bytes += s
            current = float(f.get("fuzzy_match_percent", 0) or 0)
            banked = maxima.get((unit, f.get("name", "")), 0.0)
            matched += s * max(current, banked) / 100
    outside = engine - unit_bytes
    remaining = engine - matched
    in_span, in_span_n = _in_span_unclaimed()
    print(f"\nhorizon (filtered engine, the README's denominator: "
          f"{engine / 1024:.0f} KB):")
    print(f"  {matched / 1024:8.1f} KB  banked matched so far "
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


def _run_polish(args) -> int:
    only = set(filter(None, (args.unit or "").split(",")))
    rows, failed = [], []
    maxima = _load_maxima()
    targets = _targets(maxima)
    for i, (unit, fn, pct, current, size) in enumerate(targets, 1):
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
            reason = err or "no built objects"
            unclaimed = "no shared public text symbol" in reason
            failed.append((unit, fn, reason, unclaimed))
            rows.append({
                "class": UNCLAIMED if unclaimed else "unclassified",
                "recoverable": size * (1 - pct / 100),
                "max_fuzzy": pct, "current_fuzzy": current, "size": size,
                "unit": unit, "fn": fn,
                "route": "reconstruct" if unclaimed else "diagnose",
                "knob": ("no source claim owns this address - carve a VA() "
                         "claim and reconstruct the body" if unclaimed else
                         "diagnosis failed: " + reason),
            })
            continue
        d, _eh_div, _inl, routes = routed
        primary = next((s for s, _ in routes if s), "(none)")
        rows.append({
            "class": d["class"], "recoverable": size * (1 - pct / 100),
            "max_fuzzy": pct, "current_fuzzy": current, "size": size,
            "unit": unit, "fn": fn,
            "route": primary, "knob": d.get("knob", ""),
        })

    # Hardest first: ascending effective MAX. Size only breaks score ties.
    rows.sort(key=lambda r: (r["max_fuzzy"], -r["size"], r["unit"], r["fn"]))
    out = _common.REPO / "evidence/wall-census.tsv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as fh:
        fh.write("# GENERATED: homm3 vc6 queue - regenerate, never hand-edit.\n")
        fh.write("# Sorted hardest-first by effective MAX = "
                 "max(current_fuzzy, banked MAX).\n")
        fh.write("# Banked-exact current dips are observational and excluded.\n")
        fh.write("# recoverable = size * (1 - max_fuzzy/100).\n")
        fh.write("\t".join(HEADER) + "\n")
        for r in rows:
            fh.write("\t".join((
                r["class"], f"{r['recoverable']:.0f}",
                f"{r['max_fuzzy']:.4f}", f"{r['current_fuzzy']:.4f}",
                str(r["size"]), r["unit"], r["fn"],
                r["route"], r["knob"].replace("\t", " "))) + "\n")

    by_class = collections.defaultdict(list)
    by_unit = collections.defaultdict(float)
    for r in rows:
        by_class[r["class"]].append(r)
        by_unit[r["unit"]] += r["recoverable"]

    in_unit = sum(r["recoverable"] for r in rows)
    print(f"\n[queue] {len(rows)} never-exact function(s), "
          f"{in_unit / 1024:.1f} KB recoverable")
    _horizon(in_unit, maxima)
    if failed:
        unclaimed_count = sum(unclaimed for *_rest, unclaimed in failed)
        diagnosis_count = len(failed) - unclaimed_count
        if unclaimed_count:
            print(f"[queue] {unclaimed_count} of those have no compiled "
                  "source binding, so the router cannot see them; they are "
                  f"counted as {UNCLAIMED!r} rather than dropped")
        if diagnosis_count:
            print(f"[queue] {diagnosis_count} additional diagnosis failure(s) "
                  "are retained as inspectable unclassified rows")
    print("\nby wall class:")
    for cls, v in sorted(by_class.items(),
                         key=lambda x: -sum(r["recoverable"] for r in x[1])):
        print(f"  {sum(r['recoverable'] for r in v) / 1024:7.1f} KB  "
              f"{len(v):3d} fn  {cls}")
    print("\nby unit (top 12):")
    for unit, kb in sorted(by_unit.items(), key=lambda x: -x[1])[:12]:
        print(f"  {kb / 1024:7.1f} KB  {unit}")
    lost = _lost_peaks()
    if lost:
        print(f"\n{len(lost)} row(s) below a peak they once reached "
              f"({sum(d for d, *_ in lost):.1f} points). The ratchet compares "
              "against\nmax and CANNOT see these - only hist records that they "
              "were ever higher:")
        for d, unit, fn, mx, hist in lost[:8]:
            print(f"  +{d:6.2f}  {unit:<14}{fn[:46]}  max {mx:.2f} hist "
                  f"{hist:.2f}")

    print(f"\nwrote {out.relative_to(_common.REPO)}")
    return 0


def run(args) -> int:
    if getattr(args, "polish", False):
        return _run_polish(args)
    return _run_admission(args)
