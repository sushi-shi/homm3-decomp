#!/usr/bin/env python3
"""homm3 sema - read-only semantic navigation over the retail image.

Subcommands (TARGET/ADDR = 0x<rva>, 0x<va>, an exact
build/gen/symbol_names.csv name, or a demangled Class::method / method
spelling that names exactly one retail symbol)
-----------------------------------------------------------------
  xref TARGET... [--flat [--raw]] [--callees] [--to] [--depth N]
        Caller ancestry tree (the default; depth 4, 0 = unlimited),
        flat direct callers, forward callees, or --to = every
        referencing site with its instruction (the default for a data
        address). Function views end with the reloc-backed data refs.
  diff TARGET [--no-build] [--verbose | --structure | --asm | --branches
              | --source | --calls | --relocs | --summary | --why-bytes]
              [--range START:END | --base-range ... --target-range ...]
        Base-vs-target comparison: block-SKELETON diff by default,
        --structure = explicit spelling of that block-SKELETON view,
        --verbose = block bodies, --asm = flat masked asm diff,
        --branches = symbolic branch-sequence comparison,
        --source = statement-grouped diff using candidate /Z7 lines.
        rc=1 when the REQUESTED VIEW differs (the skeleton compares
        flow shape + block sizes only; in-block changes need
        --verbose/--branches). --calls/--relocs = the ordered reference
        sequences judged like `objdiff-cli diff`; --summary = every
        verdict on one screen + the next view; --why-bytes = --summary
        + the first byte-level divergence. --verbose adds detail to ANY
        view. The unit is refreshed in place first (its ninja target,
        normalized copies, objdiff report; free when nothing changed) -
        --no-build compares the last built object.
        Names: an exact mangled name, or a demangled Class::method /
        bare method when it names one retail symbol.
  disasm TARGET [--base|--candidate] [--target] [--source] [--blocks]
                [--range START:END] [--verbose]
        One function's body: addresses + asm with call/data symbols
        folded into the operands; ANY retail function renders
        (delinked-unit object when one exists, image bytes via capstone
        otherwise). --source labels candidate statements and implies
        --base; --verbose is the raw objdump view (bytes, reloc lines).
  rva ADDR
        The address dossier: symbol, universe class, src claim,
        vtable membership, match %.
  strings [0x<addr>] [--find TEXT]
        A function's literal evidence / the functions referencing a
        matching literal.

rc: 0 = answered, 1 = answered-NO (differs), 2 = error.
Every invocation appends one line to build/homm3_sema.log.
"""
from __future__ import annotations

import argparse
import sys

from homm3.sema import _common


def _build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        prog="homm3 sema", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ss = ap.add_subparsers(dest="sema", required=True)

    sx = ss.add_parser(
        "xref", help="caller tree (default) / --flat / --callees + data refs")
    sx.add_argument("target", nargs="+", help="0x<addr> or symbol name(s)")
    sx.add_argument("--flat", action="store_true",
                    help="only direct rel32 callers (opt out of the tree)")
    sx.add_argument("--raw", action="store_true",
                    help="with --flat: every site, no per-owner dedup")
    sx.add_argument("--callees", "--calls", action="store_true",
                    help="forward: the function's own call targets")
    sx.add_argument("--to", action="store_true",
                    help="every site referencing TARGET: rel32 call/jmp sites "
                         "(functions) and dir32 data sites, each with the "
                         "referencing instruction; the default view for a "
                         "data address")
    sx.add_argument("--depth", type=int, default=4, metavar="N",
                    help="tree expansion cap (default 4; 0 = unlimited)")

    sd = ss.add_parser(
        "diff", help="base-vs-target block diff (skeleton default; rc=1 differs)")
    sd.add_argument("target", help="0x<addr> or symbol name")
    sd.add_argument("--no-build", dest="no_build", action="store_true",
                    help="compare the last built object; skip the in-place "
                         "unit refresh (ninja target + normalize + report)")
    sd.add_argument("--verbose", action="store_true",
                    help="more of the chosen view: block bodies (default/"
                         "--structure), full-context --asm, both sequences "
                         "for --branches, unchanged groups for --source")
    mode = sd.add_mutually_exclusive_group()
    mode.add_argument("--structure", "--blocks", "--skeleton",
                      action="store_true",
                      help="block-structure skeleton (explicit alias for "
                           "the default diff view; also --blocks, --skeleton)")
    mode.add_argument("--asm", action="store_true",
                      help="flat masked unified asm diff")
    mode.add_argument("--branches", action="store_true",
                      help="symbolic branch-sequence comparison (the signal "
                           "the masked views cannot show)")
    mode.add_argument("--source", action="store_true",
                      help="statement-grouped diff labelled with candidate "
                           "source from a verified /Z7 object")
    mode.add_argument("--calls", action="store_true",
                      help="ordered callee-sequence comparison judged like "
                           "`objdiff-cli diff` (function_reloc_diffs="
                           "name_address); unclaimed retail labels are marked")
    mode.add_argument("--relocs", action="store_true",
                      help="ordered comparison of every reloc reference, "
                           "calls AND data (--calls is this view restricted "
                           "to calls)")
    mode.add_argument("--summary", action="store_true",
                      help="one screen: which views agree, the first "
                           "divergence, and the next view to run")
    mode.add_argument("--why-bytes", dest="why_bytes", action="store_true",
                      help="--summary plus the first byte-level divergence "
                           "unmasked (both sides' bytes and relocs) with its "
                           "kind")
    sd.add_argument("--range", metavar="START:END",
                    help="end-exclusive function-local offset range on both "
                         "sides (for example +0xc00:+0xcdc)")
    sd.add_argument("--base-range", metavar="START:END",
                    help="candidate-local range; pair with --target-range "
                         "when the two arms begin at different offsets")
    sd.add_argument("--target-range", metavar="START:END",
                    help="retail-local range; pair with --base-range when "
                         "the two arms begin at different offsets")

    sa = ss.add_parser(
        "disasm", help="one function, symbol-folded asm; any retail fn renders")
    sa.add_argument("target", help="0x<addr> or symbol name")
    sa.add_argument("--base", "--candidate", action="store_true",
                    help="your compiled obj (the candidate) instead of retail")
    sa.add_argument("--no-build", dest="no_build", action="store_true",
                    help="with --base/--source: skip the in-place unit refresh")
    sa.add_argument("--target", dest="target_side", action="store_true",
                    help="the retail side (the default; the explicit opposite "
                         "of --base)")
    sa.add_argument("--source", action="store_true",
                    help="label candidate asm with verified /Z7 source lines "
                         "(implies --base; incompatible with --blocks)")
    sa.add_argument("--blocks", action="store_true",
                    help="basic-block CFG view (skeleton; --verbose = bodies)")
    sa.add_argument("--range", metavar="START:END",
                    help="end-exclusive function-local offset range, e.g. "
                         "+0xc00:+0xcdc")
    sa.add_argument("--verbose", action="store_true",
                    help="raw objdump rows: byte columns + reloc lines (the "
                         "default already folds every symbol into its operand)")

    sr = ss.add_parser("rva", help="address dossier (the first command on "
                                   "any address)")
    sr.add_argument("addr", help="0x<rva> or 0x<va>")

    st = ss.add_parser("strings", help="literal evidence per function / "
                                       "reverse literal lookup")
    st.add_argument("target", nargs="?", help="0x<addr> or symbol name")
    st.add_argument("--find", metavar="TEXT",
                    help="case-insensitive literal search across all functions")
    return ap


