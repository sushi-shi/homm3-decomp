"""homm3 loki: the Loki Linux h3maped image (GCC 2.95.2, ELF i386).

  init [--exe PATH] [--debs DIR] [--sgi-stl DIR]
        verify and stage the pinned h3maped and GCC 2.95.2 toolchain
  toolchain [--debs DIR] [--sgi-stl DIR]
        stage or verify the toolchain only; print the driver version
  census [--check]
        regenerate (or check) config/retail/h3maped-loki/{objects,functions}.tsv
  build [UNIT ...] [-v] [-j N]
        compile units from config/loki/units.toml, delink their retail objects,
        canonicalize both and score them with objdiff (never the game ledger)
  disasm SELECTOR
        disassemble a retail function by address or mangled-name substring
"""
from __future__ import annotations

import argparse
import sys


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 loki", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("init", help="stage h3maped and the GCC 2.95.2 toolchain")
    p.add_argument("--exe", help="Loki h3maped (otherwise HOMM3_LOKI_H3MAPED or the staged copy)")
    p.add_argument("--debs", help="directory of the pinned Debian potato packages")
    p.add_argument("--sgi-stl", help="directory holding SGI STL 3.3 stl.tar.gz")
    p = sub.add_parser("toolchain", help="stage or verify the GCC 2.95.2 toolchain")
    p.add_argument("--debs")
    p.add_argument("--sgi-stl")
    p = sub.add_parser("census", help="object/function census of the retail image")
    p.add_argument("--check", action="store_true")
    p = sub.add_parser("build", help="compile, delink and score Loki units")
    p.add_argument("units", nargs="*")
    p.add_argument("-v", "--verbose", action="store_true", help="per-function scores")
    p.add_argument("-j", "--jobs", type=int, default=3)
    p = sub.add_parser("disasm", help="disassemble a retail function")
    p.add_argument("selector")
    args = parser.parse_args(argv)

    from homm3.core import inputs
    from homm3.loki import toolchain
    try:
        if args.command in ("init", "toolchain"):
            if args.command == "init":
                from homm3.loki.image import executable
                print(f"[loki] image: {inputs.stage_executable(executable(), args.exe)}")
            toolchain.stage(args.debs, args.sgi_stl)
            print(f"[loki] g++ {toolchain.version()} staged at {toolchain.DESTINATION}")
            return 0
        if args.command == "census":
            from homm3.loki import census
            return census.main(["--check"] if args.check else [])
        if args.command == "build":
            from homm3.loki import build
            return build.run(args.units or None, jobs=args.jobs, verbose=args.verbose)
        if args.command == "disasm":
            return _disasm(args.selector)
    except (inputs.InputError, toolchain.ToolchainError, ValueError, OSError, RuntimeError) as exc:
        print(f"[loki] ERROR: {exc}", file=sys.stderr)
        return 2
    return 0


def _disasm(selector: str) -> int:
    import capstone
    from homm3.loki.delink import census
    from homm3.loki.image import LokiImage
    rows = census()
    try:
        address = int(selector, 16)
        matches = [f for f in rows if f.address == address]
    except ValueError:
        matches = [f for f in rows if selector in f.name]
    if len(matches) != 1:
        for f in matches[:20]:
            print(f"{f.address:08x} {f.size:6} {f.name}")
        print(f"[loki] {len(matches)} functions match {selector!r}", file=sys.stderr)
        return 1
    function = matches[0]
    image = LokiImage()
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.syntax = capstone.CS_OPT_SYNTAX_ATT
    print(f"{function.address:08x} <{function.name or 'static'}> object {function.obj}, {function.size} bytes")
    for insn in decoder.disasm(image.elf.read(function.address, function.size), function.address):
        print(f"  {insn.address:08x}  {insn.mnemonic:8} {insn.op_str}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
