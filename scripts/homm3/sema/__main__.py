#!/usr/bin/env python3
"""homm3 sema - read-only semantic navigation over the retail image.

Subcommands (TARGET/ADDR = 0x<rva>, 0x<va>, an exact
build/gen/symbol_names.csv name, or a demangled Class::method / method
spelling that names exactly one retail symbol)
-----------------------------------------------------------------
  xref TARGET... [--flat [--raw]] [--callees] [--depth N]
        Caller ancestry tree (the default; depth 4, 0 = unlimited),
        flat direct callers, or forward callees - always ending with
        the exact reloc-backed data-side references.
  diff TARGET [--verbose | --structure | --asm | --branches | --source]
        Base-vs-target comparison: block-SKELETON diff by default,
        --structure = explicit spelling of that block-SKELETON view,
        --verbose = block bodies, --asm = flat masked asm diff,
        --branches = symbolic branch-sequence comparison,
        --source = statement-grouped diff using candidate /Z7 lines.
        rc=1 when the REQUESTED VIEW differs (the skeleton compares
        flow shape + block sizes only; in-block changes need
        --verbose/--branches). --verbose adds detail to ANY view.
        Names: an exact mangled name, or a demangled Class::method /
        bare method when it names one retail symbol.
  disasm TARGET [--base] [--source] [--blocks] [--verbose]
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
    sx.add_argument("--callees", action="store_true",
                    help="forward: the function's own call targets")
    sx.add_argument("--depth", type=int, default=4, metavar="N",
                    help="tree expansion cap (default 4; 0 = unlimited)")

    sd = ss.add_parser(
        "diff", help="base-vs-target block diff (skeleton default; rc=1 differs)")
    sd.add_argument("target", help="0x<addr> or symbol name")
    sd.add_argument("--verbose", action="store_true",
                    help="more of the chosen view: block bodies (default/"
                         "--structure), full-context --asm, both sequences "
                         "for --branches, unchanged groups for --source")
    mode = sd.add_mutually_exclusive_group()
    mode.add_argument("--structure", action="store_true",
                      help="block-structure skeleton (explicit alias for "
                           "the default diff view)")
    mode.add_argument("--asm", action="store_true",
                      help="flat masked unified asm diff")
    mode.add_argument("--branches", action="store_true",
                      help="symbolic branch-sequence comparison (the signal "
                           "the masked views cannot show)")
    mode.add_argument("--source", action="store_true",
                      help="statement-grouped diff labelled with candidate "
                           "source from a verified /Z7 object")

    sa = ss.add_parser(
        "disasm", help="one function, symbol-folded asm; any retail fn renders")
    sa.add_argument("target", help="0x<addr> or symbol name")
    sa.add_argument("--base", action="store_true",
                    help="your compiled obj instead of retail")
    sa.add_argument("--source", action="store_true",
                    help="label candidate asm with verified /Z7 source lines "
                         "(implies --base; incompatible with --blocks)")
    sa.add_argument("--blocks", action="store_true",
                    help="basic-block CFG view (skeleton; --verbose = bodies)")
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


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    # Log what THIS call parsed, not sys.argv: a programmatic main(argv)
    # would otherwise write another process's command line to the log.
    import shlex
    cmd = shlex.join(["homm3", "sema", *argv])
    rc = 0
    try:
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
