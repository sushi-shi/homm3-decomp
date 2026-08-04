#!/usr/bin/env python3
"""homm3.cli - the single entry point for the HoMM3 matching pipeline.

Run inside the Nix dev shell (the `homm3` wrapper, or `python3 -m homm3`).
Compiling and linking need the toolchain shell: `nix develop .#build`.

Subcommands
-----------
  init [--force] [--no-smoke]
        One-time local setup so a fresh checkout goes straight to `homm3 build`:
        the git-ignored build dirs; build.ninja + objdiff.json (configure); the
        VC6 SP3 toolchain (unpacked from build/homm3-toolchain-vc6-sp3.tar.xz if
        absent); the Wine prefix; and a smoke compile through the real cc_wrap
        path. Idempotent. --force re-inits the Wine prefix.

  configure
        Regenerate build.ninja + build/objdiff/objdiff.json from
        config/units.toml (homm3.build.configure; ninja also re-runs it as a
        generator rule).

  build [--fast] [-- <ninja args>]
        The loop tail (homm3.build.build): configure -> ninja (base objs via
        the pinned `wine cl`) -> normalize comparison copies -> objdiff
        report -> overall %% line -> baseline raise + RATCHET check (a
        function below its recorded best FAILS the build) + README score
        block + a warning when the synth-PDB inputs are newer than the PDB.
        --fast stops after the %% line (the inner matching loop).

  delink
        The delink half (homm3.build.delink, explicit - build never
        RE-delinks; a fresh tree bootstraps the first one):
        labels -> synth PDB -> data manifests -> vostok -> per-unit target
        objs -> normalize -> objdiff.json.

  status [update|check [--gate]] [--write-readme]
        Scoreboard (homm3.match.status): per-unit table; `update` raises the
        per-function maxima (config/match_baseline.tsv); `check` reports
        functions below their recorded best.

  sema <xref|diff|disasm|rva|strings> ...
        Read-only navigation over the retail image (homm3.sema): caller
        trees + exact data refs, base-vs-target block diffs (skeleton by
        default; rc=1 when the requested view differs), per-function
        disassembly of ANY retail function, address dossiers, literal
        evidence. Every invocation logs one line to build/homm3_sema.log.

  link [<homm3.build.link args>] [-- <extra link flags>]
        OPT-IN candidate link (also `ninja candidate`): genuine VC6 link.exe
        over the base objs with /FORCE /NODEFAULTLIB /MAP into
        build/exe/HEROES3.candidate.EXE. Not runnable - it exists for the .map
        layout study and the unresolved-externals punch list.

  clean
        Nuke build/ + stray root artifacts (build.ninja/*.obj/.ninja_*) so
        `homm3 init && homm3 build` rebuilds from scratch. Touches nothing
        under src/, config/, vendor/, or docs/. build/ is wholly disposable:
        the toolchain tarball re-downloads from the pinned GitHub release
        (homm3.init.toolchain) and everything else regenerates. NOTE: the next
        `homm3 init` is a heavier run (download + wine prefix).
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(os.environ.get("HOMM3_DIR",
                           next((p for p in Path(__file__).resolve().parents
                                 if (p / "flake.nix").exists()),
                                Path(__file__).resolve().parents[2])))

def log(msg: str) -> None:
    print(f"[homm3] {msg}", flush=True)


def run(*command: str) -> int:
    return subprocess.run(list(command), cwd=ROOT).returncode


def run_module(module: str, *args: str) -> int:
    return run(sys.executable, "-m", module, *args)


def cmd_init(args) -> int:
    for d in ("build/gen", "build/objdiff/base", "build/exe", "build/smoke"):
        (ROOT / d).mkdir(parents=True, exist_ok=True)
    if run_module("homm3.build.configure"):
        return 1
    toolchain_args = []
    if args.force:
        toolchain_args.append("--force")
    if args.no_smoke:
        toolchain_args.append("--no-smoke")
    if run_module("homm3.init.toolchain", *toolchain_args):
        return 1
    log("init complete. Next: `homm3 build` (inside `nix develop .#build`).")
    return 0


def cmd_configure(args) -> int:
    return run_module("homm3.build.configure")


def cmd_build(args) -> int:
    ninja_args = args.ninja_args
    if ninja_args and ninja_args[0] == "--":
        ninja_args = ninja_args[1:]
    extra = ["--fast"] if args.fast else []
    return run_module("homm3.build.build", *extra, *(ninja_args or []))


def cmd_delink(args) -> int:
    return run_module("homm3.build.delink")


def cmd_status(args) -> int:
    return run_module("homm3.match.status", *args.status_args)


def cmd_sema(args) -> int:
    return run_module("homm3.sema", *args.sema_args)


def cmd_link(args) -> int:
    if run_module("homm3.build.configure"):
        return 1
    return run_module("homm3.build.link", *args.link_args)


def _kill_wine_session() -> None:
    """Reap this prefix's wineserver BEFORE deleting build/wineprefix: a server
    left running against a deleted prefix errors saving its registry and lingers
    as a stale server that flakes the next fresh build's first compiles."""
    prefix = ROOT / "build/wineprefix"
    if not prefix.is_dir() or shutil.which("wineserver") is None:
        return
    env = dict(os.environ, WINEPREFIX=str(prefix))
    subprocess.run(["wineserver", "-k"], env=env, check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["wineserver", "--wait"], env=env, check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def cmd_clean(args) -> int:
    _kill_wine_session()
    removed = 0
    for target in [ROOT / "build", ROOT / "build.ninja", ROOT / ".ninja_log",
                   ROOT / ".ninja_deps", *sorted(ROOT.glob("*.obj"))]:
        if target.is_dir():
            shutil.rmtree(target)
            removed += 1
            log(f"removed {target.relative_to(ROOT)}/")
        elif target.exists():
            target.unlink()
            removed += 1
            log(f"removed {target.relative_to(ROOT)}")
    log(f"clean: removed {removed} path(s). Next: `homm3 init` then `homm3 build`.")
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        prog="homm3", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command")

    p = sub.add_parser("init", help="one-time local setup (toolchain, prefix, dirs)")
    p.add_argument("--force", action="store_true", help="re-init the wine prefix")
    p.add_argument("--no-smoke", action="store_true", help="skip the smoke compile")
    p.set_defaults(fn=cmd_init)

    p = sub.add_parser("configure", help="regenerate build.ninja + objdiff.json")
    p.set_defaults(fn=cmd_configure)

    p = sub.add_parser(
        "build", help="configure + ninja + report + ratchet (homm3.build.build)")
    p.add_argument("--fast", action="store_true",
                   help="inner loop: stop after the objdiff %% line")
    p.add_argument("ninja_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=cmd_build)

    p = sub.add_parser("delink", help="the delink loop: labels -> synth PDB "
                       "-> vostok -> normalized targets (homm3.build.delink)")
    p.set_defaults(fn=cmd_delink)

    p = sub.add_parser("status", help="objdiff scoreboard + ratchet "
                       "(homm3.match.status: [update|check] [--write-readme])")
    p.add_argument("status_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=cmd_status)

    p = sub.add_parser("sema", help="read-only navigation: xref / diff / "
                       "disasm / rva / strings (homm3.sema, logged)")
    p.add_argument("sema_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=cmd_sema)

    p = sub.add_parser("link", help="candidate link (layout study)")
    p.add_argument("link_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=cmd_link)

    p = sub.add_parser("clean", help="nuke build/ + stray root artifacts")
    p.set_defaults(fn=cmd_clean)

    args = ap.parse_args(argv)
    if not getattr(args, "fn", None):
        ap.print_help()
        return 0
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
