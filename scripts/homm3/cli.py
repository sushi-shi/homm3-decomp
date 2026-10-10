#!/usr/bin/env python3
"""homm3.cli - the single entry point for the HoMM3 matching pipeline.

Run inside the Nix dev shell (the `homm3` wrapper, or `python3 -m homm3`).
Compiling and linking need the toolchain shell: `nix develop .#build`.

Images
------
  --image KEY (before the subcommand) selects the linked program: `game`
  (HEROES3.EXE, the default) or another `image = true` pin in
  config/project.toml, such as `h3maped` (the GOG Complete map editor). Its
  tables live under config/retail/<KEY>/, its units in config/units.<KEY>.toml,
  its ledger in config/match_baseline.<KEY>.tsv and its state under
  build/<KEY>/ (homm3.core.images, homm3.core.paths).

Subcommands
-----------
  census [--write] [--check]
        Derive a non-game image's function universe, vtables and relocation
        manifest from its pinned executable (homm3.census).

  placements [--write] [--check]
        Place the functions of a non-game image's shared units by masked body
        identity, retail references and RTTI vtables (homm3.census.placements).

  init [--exe PATH] [--dreamcast-exe PATH] [--mac-exe PATH]
       [--mac-toolchain PATH] [--force] [--no-smoke]
        One-time local setup so a fresh checkout goes straight to `homm3 build`:
        verify and stage HOMM3_EXE, HOMM3_DREAMCAST_EXE and HOMM3_MAC_EXE (CLI paths override
        the environment); read Dreamcast debug symbols from its executable;
        the git-ignored build dirs; build.ninja + objdiff.json (configure); the
        VC6 SP3 toolchain (unpacked from build/homm3-toolchain-vc6-sp3.tar.xz if
        absent); the Wine prefix; and a smoke compile through the real cc_wrap
        path. Idempotent. --force re-inits the Wine prefix.

  configure
        Regenerate build.ninja + build/objdiff/objdiff.json from
        config/units.toml (homm3.build.configure; ninja also re-runs it as a
        generator rule).

  build [--fast|--data] [--skip-image KEY] [TU ...] [-- <ninja args>]
        The final checkpoint (homm3.build.build): configure -> ninja (base objs via
        the pinned `wine cl`) -> delink and normalize comparison copies -> objdiff
        report -> overall %% line -> checkpoint-ledger refresh + observational
        dip report + fatal evidence/source gates + README score block.
        Windows only: Classic Mac pairs are the separate `homm3 mac build`.
        --data adds the complete byte accounting (`homm3 verify data-coverage`).
        A full build also builds every pinned editor image and fails at once
        when one's executable or SP3 MFC overlay is missing; --skip-image KEY
        opts out explicitly and the summary names it.
        Normally use `homm3 build --fast TU` for the inner matching loop:
        compile and normalize only the selected manifest unit, keep existing
        retail targets, and report per-function projected MAX changes without
        banking them.
        Run a full `homm3 build` for the final checkpoint.

  warnings [--compiler both|clang|msvc] [--unit TU] [--jobs N]
        Generate a fresh compiler-diagnostic report with Clang -Weverything
        and pinned VC6 /W4. Full logs and isolated objects go under build/;
        source, matching objects, the ledger and README stay unchanged.

  labels [--unit U ...|--all]
        Source-claim extraction (homm3.retail_labels.source): the lexical
        VA/VA_COMPGEN/DATA/DATA_COMPGEN scan over src/*.c* + the base-obj
        name-authority join -> per-TU claim fragments in build/gen/claims/
        (content-idempotent cache; the macros in src/ are the storage).

  model
        The one label join (homm3.model - the only place labeling policy
        lives): function census x provider tables x claim fragments ->
        build/gen/symbol_names.csv (the synth-PDB inventory) +
        build/gen/compgen_claims.tsv.

  delink
        Refresh retail targets directly (also part of full `homm3 build`):
        labels -> model -> synth PDB -> data manifests -> vostok ->
        per-unit target objs -> normalize -> objdiff.json.

  status [summary|functions|update|check|merge-baseline|snapshot|diff|hist-gap|last-exact] ...
        Scoreboard (homm3.match.status): per-unit table; `functions` shows
        cur/max/hist (filters: --unit, --va, --below, --non-exact, --json);
        `update` regenerates config/match_baseline.tsv; `check`
        classifies source-edit MAX resets against the local or a committed
        baseline without gating. `merge-baseline` merges concurrent score rows
        three ways. `snapshot FILE` / `diff --against REF|FILE` compare
        per-function CUR/MAX. Read-only views show the last measured report
        while units have unbuilt edits, naming them. Unrelated CUR dips are
        silent; HIST preserves older peaks. `hist-gap` lists rows whose HIST
        exceeds MAX with the commits that lost the peak; `last-exact SEL
        --diff` diffs a function against its last exact commit.
        `homm3 status <cmd> --help`.

  sema <xref|diff|disasm|switchmap|rva|strings|data|coverage|candidates|compare> ...
        Read-only navigation over the retail image (homm3.sema): caller
        trees + exact data refs (xref --to = every referencing site),
        base-vs-target diffs (skeleton by default; --summary = every
        verdict on one screen; --calls/--relocs judged like objdiff; rc=1
        when the requested view differs), per-function disassembly of ANY
        retail function, address dossiers, literal evidence. Every
        invocation logs one line to build/homm3_sema.log.

  dreamcast <show|lines|asm|find|gaps|inline-clues|stats|structure|audit|diff-locals|compare-calls> ...
        Source-shape navigation over the older WinCE/SH4 pressing:
        joined CodeView names/signatures/locals/scopes, breakpoint-labelled SH4
        assembly and CFG blocks, explicitly qualified retail correlations,
        and generated C++/JSON reference trees (structure).

  constants [VALUE ...] [--name TEXT] [--literals [--file F]] [--json]
        Named-constant index (homm3.analysis.constants): value -> every
        enumerator, integral const, numeric #define and array length in
        src/include, plus Dreamcast CodeView and NH3API enumerators.
        --literals ranks bare integer literals that should be tied to one.

  evidence SELECTOR... [--only|--skip SECTION,...] [--json] [--out DIR]
        The AGENTS.md evidence pass in one process: dreamcast show / lines /
        asm --blocks / inline-clues / audit, sema diff --summary / --structure
        / --source, and mac show / disasm / calls when a Mac pair is claimed.

  worktree <new PATH [-b BRANCH] [--from REF]|remove PATH>
        Create a linked worktree with the toolchain, inputs, Mac SDK and
        seeded comparison state so `homm3 build --fast TU` works at once;
        remove a clean one and stop its Wine server.

  mac <labels|show|disasm|diff|build> ...
        Navigate and byte-compare admitted Classic Mac PowerPC counterparts.

  link [<homm3.build.link args>] [-- <extra link flags>]
        Link the base objects with genuine VC6 on the retail link line (object
        order, victor/zlib libraries, /OPT:REF) into
        build/exe/HEROES3.candidate.EXE (also `ninja candidate`); `--study`
        keeps the name-ordered layout study. Unresolved or duplicate symbols
        fail the link. A .map and full linker diagnostics accompany it.
        `homm3 verify link-diff` compares it with retail region by region
        against config/link_diff.tsv; `homm3 build` gates on it.

  clean
        Nuke build/ + stray root artifacts (build.ninja/*.obj/.ninja_*) so
        `homm3 init && homm3 build` rebuilds from scratch. Touches nothing
        under src/, config/, vendor/, or docs/. build/ is wholly disposable:
        the toolchain tarball re-downloads from the pinned GitHub release
        (homm3.init.toolchain). Supply the executable paths again after cleaning.
        NOTE: the next `homm3 init` is a heavier run (download + wine prefix).
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

from homm3.core import root as _root
from homm3.core.images import path as _image_path

ROOT = _root.process_root(_root.code_root(__file__) or Path(__file__).resolve().parents[2])

def log(msg: str) -> None:
    print(f"[homm3] {msg}", flush=True)


def run(*command: str) -> int:
    from homm3.core.usage import run_process
    return run_process(list(command), cwd=ROOT)


def run_module(module: str, *args: str) -> int:
    return run(sys.executable, "-m", module, *args)


def cmd_init(args) -> int:
    from homm3.core import inputs, nb11
    from homm3.mac import toolchain as mac_toolchain
    from homm3.core import paths
    if not paths.is_game():
        # Another image needs only its own pinned executable and directories;
        # the toolchain, Wine prefix and game inputs are shared with the game.
        try:
            staged = inputs.stage_executable(inputs.RETAIL, args.exe)
        except inputs.InputError as exc:
            log(f"ERROR: {exc}")
            return 1
        for d in ("build/gen", "build/objdiff/base"):
            (ROOT / _image_path(d)).mkdir(parents=True, exist_ok=True)
        log(f"{paths.image_key()}: input verified: {staged}")
        from homm3.init import mfc_sp3
        mfc = args.mfc_sp3 or os.environ.get("HOMM3_MFC_SP3")
        if mfc:
            try:
                log(f"SP3 MFC overlay verified and staged: {mfc_sp3.stage(mfc)}")
            except (ValueError, OSError) as exc:
                log(f"ERROR: {exc}")
                return 1
        elif not mfc_sp3.staged():
            log("SP3 MFC overlay not staged; pass --mfc-sp3 DIR (the extracted "
                "VS6 SP3 vc98/mfc) or set HOMM3_MFC_SP3")
        return 0
    try:
        retail = inputs.stage_executable(inputs.RETAIL, args.exe)
        dreamcast = inputs.stage_executable(inputs.DREAMCAST, args.dreamcast_exe)
        mac = inputs.stage_executable(inputs.MAC, args.mac_exe)
        nb11.parse(inputs.read_verified(inputs.DREAMCAST, dreamcast))
        mac_toolchain.stage(args.mac_toolchain)
    except (inputs.InputError, ValueError, OSError) as exc:
        log(f"ERROR: {exc}")
        return 1
    log(f"inputs verified: {retail}, {dreamcast} (with embedded debug symbols), {mac}")
    for d in (_image_path("build/gen"), _image_path("build/objdiff/base"), _image_path("build/exe"), "build/smoke"):
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
    if run_module("homm3.build.compilation_database"):
        return 1
    log("init complete. Next: `homm3 build` (inside `nix develop .#build`).")
    return 0


def cmd_configure(args) -> int:
    return run_module("homm3.build.configure")


def cmd_build(args) -> int:
    ninja_args = args.ninja_args
    if ninja_args and ninja_args[0] == "--":
        ninja_args = ninja_args[1:]
    extra = [flag for flag, on in (("--fast", args.fast), ("--data", args.data)) if on]
    for key in args.skip_image or []:
        extra += ["--skip-image", key]
    return run_module("homm3.build.build", *extra, *(ninja_args or []))


def cmd_labels(args) -> int:
    if args.selftest:
        return run_module("homm3.retail_labels.source", "--selftest")
    forwarded = ["--all"] if args.all else []
    for unit in args.unit or []:
        forwarded += ["--unit", unit]
    return run_module("homm3.retail_labels.source", *forwarded)


def cmd_model(args) -> int:
    return run_module("homm3.model")


def cmd_delink(args) -> int:
    forwarded = []
    for unit in args.unit or []:
        forwarded += ["--unit", unit]
    return run_module("homm3.build.delink", *forwarded)


def run_status(status_args: list[str]) -> int:
    if "update" in status_args or "--write-readme" in status_args:
        # Banking and README regeneration rewrite the ledger and report.
        from homm3.core import worktree_lock
        with worktree_lock.hold(" ".join(["homm3 status", *status_args])):
            return run_module("homm3.match.status", *status_args)
    return run_module("homm3.match.status", *status_args)


def cmd_sema(args) -> int:
    return run_module("homm3.sema", *args.sema_args)


def cmd_dreamcast(args) -> int:
    return run_module("homm3.analysis.dreamcast", *args.dreamcast_args)


def cmd_vc6(args) -> int:
    return run_module("homm3.vc6", *args.vc6_args)


def cmd_hypotheses(args) -> int:
    return run_module("homm3.hypotheses", *args.hypotheses_args)


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


def _dispatch(argv: list[str]) -> int:
    if argv and argv[0] == 'verify':
        return run_module('homm3.verify', *argv[1:])
    # argparse does not reliably pass option-looking tokens through a
    # REMAINDER positional (notably ``homm3 dreamcast --help``).  Dreamcast is
    # a complete nested CLI, so hand it its argv before the umbrella parser
    # gets a chance to consume or reject any of those options.
    if argv and argv[0] == "compare":
        return run_module("homm3.compare.run", *argv[1:])
    if argv and argv[0] == "source-ownership":
        return run_module("homm3.match.source_ownership", *argv[1:])
    if argv and argv[0] == "source-inventory":
        return run_module("homm3.match.source_inventory", *argv[1:])
    if argv and argv[0] == "status":
        return run_status(argv[1:])
    if argv and argv[0] == "dreamcast":
        return run_module("homm3.analysis.dreamcast", *argv[1:])
    if argv and argv[0] == "mac":
        return run_module("homm3.mac", *argv[1:])
    if argv and argv[0] == "warnings":
        return run_module("homm3.analysis.compiler_warnings", *argv[1:])
    if argv and argv[0] == "victor":
        return run_module("homm3.victor", *argv[1:])
    if argv and argv[0] == "link":
        return cmd_link(argparse.Namespace(link_args=argv[1:]))
    if argv and argv[0] == "rmg":
        return run_module("homm3.rmg", *argv[1:])
    if argv and argv[0] == "worktree":
        return run_module("homm3.worktree", *argv[1:])
    if argv and argv[0] == "evidence":
        return run_module("homm3.evidence", *argv[1:])
    if argv and argv[0] == "census":
        return run_module("homm3.census", *argv[1:])
    if argv and argv[0] == "placements":
        return run_module("homm3.census.placements", *argv[1:])
    if argv and argv[0] == "constants":
        return run_module("homm3.analysis.constants", *argv[1:])

    ap = argparse.ArgumentParser(
        prog="homm3", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command")

    sub.add_parser("victor", add_help=False,
                   help="execute Victor resource comparisons (homm3 victor --help)")
    sub.add_parser("rmg", add_help=False,
                   help="retail/candidate whole-map comparisons and rainbow tables (homm3 rmg --help)")

    sub.add_parser("worktree", add_help=False,
                   help="create/remove a linked worktree ready for `build --fast` "
                        "(homm3 worktree --help)")
    sub.add_parser("evidence", add_help=False,
                   help="one-shot matching evidence pass for selectors (homm3 evidence --help)")
    sub.add_parser("placements", add_help=False,
                   help="place shared units' functions in another image "
                        "(homm3 --image KEY placements --help)")
    sub.add_parser("census", add_help=False,
                   help="derive another image's function/vtable/relocation census "
                        "(homm3 --image KEY census --help)")
    sub.add_parser("constants", add_help=False,
                   help="value -> named constants index and literal scan (homm3 constants --help)")

    p = sub.add_parser("init", help="one-time local setup (executables, symbols, toolchain)")
    p.add_argument("--exe", metavar="PATH",
                   help="retail HEROES3.EXE (otherwise HOMM3_EXE or staged copy)")
    p.add_argument("--dreamcast-exe", metavar="PATH",
                   help="Dreamcast H3.EXE (otherwise HOMM3_DREAMCAST_EXE or staged copy)")
    p.add_argument("--mac-exe", metavar="PATH",
                   help="Classic Mac Heroes_III_raw.pef (otherwise HOMM3_MAC_EXE or staged copy)")
    p.add_argument("--mac-toolchain", metavar="PATH",
                   help="directory with pinned CodeWarrior tools (otherwise HOMM3_MAC_TOOLCHAIN or staged copy)")
    p.add_argument("--mfc-sp3", metavar="DIR",
                   help="with --image: the extracted VS6 SP3 vc98/mfc directory "
                        "(otherwise HOMM3_MFC_SP3 or the staged build/mfc-sp3)")
    p.add_argument("--force", action="store_true", help="re-init the wine prefix")
    p.add_argument("--no-smoke", action="store_true", help="skip the smoke compile")
    p.set_defaults(fn=cmd_init)

    p = sub.add_parser("configure", help="regenerate build.ninja + objdiff.json")
    p.set_defaults(fn=cmd_configure)

    sub.add_parser("warnings", add_help=False,
                   help="fresh Clang/VC6 warning report (homm3 warnings --help)")
    sub.add_parser('compare', add_help=False, help='compare existing objects without compiling or banking scores')
    sub.add_parser('verify', add_help=False,
                   help='data-relocs / data-access / data-coverage / data-tu-order / library-data-refs / library-code / generated-code / link-diff')

    p = sub.add_parser(
        "build", help="compile + delink + report + evidence/source gates")
    p.add_argument("--fast", action="store_true",
                   help="inner loop: normally supply a TU; stop after the objdiff %% line")
    p.add_argument("--data", action="store_true",
                   help="full build: also refresh the complete byte accounting")
    p.add_argument("--skip-image", action="append", metavar="KEY",
                   help="full build: do not build this pinned editor image (repeatable; "
                        "the summary names it). Without it a missing image input fails")
    p.add_argument("ninja_args", nargs=argparse.REMAINDER,
                   help="manifest TU names or Ninja targets/arguments")
    p.set_defaults(fn=cmd_build)

    p = sub.add_parser("labels", help="source macros -> per-TU claim "
                       "fragments (homm3.retail_labels.source)")
    p.add_argument("--unit", action="append",
                   help="extract one unit (repeatable)")
    p.add_argument("--all", action="store_true",
                   help="extract every src/ unit")
    p.add_argument("--selftest", action="store_true",
                   help="run the completeness gate's negative control")
    p.set_defaults(fn=cmd_labels)

    p = sub.add_parser("model", help="the one label join -> symbol_names.csv "
                       "+ compgen_claims.tsv (homm3.model)")
    p.set_defaults(fn=cmd_model)

    p = sub.add_parser("delink", help="the delink loop: labels -> model -> "
                       "synth PDB -> vostok -> normalized targets "
                       "(homm3.build.delink)")
    p.add_argument("--unit", action="append",
                   help="refresh selected source labels without the global "
                        "label self-test/completeness gate (repeatable)")
    p.set_defaults(fn=cmd_delink)

    p = sub.add_parser("source-ownership", add_help=False,
                       help="validate CodeView definition ownership and order")
    p.add_argument("ownership_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=lambda args: run_module("homm3.match.source_ownership", *args.ownership_args))

    p = sub.add_parser("source-inventory", add_help=False,
                       help="reconcile DC and authored functions in both directions")
    p.add_argument("inventory_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=lambda args: run_module("homm3.match.source_inventory", *args.inventory_args))

    sub.add_parser("status", add_help=False,
                   help="objdiff scoreboard + checkpoint ledger (homm3 status --help)")

    p = sub.add_parser("sema", help="read-only navigation: xref / diff / "
                       "disasm / rva / strings (homm3.sema, logged)")
    p.add_argument("sema_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=cmd_sema)

    p = sub.add_parser("dreamcast", add_help=False,
                       help="Dreamcast CodeView source-shape tools: "
                       "show / lines / asm / find / gaps / inline-clues / stats / structure / audit")
    p.add_argument("dreamcast_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=cmd_dreamcast)

    sub.add_parser("mac", add_help=False,
                   help="Classic Mac exact target: labels / show / disasm / diff / build")

    p = sub.add_parser("vc6", help="compiler model + solvers: argv / il-diff / "
                       "predict-inline / why-reg / oracle / check (homm3.vc6)")
    p.add_argument("vc6_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=cmd_vc6)

    p = sub.add_parser("hypotheses", help="compile and rank JSON source-hypothesis batches")
    p.add_argument("hypotheses_args", nargs=argparse.REMAINDER)
    p.set_defaults(fn=cmd_hypotheses)

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


def _select_image(argv: list[str]) -> list[str]:
    """Strip a leading `--image KEY` / `--image=KEY` and export it as
    $HOMM3_IMAGE before any module reads the image (homm3.core.images). Child
    processes inherit the selection."""
    from homm3.core import images
    key = None
    if argv[:1] == ["--image"] and len(argv) > 1:
        key, argv = argv[1], argv[2:]
    elif argv and argv[0].startswith("--image="):
        key, argv = argv[0].split("=", 1)[1], argv[1:]
    if key is not None:
        if key not in images.images(ROOT):
            print(f"[homm3] unknown image {key!r}; pinned images: "
                  f"{', '.join(images.images(ROOT))}", file=sys.stderr)
            raise SystemExit(1)
        os.environ[images.IMAGE_ENV] = key
    return argv


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    argv = _select_image(argv)
    from homm3.core import usage
    import shlex
    image = os.environ.get("HOMM3_IMAGE")
    shown = ["homm3", *([f"--image={image}"] if image and image != "game" else []), *argv]
    # Analysis rc=1 means an answered difference. Build/init and the other
    # pipeline commands use rc=1 for failure.
    failure_rc = 2 if argv and argv[0] in {"sema", "vc6", "dreamcast", "mac", "rmg", "victor"} else 1
    # `dreamcast audit` answers coverage gaps with 3 (gaps) or 4 (findings + gaps).
    audit = argv[:2] == ["dreamcast", "audit"]
    return usage.run_logged(
        _dispatch, argv,
        lambda rc, **meta: usage.append(ROOT / "build/homm3_usage.log",
                                       shlex.join(shown), rc, **meta),
        failure_rc=failure_rc, scope="cli",
        difference_codes=(3, 4) if audit else ())


if __name__ == "__main__":
    sys.exit(main())