COMMANDS = ("xref", "diff", "disasm", "rva", "strings")

# What agents typed under `homm3 sema` that lives elsewhere (usage-log
# audit): the vc6 solvers, dreamcast lookups, and flag spellings guessed
# as subcommands. A wrong guess names its home instead of "invalid choice".
ELSEWHERE = {
    "why-reg": "homm3 vc6 why-reg", "why-branch": "homm3 vc6 why-branch",
    "predict-inline": "homm3 vc6 predict-inline",
    "diagnose": "homm3 vc6 diagnose", "report": "homm3 vc6 report",
    "queue": "homm3 vc6 queue", "il-diff": "homm3 vc6 il-diff",
    "xrefs": "homm3 sema xref", "callers": "homm3 sema xref",
    "callees": "homm3 sema xref TARGET --callees",
    "blocks": "homm3 sema disasm TARGET --blocks",
    "branches": "homm3 sema diff TARGET --branches",
    "calls": "homm3 sema diff TARGET --calls",
    "summary": "homm3 sema diff TARGET --summary",
    "asm": "homm3 sema disasm", "show": "homm3 dreamcast show",
    "find": "homm3 dreamcast find",
    "locate": "homm3 dreamcast find NAME (the Dreamcast roster) or "
              "homm3 sema rva 0xADDR (the address dossier)",
    "status": "homm3 status", "build": "homm3 build",
}


def _redirect(argv: list[str]) -> None:
    """Die with the command's real home for a wrong-namespace guess."""
    if not argv or argv[0].startswith("-") or argv[0] in COMMANDS:
        return
    hint = ELSEWHERE.get(argv[0])
    _common.die(f"'{argv[0]}' is not a homm3 sema command "
                f"({', '.join(COMMANDS)})"
                + (f" - you want `{hint}`" if hint else ""))


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    # Log what THIS call parsed, not sys.argv: a programmatic main(argv)
    # would otherwise write another process's command line to the log.
    import shlex
    cmd = shlex.join(["homm3", "sema", *argv])
    rc = 0
    try:
        _redirect(argv)
        args = _build_parser().parse_args(argv)
        from homm3.sema import diff, disasm, rva, strings, xref
        tool = {"xref": xref, "diff": diff, "disasm": disasm,
                "rva": rva, "strings": strings}[args.sema]
        tool.run(args)
    except SystemExit as e:
        rc = e.code if isinstance(e.code, int) else (0 if e.code is None else 1)
        _common.log_invocation(rc, cmd)
        raise
    _common.log_invocation(rc, cmd)
    return rc


if __name__ == "__main__":
    sys.exit(main())
