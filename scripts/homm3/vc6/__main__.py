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
  predict-inline SELECTOR [--against SELECTOR]        (phase 3)
        Per-call-site expand/call decisions with the budget trajectory.
  why-reg SELECTOR                                   (phase 0 v1)
        Which known knob moves a divergent register binding toward retail.
  oracle <subsystem> [--probe NAME | --all]
        Ground-truth runner: compile a probe with the real compiler, read
        back the fact the model predicts.
  atlas --regen                                         (phase 2)
        Headless-Ghidra C2 TU/globals map -> evidence/vc6/.
  tryblocks
        Retail's CATCH-SCOPE census, read straight off the image's
        `_s_FuncInfo` records: every function with nTryBlocks > 0, each try
        block's [tryLow, tryHigh] state range, the type each arm catches
        and the catch funclet addresses. A body where retail has a try and
        we have none is a target, not an inliner wall.
  check [--argv|--il|--inline|--reg|--locator|--all]
        The gates (each ships a negative control).

rc: 0 = agrees / answered, 1 = disagrees / answered-NO, 2 = error.
Every invocation appends one line to build/homm3_vc6.log.
"""
from __future__ import annotations

import argparse
import sys

from homm3.vc6 import _common


def _solver_arguments(parser):
    parser.add_argument("src", nargs="?", metavar="SELECTOR|SOURCE",
                        help="retail VA/RVA, symbol, Class::method, bare name, "
                             "or UNIT:SELECTOR; alternatively SOURCE or TU --fn SELECTOR")
    parser.add_argument("--fn", metavar="SELECTOR",
                        help="function selector when supplying a source file")
    reference = parser.add_mutually_exclusive_group()
    reference.add_argument("--against", metavar="SELECTOR",
                           help="override the inferred retail reference")
    reference.add_argument("--against-src", metavar="FILE",
                           help="compile a reference source instead of using retail")


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
    _solver_arguments(pp)
    pp.add_argument("--json", action="store_true")
    pp.add_argument("--no-build", action="store_true",
                    help="use the last built manifest object without a source/header refresh")

    pw = ss.add_parser("why-reg", help="which knob fixes a register binding")
    _solver_arguments(pw)
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
    _solver_arguments(pb)
    pb.add_argument("--json", action="store_true")

    po = ss.add_parser("oracle", help="real-compiler ground-truth runner")
    po.add_argument("subsystem")
    po.add_argument("--probe")
    po.add_argument("--all", action="store_true")
    po.add_argument("--json", action="store_true")

    pt = ss.add_parser("atlas", help="headless-Ghidra C2 map -> evidence/vc6")
    pt.add_argument("--regen", action="store_true")

    pa = ss.add_parser("disasm", help="labeled pinned C2.DLL assembly and references")
    pa.add_argument("target", help="C2 RVA, VA, Ghidra name, or documented role")
    pa.add_argument("--range", help="end-exclusive offsets from target, e.g. +0:+0x80")
    pa.add_argument("--refs", action="store_true", help="show incoming code references")
    pa.add_argument("--verbose", action="store_true", help="include instruction bytes")

    pab = ss.add_parser("ab", help="RTM-vs-SP3 generation A/B (Track R): "
                        "build-rtm | build-rtm-fe | verify [--gen ...] | "
                        "run [--gen rtm|rtm-fe] [--fn ...] [--all-units] | "
                        "clean")
    pab.add_argument("ab_args", nargs=argparse.REMAINDER)

    pd = ss.add_parser("diagnose", help="classify a residual + route to the "
                       "right solver (register / control-flow / inliner)")
    pd.add_argument("target", help="retail VA/RVA, symbol, Class::method, "
                                  "bare name, or UNIT:SELECTOR")
    pd.add_argument("--run", action="store_true",
                    help="also run the routed solver(s) and show the edit")
    pd.add_argument("--json", action="store_true")

    pr = ss.add_parser("report", help="per-plateau markdown table -> "
                       "evidence/vc6/plateau-diagnosis.md")
    pr.add_argument("--lo", type=float, default=50.0)
    pr.add_argument("--unit")
    pr.add_argument("--limit", type=int)

    pq = ss.add_parser("queue", help="polish compiled functions by ascending "
                       "banked MAX, excluding banked-exact functions")
    pq.add_argument("--unit", help="restrict to a comma-separated unit list")
    mode = pq.add_mutually_exclusive_group()
    mode.add_argument("--polish", action="store_true",
                      help="polish existing compiled functions (the default)")
    mode.add_argument("--admission", action="store_true",
                      help="list functions without compiled bodies, largest first")
    pq.add_argument("--diagnose", action="store_true",
                    help="also diagnose every polish target (slower)")
    pq.add_argument("--quiet", action="store_true")
    pq.add_argument("--limit", type=int, default=20, metavar="N",
                    help="maximum ranked functions to display (default 20; 0 = all)")

    ss.add_parser("tryblocks", help="retail's catch-scope census: every "
                  "FuncInfo with nTryBlocks > 0, its try extents, catch "
                  "types and funclet addresses")

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
    "disasm": ("disasm", "run"),
    "report": ("report", "run"),
    "queue": ("queue", "run"),
    "tryblocks": ("tryblocks", "run"),
    "check": ("census", "run_check"),
}


def _dispatch(argv):
    args = _build_parser().parse_args(argv)
    if args.cmd == "ab":
        from homm3.vc6 import genab
        return genab.main(args.ab_args)
    mod_name, fn_name = _TOOLS[args.cmd]
    mod = __import__(f"homm3.vc6.{mod_name}", fromlist=[fn_name])
    return getattr(mod, fn_name)(args) or 0


def main(argv=None) -> int:
    from homm3.core.usage import run_logged
    import shlex
    argv = list(sys.argv[1:] if argv is None else argv)
    cmd = shlex.join(["homm3", "vc6", *argv])
    return run_logged(_dispatch, argv,
                      lambda rc, **meta: _common.log_invocation(rc, cmd, **meta))


if __name__ == "__main__":
    sys.exit(main())
