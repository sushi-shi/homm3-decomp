#!/usr/bin/env python3
"""homm3 vc6 - predictive model of the VC6 SP3 compiler + match-loop solvers.

The retail image is matched by whatever CL/C1XX/C2 actually do. This area
reverse-engineers that behaviour and turns it into predictions, so a residual
is diagnosed by the model instead of a blind spelling sweep.

Subcommands
-----------
  argv [--flags "<cl flags>" | --unit <tu>] [--pass c1|c2] [--verify]
        Decode the CL.EXE option-spec table: what per-pass argv C1XX and C2
        receive for a command line. --verify checks against the shim log.
  il-diff <srcA> <srcB> [--flags ...] [--fn NAME]      (phase 1)
        Compile both, diff the C1XX->C2 IL at record granularity.
  predict-inline <src> --fn CALLER [--against UNIT:FN]  (phase 3)
        Per-call-site expand/call decisions with the budget trajectory.
  why-reg <src> --fn F --against UNIT:FN                (phase 0 v1)
        Which known knob moves a divergent register binding toward retail.
  oracle <subsystem> [--probe NAME | --all]
        Ground-truth runner: compile a probe with the real compiler, read
        back the fact the model predicts.
  atlas --regen                                         (phase 2)
        Headless-Ghidra C2 TU/globals map -> evidence/vc6/.
  check [--argv|--il|--inline|--reg|--locator|--all]
        The gates (each ships a negative control).

rc: 0 = agrees / answered, 1 = disagrees / answered-NO, 2 = error.
Every invocation appends one line to build/homm3_vc6.log.
"""
from __future__ import annotations

import argparse
import sys

from homm3.vc6 import _common


def _build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        prog="homm3 vc6", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ss = ap.add_subparsers(dest="cmd", required=True)

    pa = ss.add_parser("argv", help="per-pass argv from the CL spec table")
    src = pa.add_mutually_exclusive_group()
    src.add_argument("--flags", help='a full CL command line, e.g. "/O2 /Ob2 /Gr"')
    src.add_argument("--unit", help="resolve a units.toml unit's flag profile")
    pa.add_argument("--pass", dest="which", choices=["c1", "c2"],
                    help="show only one pass's argv")
    pa.add_argument("--verify", action="store_true",
                    help="compare the model against the shim-logged actuals")
    pa.add_argument("--json", action="store_true")

    pi = ss.add_parser("il-diff", help="record-granular C1XX->C2 IL diff")
    pi.add_argument("srcA")
    pi.add_argument("srcB")
    pi.add_argument("--flags", default="")
    pi.add_argument("--fn")
    pi.add_argument("--json", action="store_true")

    pp = ss.add_parser("predict-inline", help="inline-structure divergence "
                       "(which callees retail inlines vs we do)")
    pp.add_argument("src")
    pp.add_argument("--fn", required=True)
    pp.add_argument("--against", metavar="UNIT:FN")
    pp.add_argument("--against-src", dest="against_src", metavar="FILE")
    pp.add_argument("--json", action="store_true")

    pw = ss.add_parser("why-reg", help="which knob fixes a register binding")
    pw.add_argument("src")
    pw.add_argument("--fn", required=True)
    ref = pw.add_mutually_exclusive_group(required=True)
    ref.add_argument("--against", metavar="UNIT:FN",
                     help="retail reference function (via sema)")
    ref.add_argument("--against-src", metavar="FILE",
                     help="compile this TU and use its --fn as the reference "
                          "(hermetic self-test / demo)")
    pw.add_argument("--model", action="store_true",
                    help="v2: RE'd-allocator model path (predicts the ONE "
                         "creation-order edit) instead of the guided sweep")
    pw.add_argument("--tries", type=int, default=1,
                    help="v2: model-proposed candidates to compile (default 1)")
    pw.add_argument("--il-order", action="store_true",
                    help="v2: derive pseudo order from the captured IL handles")
    pw.add_argument("--json", action="store_true")

    pb = ss.add_parser("why-branch", help="which control-flow spelling "
                       "reproduces retail's jumps")
    pb.add_argument("src")
    pb.add_argument("--fn", required=True)
    bref = pb.add_mutually_exclusive_group(required=True)
    bref.add_argument("--against", metavar="UNIT:FN",
                      help="retail reference function (via sema)")
    bref.add_argument("--against-src", metavar="FILE",
                      help="compile this TU and use its --fn as the reference")
    pb.add_argument("--json", action="store_true")

    po = ss.add_parser("oracle", help="real-compiler ground-truth runner")
    po.add_argument("subsystem")
    po.add_argument("--probe")
    po.add_argument("--all", action="store_true")
    po.add_argument("--json", action="store_true")

    pt = ss.add_parser("atlas", help="headless-Ghidra C2 map -> evidence/vc6")
    pt.add_argument("--regen", action="store_true")

    pab = ss.add_parser("ab", help="RTM-vs-SP3 C2 A/B (Track R): build-rtm | "
                        "run [--fn ...] | clean")
    pab.add_argument("ab_args", nargs=argparse.REMAINDER)

    pd = ss.add_parser("diagnose", help="classify a residual + route to the "
                       "right solver (register / control-flow / inliner)")
    pd.add_argument("target", help="UNIT:FN or a mangled function name")
    pd.add_argument("--run", action="store_true",
                    help="also run the routed solver(s) and show the edit")
    pd.add_argument("--json", action="store_true")

    pc = ss.add_parser("check", help="the model gates (with negative controls)")
    for g in ("argv", "il", "inline", "reg", "locator"):
        pc.add_argument(f"--{g}", action="store_true")
    pc.add_argument("--all", action="store_true")

    return ap


# subcommand -> (module attribute, function name); imported lazily so a
# half-built area still runs the modules that DO exist.
_TOOLS = {
    "argv": ("argv", "run"),
    "il-diff": ("il", "run_diff"),
    "predict-inline": ("inline_model", "run_predict"),
    "why-reg": ("reg_model", "run_why"),
    "why-branch": ("flow_model", "run_why"),
    "oracle": ("oracle", "run"),
    "diagnose": ("diagnose", "run"),
    "atlas": ("atlas", "run"),
    "check": ("census", "run_check"),
}


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    import shlex
    cmd = shlex.join(["homm3", "vc6", *argv])
    rc = 0
    try:
        args = _build_parser().parse_args(argv)
        if args.cmd == "ab":  # genab has its own argv-style CLI
            from homm3.vc6 import genab
            rc = genab.main(args.ab_args)
            _common.log_invocation(rc, cmd)
            return rc
        mod_name, fn_name = _TOOLS[args.cmd]
        try:
            mod = __import__(f"homm3.vc6.{mod_name}", fromlist=[fn_name])
        except ImportError as e:
            _common.die(f"'{args.cmd}' not yet implemented ({e})")
        rc = getattr(mod, fn_name)(args) or 0
    except SystemExit as e:
        rc = e.code if isinstance(e.code, int) else (0 if e.code is None else 1)
        _common.log_invocation(rc, cmd)
        raise
    _common.log_invocation(rc, cmd)
    return rc


if __name__ == "__main__":
    sys.exit(main())
