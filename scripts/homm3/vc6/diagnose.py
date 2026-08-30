#!/usr/bin/env python3
"""homm3.vc6.diagnose - one command from a red `sema diff` to a routed solver.

Given UNIT:FN, read the same base-vs-delinked-target pair the ratchet scores,
classify the residual into a wall family (register / control-flow / inliner /
masked-cosmetic), and route to the solver that owns it:

    register-homing  -> why-reg     (guided register-knob search)
    control-flow     -> why-branch  (guided CFG-knob search)
    inliner (block-count / string-return)  -> predict-inline  [Phase 3]
    masked-equal     -> no source knob (objdiff-only residual)

With --run it invokes the routed solver(s) and prints the proposed edit;
otherwise it prints the exact command(s) to run. It NEVER edits source; the
solvers leave application to the caller.
"""
from __future__ import annotations

import json

from homm3.sema import _asm
from homm3.vc6 import _common, _eh, _unit, inline_model, report


def _resolve(target: str):
    """(unit, fn, source_path) from UNIT:FN or a bare mangled name."""
    if ":" in target:
        unit, fn = target.split(":", 1)
        src = _unit.source_for_unit(unit)
        return unit, fn, src
    # bare name: resolve the owning unit via the carve roster
    try:
        from homm3.sema.context import get_context
        _n, unit, *_ = get_context().symbols.resolve_fn(target)
    except Exception:
        unit = None
    return unit, target, (_unit.source_for_unit(unit) if unit else None)


def _inline_divergence(unit: str, fn: str, ordinal: int = 0):
    """A short 'N under-inline, M over-inline' note if the out-of-line CALL
    multisets of base vs retail differ, else None. Reads built objs; no
    compile."""
    base, tgt = _asm.BASE / f"{unit}.obj", _asm.TARGET / f"{unit}.c.obj"
    if not base.is_file() or not tgt.is_file():
        return None
    try:
        bc = inline_model._called(
            _asm.objdump(base, fn, ordinal))
        rc = inline_model._called(
            _asm.objdump(tgt, fn, ordinal))
    except (Exception, SystemExit):
        return None
    return inline_model.divergence_note(bc, rc)


def route(unit: str, fn: str):
    """(diagnosis, eh_div, inline_div, routes) for one function, or None when
    the unit has no built objects.

    THE routing decision - `run` renders it for one target, `queue` sweeps it
    over the tree. Both must see the same answer, so neither owns a second
    copy of these rules. Raises ValueError if the diagnosis itself failed.
    """
    d = report._diagnose_one(unit, fn)
    if d is None:
        return None
    if "error" in d:
        raise ValueError(d["error"])

    reg, flow = d["reg_dist"], d["flow_dist"]
    cls = d["class"]
    # inline structure first: a diverging out-of-line CALL multiset means a
    # callee is inlined on one side only (the A-family), which reshapes blocks
    # and registers downstream - fix it before any spelling.
    compiled_fn = d.get("_symbol", fn)
    ordinal = d.get("_ordinal", 0)
    inline_div = _inline_divergence(unit, compiled_fn, ordinal)
    # EH cleanup transcript: object lifetimes, which none of the three solvers
    # reads. A COUNT divergence outranks everything below it - it says a
    # statement (or a throwing callee) is missing, not mis-spelled.
    eh_div = _eh.divergence(unit, compiled_fn)
    # routing: eh-cleanup -> inline -> control-flow -> register
    #          (lifetimes, then structure, then spelling)
    routes = []
    if eh_div:
        routes.append((None,
                       f"EH cleanup transcript diverges ({eh_div['kind']}) - "
                       + eh_div["note"] + "; read the unwind map before any "
                       "spelling (docs/vc6/eh-cleanup.md)"))
    if inline_div:
        routes.append(("predict-inline",
                       f"inline structure diverges ({inline_div}) - a callee "
                       "is expanded on one side only (A8/A9/A12)"))
    if "control-flow" in cls or (flow and flow >= 3):
        routes.append(("why-branch", "control-flow (CFG shape) first - "
                       "structural before register"))
    if "register" in cls or (reg and reg > 0 and not routes):
        routes.append(("why-reg", "register-binding / homing knobs"))
    if "masked" in cls:
        routes.append((None, "objdiff-only residual (displacement/reloc); "
                       "no source knob - see catalog C9/D21"))
    if not routes:
        routes.append((None, "unclassified - try why-reg then why-branch"))
    return d, eh_div, inline_div, routes


def run(args) -> int:
    unit, fn, src = _resolve(args.target)
    if not unit:
        _common.die(f"could not resolve the owning unit for {args.target!r} "
                    "(pass UNIT:FN explicitly)")
    try:
        routed = route(unit, fn)
    except ValueError as e:
        _common.die(f"diagnosis failed: {e}")
    if routed is None:
        _common.die(f"no build objects for {unit} - run `homm3 build` first")
    d, eh_div, _inline_div, routes = routed
    reg, flow, cls = d["reg_dist"], d["flow_dist"], d["class"]

    solver_cmds = []
    for solver, _why in routes:
        if solver:
            solver_cmds.append(
                f"homm3 vc6 {solver} {src and src.relative_to(_common.REPO)} "
                f"--fn '{fn}' --against {unit}:'{fn}'")

    if args.json:
        print(json.dumps({
            "target": args.target, "unit": unit, "fn": fn,
            "reg_dist": reg, "flow_dist": flow, "class": cls,
            "knob": d["knob"], "eh": eh_div,
            "routes": [r[0] for r in routes],
            "commands": solver_cmds}, indent=2))
        return 0

    print(f"[diagnose] {fn}  in {unit}")
    print(f"  register-distance {reg} | flow-distance {flow}")
    print(f"  wall class : {cls}")
    if d.get("reg_top"):
        print(f"  reg signal : {d['reg_top']}")
    if d.get("flow_top"):
        print(f"  flow signal: {d['flow_top']}")
    if eh_div:
        print(f"  eh signal  : {_eh.format_line(eh_div)}")
    print(f"  knob to try: {d['knob']}")
    print("  route ->")
    for solver, why in routes:
        tag = solver or "(none)"
        print(f"    {tag:<11} {why}")
    if solver_cmds:
        print("  run:")
        for c in solver_cmds:
            print(f"    {c}")

    if args.run and src:
        from types import SimpleNamespace
        for solver, _why in routes:
            if not solver:
                continue
            print(f"\n===== {solver} =====")
            sub = SimpleNamespace(src=str(src), fn=fn,
                                  against=f"{unit}:{fn}", against_src=None,
                                  json=False,
                                  # v2 model path for register walls (it
                                  # predicts + bounds in 1 compile); harmless
                                  # attrs the other solvers ignore.
                                  model=(solver == "why-reg"), tries=1,
                                  il_order=False)
            modname, fname = {
                "why-reg": ("reg_model", "run_why"),
                "why-branch": ("flow_model", "run_why"),
                "predict-inline": ("inline_model", "run_predict"),
            }[solver]
            mod = __import__(f"homm3.vc6.{modname}", fromlist=[fname])
            try:
                getattr(mod, fname)(sub)
            except SystemExit:
                pass
    return 0


if __name__ == "__main__":
    import argparse
    import sys
    ap = argparse.ArgumentParser(prog="python3 -m homm3.vc6.diagnose")
    ap.add_argument("target")
    ap.add_argument("--run", action="store_true")
    ap.add_argument("--json", action="store_true")
    sys.exit(run(ap.parse_args()))
