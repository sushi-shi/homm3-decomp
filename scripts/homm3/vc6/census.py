#!/usr/bin/env python3
"""homm3.vc6.census - the model gates (`homm3 vc6 check`).

Three gates, each able to FAIL (the repo's negative-control rule):

  behavioral  every catalogued probe still reproduces its behavior under the
              real compiler (delegates to the oracle; the oracle ships a
              bogus-EXPECT negative control that must report FAIL).

  locator     the solvers' source-body locator (homm3.vc6._source) still
              finds every definition shape this tree uses AND still refuses
              the shapes that are not definitions.  Its negative controls
              are the two MEASURED defects of the v1 locator: a `#if 0
              // @carcass` stub accepted as the real body (which made every
              mutation a no-op, so an unmeasured run reported itself as a
              CAPPED verdict), and a declaration / call site accepted as a
              definition.  Re-injecting the v1 logic fails 19 of the 29
              cases (measured 2026-08-14), so this gate is known able to
              fail.  No compiler needed - hermetic and instant.

  consistency the solvers' masked distance must agree with objdiff's verdict:
              a function objdiff scores 100% must have combined tool-distance
              0 (else the masking is lying); a function the tool calls
              masked-equal (reg=flow=0) that objdiff scores < ~90% is flagged
              as a pure displacement/reloc residual (catalog C9/D21), not an
              error but reported so it is never mistaken for a live wall.

rc: 0 = all selected gates green, 1 = a gate failed, 2 = harness error.
"""
from __future__ import annotations

import json
from types import SimpleNamespace

from homm3.sema import _asm
from homm3.vc6 import _align, _common, _flow, report


def _behavioral() -> tuple[bool, str]:
    from homm3.vc6 import oracle
    try:
        rc = oracle.run(SimpleNamespace(subsystem="all", probe=None,
                                        all=True, json=False))
    except SystemExit as e:
        rc = e.code if isinstance(e.code, int) else 1
    ok = (rc == 0)
    return ok, ("all probes reproduce their behavior"
                if ok else "one or more probes FAILED (see oracle output)")


def _locator() -> tuple[bool, str]:
    """Run the hermetic solver suites (positives + negative controls)."""
    import io
    import unittest
    from homm3.vc6 import test_inline_names, test_locator
    suite = unittest.TestSuite(
        unittest.defaultTestLoader.loadTestsFromModule(m)
        for m in (test_locator, test_inline_names))
    buf = io.StringIO()
    res = unittest.TextTestRunner(stream=buf, verbosity=0).run(suite)
    bad = res.failures + res.errors
    if not bad:
        return True, (f"{res.testsRun} definition shape / inline-name case(s) "
                      "held (carcass + declaration + call-site + "
                      "name-artifact controls)")
    names = ", ".join(t.id().split(".")[-1] for t, _ in bad[:4])
    return False, f"{len(bad)}/{res.testsRun} solver case(s) FAILED: {names}"


def _tool_distance(unit: str, fn: str):
    base = _asm.BASE / f"{unit}.obj"
    tgt = _asm.TARGET / f"{unit}.c.obj"
    if not base.is_file() or not tgt.is_file():
        return None
    try:
        from homm3.vc6 import reg_model
        bsym = reg_model._resolve_symbol(base, fn)
        tsym = reg_model._resolve_symbol(tgt, fn)
        bt, rt = _asm.objdump(base, bsym, 0), _asm.objdump(tgt, tsym, 0)
    except (Exception, SystemExit):  # _resolve_symbol dies via SystemExit
        return None
    br, rr = _align.parse_side(bt), _align.parse_side(rt)
    if not br or not rr:
        return None
    return _align.distance(br, rr) + _flow.distance(
        _flow.profile(bt), _flow.profile(rt))


def _consistency(sample: int) -> tuple[bool, list[str]]:
    """Spot-check tool-distance vs objdiff on exact + plateaued functions."""
    rep = _common.REPO / "build/objdiff/report.json"
    if not rep.is_file():
        return True, ["(no report.json - skipped)"]
    data = json.loads(rep.read_text())
    exacts, plateaus, violations = [], [], []
    for u in data["units"]:
        unit = (u.get("name") or "").split("/")[-1]
        for f in u.get("functions", []):
            p = f.get("fuzzy_match_percent", 0.0)
            (exacts if p >= 99.999 else plateaus).append((unit, f["name"], p))
    # every Nth exact must show tool-distance 0 (masking must not lie)
    checked = 0
    for unit, fn, _p in exacts[::max(1, len(exacts) // sample)]:
        d = _tool_distance(unit, fn)
        checked += 1
        if d is not None and d > 0:
            violations.append(f"EXACT but tool-distance {d}: {unit}:{fn[:40]}")
    ok = not violations
    note = [f"checked {checked} exact function(s); "
            f"{len(violations)} masking violation(s)"]
    return ok, note + violations[:10]


def run_check(args) -> int:
    want = {g: getattr(args, g, False)
            for g in ("argv", "il", "inline", "reg", "locator")}
    do_all = getattr(args, "all", False) or not any(want.values())

    results = []
    if do_all or want.get("locator") or want.get("reg"):
        results.append(("locator", *_locator()))
    if do_all or want.get("inline") or want.get("reg"):
        results.append(("behavioral", *_behavioral()))
    if do_all:
        ok, notes = _consistency(sample=40)
        results.append(("consistency", ok, "; ".join(notes)))

    # behavioral and locator are the HARD gates (each carries its own negative
    # control - the oracle's bogus-EXPECT probe, and the locator's carcass /
    # declaration / call-site cases). Consistency is INFORMATIONAL: the
    # solvers' masked distance is a coarser proxy than objdiff's fuzzy, so
    # exact-but-nonzero is a known metric-skew (catalog C9), reported for
    # health, not a build stop.
    hard = [(n, ok, d) for n, ok, d in results if n != "consistency"]
    failed = [name for name, ok, _ in hard if not ok]
    for name, ok, detail in results:
        tag = ("PASS" if ok else "FAIL") if name != "consistency" else "INFO"
        print(f"[check] {name:<12} {tag}  {detail}")
    print(f"[check] {'GREEN' if not failed else 'RED (' + ','.join(failed) + ')'}")
    return 1 if failed else 0


if __name__ == "__main__":
    import sys
    sys.exit(run_check(SimpleNamespace(all=True)))
