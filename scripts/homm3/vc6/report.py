#!/usr/bin/env python3
"""homm3.vc6.report - the diagnosis-only plateau report.

Runs the why-reg (register) and why-branch (control-flow) DIAGNOSES over the
current plateaus, reading the objects the build already produced
(`build/objdiff/base/<unit>.obj` vs the delinked `target/<unit>.c.obj`) - NO
recompiles. That means:
  * zero wine contention with a concurrent matching session;
  * the exact obj the ratchet scored (not a standalone compile, so the C1
    include-set discrepancy does not enter);
  * a collision-free read-only artifact - it proposes nothing to source.

Emits `evidence/vc6/plateau-diagnosis.md`: one row per plateaued function
(fuzzy in [lo, 99.999%)), its combined wall class, the register/flow distance
signals, and the catalog knob to try. The mutation SEARCH (which needs
compiles) is left to `homm3 vc6 why-reg`/`why-branch` on the individual
function - this report routes you there.

    python3 -m homm3.vc6.report [--lo 50] [--unit U] [--limit N]
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from homm3.sema import _asm
from homm3.sema.context import get_context
from homm3.vc6 import _align, _common, _flow, reg_model

try:
    from homm3.vc6 import flow_model  # noqa: F401  (kept for parity/imports)
except Exception:
    flow_model = None


def _plateaus(lo: float, unit_filter: str | None):
    import json
    rep = _common.REPO / "build/objdiff/report.json"
    if not rep.is_file():
        _common.die("no build/objdiff/report.json - run `homm3 build --fast`")
    data = json.loads(rep.read_text())
    rows = []
    for u in data["units"]:
        unit = (u.get("name") or u.get("id") or "").split("/")[-1]
        if unit_filter and unit != unit_filter:
            continue
        for fn in u.get("functions", []):
            p = fn.get("fuzzy_match_percent", 0.0)
            if lo <= p < 99.999:
                rows.append((p, unit, fn["name"]))
    rows.sort()
    return rows


def _inline_div(base_text, ref_text):
    """'N under-inline, M over-inline' if out-of-line CALL multisets differ."""
    from homm3.vc6 import inline_model
    bc, rc = inline_model._called(base_text), inline_model._called(ref_text)
    keys = set(bc) | set(rc)
    under = sum(max(0, bc[s] - rc[s]) for s in keys)
    over = sum(max(0, rc[s] - bc[s]) for s in keys)
    if not under and not over:
        return None
    return ", ".join([f"{under} under-inline"] * bool(under)
                     + [f"{over} over-inline"] * bool(over))


# combined wall class from the diagnoses -> the catalog knob to reach for
def _route(reg_dist, reg_findings, flow_dist, flow_findings, inline_div=None):
    cats = {f.get("catalog", "") for f in reg_findings} | \
           {f.get("catalog", "") for f in flow_findings}
    kinds = {f.get("kind", "") for f in reg_findings} | \
            {f.get("kind", "") for f in flow_findings}
    flow_struct = flow_dist and any(
        "STRUCT" in f.get("kind", "").upper() or "block" in f.get("detail", "")
        for f in flow_findings)
    if inline_div:  # inline structure is upstream of blocks + registers
        return "inliner (predict-inline)", (
            "callee expanded on one side only (A8/A9/A12): " + inline_div)
    if flow_struct or any(k.startswith("D") for k in cats):
        cls = "control-flow (why-branch)"
        knob = "loop-form / merged-return placement / case order (D1-D9)"
    elif reg_dist and any(c.startswith("B") for c in cats):
        cls = "register-homing (why-reg)"
        # pick the most specific B-signal present
        if any("param" in k for k in kinds):
            knob = "spill to dead-parameter slot (B4)"
        elif any("homing" in k for k in kinds):
            knob = "cache-vs-reload a member/local (B13) / homing (B2/B3)"
        elif any("binding" in k for k in kinds):
            knob = "name a value to steer pseudo order->EAX (B14) / decl order (B6)"
        else:
            knob = "register-homing knob (B-family)"
    elif reg_dist == 0 and flow_dist == 0:
        cls = "masked-equal (displacement/reloc only)"
        knob = "no source knob - objdiff-only residual (C9 / D21 cosmetic)"
    else:
        cls = "unclassified"
        knob = "run why-reg / why-branch for the full search"
    return cls, knob


def _diagnose_one(unit, fnspec):
    base_obj = _asm.BASE / f"{unit}.obj"
    tgt_obj = _asm.TARGET / f"{unit}.c.obj"
    if not base_obj.is_file() or not tgt_obj.is_file():
        return None
    ctx = get_context()
    try:
        name, runit, rva, size, ordinal = ctx.symbols.resolve_fn(fnspec)
    except Exception:
        name, ordinal = fnspec, 0
    try:
        base_text = _asm.objdump(base_obj, name, ordinal)
        ref_text = _asm.objdump(tgt_obj, name, ordinal)
    except Exception as e:
        return {"error": str(e)[:60]}
    br, rr = _align.parse_side(base_text), _align.parse_side(ref_text)
    if not br or not rr:
        return {"error": "no instructions parsed"}
    reg_dist = _align.distance(br, rr)
    reg_findings, _ = _align.diagnose(br, rr, base_text, ref_text)
    bp, rp = _flow.profile(base_text), _flow.profile(ref_text)
    flow_dist = _flow.distance(bp, rp)
    flow_findings, _ = _flow.diagnose(bp, rp)
    inline_div = _inline_div(base_text, ref_text)
    cls, knob = _route(reg_dist, reg_findings, flow_dist, flow_findings,
                       inline_div)
    return {"reg_dist": reg_dist, "flow_dist": flow_dist,
            "class": cls, "knob": knob,
            "reg_top": reg_findings[0]["detail"][:70] if reg_findings else "",
            "flow_top": flow_findings[0]["detail"][:70] if flow_findings else ""}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="python3 -m homm3.vc6.report")
    ap.add_argument("--lo", type=float, default=50.0)
    ap.add_argument("--unit")
    ap.add_argument("--limit", type=int)
    a = ap.parse_args(argv)

    rows = _plateaus(a.lo, a.unit)
    if a.limit:
        rows = rows[:a.limit]
    out = _common.EVIDENCE / "plateau-diagnosis.md"
    out.parent.mkdir(parents=True, exist_ok=True)

    from collections import Counter
    by_class = Counter()
    lines = ["<!-- " + " | ".join(_common.provenance(
        "homm3.vc6.report",
        [f"plateaus in [{a.lo}, 99.999%); base-vs-delinked-target diagnosis, "
         "no recompiles"])) + " -->",
        "# vc6 plateau diagnosis (read-only; solvers propose, never land)", "",
        f"{len(rows)} function(s). why-reg = register-homing knobs; "
        "why-branch = control-flow knobs; predict-inline = out-of-line CALL "
        "multiset divergence (a callee inlined on one side only - dominated by "
        "STL basic_string/vector ops + small dtors retail inlines and we do "
        "not). MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = "
        "clamp(2*caller_cb,1000,35000) spent sequentially; our leaner "
        "reconstructions sit at the 1000 floor and STARVE, so retail inlines "
        "what we call. FIX = finish the caller's body (budget follows statement "
        "mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or "
        "pragmas. So on LOW-% rows inline divergence largely self-resolves as "
        "reconstruction completes; it is the pure wall only on high-% rows. "
        "Mixed walls list both distances.",
        "", "| fuzzy | unit | function | wall class | reg-dist | flow-dist | "
        "knob to try |", "|---|---|---|---|---|---|---|"]
    for p, unit, fn in rows:
        d = _diagnose_one(unit, fn)
        short = fn if len(fn) < 46 else fn[:44] + ".."
        if d is None:
            lines.append(f"| {p:.2f} | {unit} | `{short}` | "
                         "(objs missing) | - | - | build first |")
            continue
        if "error" in d:
            lines.append(f"| {p:.2f} | {unit} | `{short}` | "
                         f"(diag error: {d['error']}) | - | - | - |")
            continue
        by_class[d["class"]] += 1
        lines.append(
            f"| {p:.2f} | {unit} | `{short}` | {d['class']} | "
            f"{d['reg_dist']} | {d['flow_dist']} | {d['knob']} |")
    summary = ["", "## Wall-class summary", ""]
    for cls, n in by_class.most_common():
        summary.append(f"- **{n}** {cls}")
    lines[4:4] = summary  # inject after the intro line
    out.write_text("\n".join(lines) + "\n")
    print(f"[vc6 report] {len(rows)} plateau(s) diagnosed -> "
          f"{out.relative_to(_common.REPO)}")
    for cls, n in by_class.most_common():
        print(f"  {n:3d}  {cls}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
