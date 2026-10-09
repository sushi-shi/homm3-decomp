"""homm3 loki-game: the Loki Linux game 1.3.1a as evidence for the Windows game.

  init [--exe PATH]
        verify and stage heroes3.dynamic (HOMM3_LOKI_HEROES3) at build/orig/loki/
  census
        headless Ghidra census: functions, calls and data references
        (build/heroes3-loki/census/; Java runs with -Djava.awt.headless=true)
  profile [TAG ...]
        compile the proof units (config/loki/game.toml [proof]) under each flag
        variant with the staged GCC 2.95.2 (`homm3 loki toolchain`) and count
        the unique exact bodies found in the image
  pair [--no-fingerprint]
        pair Loki functions with Windows functions (homm3.loki_game.pair) and
        write config/retail/heroes3-loki/functions.tsv; prints counts by evidence
  vtables
        write config/retail/heroes3-loki/vtables.tsv from the image's RTTI
  calls WIN_RVA [LOKI_ADDRESS]
        ordered callees of a paired function on both sides, Loki callees
        annotated with their Windows pair and whether Windows calls it
  compile UNIT ... [--scan]
        compile src/UNIT.cpp at the game profile (build/heroes3-loki/objects/)
        and, with --scan, search its bodies in the image
  diff WIN_RVA [--no-compile]
        compile the function's Windows unit and list its body beside the
        paired Loki body (normal form of homm3.loki_game.diff) with a score
  score [UNIT ...] [--below-100] [--no-compile] [--all]
        the Loki game score: paired Windows functions whose Loki body the
        units reproduce (exact / found / paired); default units are those
        with a paired function, --below-100 restricts to the functions not
        exact on Windows; --all lists every row
"""
from __future__ import annotations

import argparse
import sys


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 loki-game", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("init")
    p.add_argument("--exe")
    sub.add_parser("census")
    p = sub.add_parser("profile")
    p.add_argument("tags", nargs="*")
    p = sub.add_parser("pair")
    p.add_argument("--no-fingerprint", action="store_true")
    sub.add_parser("vtables")
    p = sub.add_parser("compile")
    p.add_argument("units", nargs="+")
    p.add_argument("--scan", action="store_true")
    p = sub.add_parser("diff")
    p.add_argument("win")
    p.add_argument("--no-compile", action="store_true")
    p = sub.add_parser("score")
    p.add_argument("units", nargs="*")
    p.add_argument("--below-100", action="store_true")
    p.add_argument("--no-compile", action="store_true")
    p.add_argument("--all", action="store_true")
    p.add_argument("--readme", action="store_true")
    p = sub.add_parser("calls")
    p.add_argument("win")
    p.add_argument("loki", nargs="?")
    args = parser.parse_args(argv)
    try:
        if args.command == "init":
            from homm3.loki_game.image import stage
            print(f"[loki-game] image: {stage(args.exe)}")
        elif args.command == "census":
            from homm3.loki_game import census
            print(f"[loki-game] census: {census.run()}")
        elif args.command == "profile":
            from homm3.loki_game import scan
            variants = scan.profile_spec()["proof"]["variants"]
            if args.tags:
                variants = {t: variants[t] for t in args.tags}
            scan.profile(variants)
        elif args.command == "pair":
            from homm3.loki_game import pair
            pairing = pair.Pairing()
            counts = pairing.run([] if args.no_fingerprint else pair.default_objects())
            pairing.write()
            print(f"[loki-game] {sum(counts.values())} pairs: " +
                  ", ".join(f"{k} {v}" for k, v in counts.most_common()))
        elif args.command == "vtables":
            return _vtables()
        elif args.command == "compile":
            return _compile(args.units, args.scan)
        elif args.command == "diff":
            return _diff(int(args.win, 16), not args.no_compile)
        elif args.command == "score":
            return _score(args.units, args.below_100, not args.no_compile, args.all, args.readme)
        elif args.command == "calls":
            return _calls(int(args.win, 16), int(args.loki, 16) if args.loki else None)
    except (ValueError, OSError, RuntimeError, KeyError) as exc:
        print(f"[loki-game] ERROR: {exc}", file=sys.stderr)
        return 2
    return 0


def _compile(units: list[str], scan: bool) -> int:
    from homm3.loki_game import compile as cc
    rc = 0
    for unit in units:
        obj, diagnostics = cc.compile_unit(unit)
        errors = [line for line in diagnostics.splitlines() if ": " in line and "In file included" not in line]
        if not obj.is_file() or errors and "error" in diagnostics.lower():
            print(f"[loki-game] {unit}: compile failed")
            for line in errors[:20]:
                print("  " + line.replace(str(cc.ROOT) + "/", ""))
            rc = 1
            continue
        print(f"[loki-game] {unit}: {obj}")
        if scan:
            from homm3.loki_game.scan import Scanner
            for name, size, hits in Scanner().scan([obj]):
                where = f"0x{hits[0]:08x}" if len(hits) == 1 else f"{len(hits)} hits"
                print(f"  {size:6} {where:>12} {name}")
    return rc


def _objects(units: list[str], build: bool, scorer=None, rows=None) -> dict[str, list]:
    """unit -> its object's functions; a unit that does not compile maps to None.
    With a scorer, each unit is compiled again with its scored functions
    placed at their image residues (compile.place_functions) and rescored."""
    from homm3.loki_game import compile as cc, diff
    from homm3.loki_game.image import BUILD
    out = {}
    for unit in units:
        obj = BUILD / "objects" / f"{unit}.o"
        if not (cc.ROOT / "src" / f"{unit}.cpp").is_file():
            out[unit] = None
            continue
        if build:
            obj, diagnostics = cc.compile_unit(unit)
            if not obj.is_file():
                (BUILD / "objects" / f"{unit}.log").write_text(diagnostics)
        out[unit] = diff.object_functions(obj) if obj.is_file() else None
        if out[unit] is None or scorer is None:
            continue
        unit_rows = [r for r in rows if r.unit == unit]
        scorer.score_unit(unit_rows, out[unit])
        placements = {r.mangled: r.loki for r in unit_rows if r.mangled}
        if build and placements:
            obj, _ = cc.compile_unit(unit, placements=placements)
            out[unit] = diff.object_functions(obj) if obj.is_file() else None
            if out[unit] is not None:
                scorer.score_unit(unit_rows, out[unit])
    return out


def _diff(win: int, build: bool) -> int:
    from homm3.loki_game import diff, score
    scorer = score.Scorer()
    rows = [r for r in scorer.rows() if r.win == win]
    if not rows:
        print(f"[loki-game] 0x{win:x} is not paired", file=sys.stderr)
        return 1
    row = rows[0]
    functions = _objects([row.unit], build, scorer, [row])[row.unit]
    if functions is None:
        print(f"[loki-game] {row.unit} does not compile (build/heroes3-loki/objects/{row.unit}.log)")
        return 1
    scorer.score_unit([row], functions)
    if row.symbol is None:
        print(f"[loki-game] {row.name}: {row.note}")
        return 1
    mine = next(f for f in functions if diff.gnu_demangle([f.symbol])[f.symbol] == row.symbol)
    target = scorer.body(row)
    for line in diff.side_by_side(mine.body, target):
        print(line)
    print(f"WIN 0x{row.win:x} {row.windows:.2f}  LOKI 0x{row.loki:08x}  {row.symbol}")
    print(f"loki score {row.score:.2f}{' (exact)' if row.exact else ''}"
          f"  ({len(mine.body.code)} vs {len(target.code)} bytes)")
    return 0


def _score(units: list[str], below: bool, build: bool, every: bool, readme: bool = False) -> int:
    from homm3.loki_game import score
    scorer = score.Scorer()
    rows = scorer.rows(set(units) if units else None)
    if below:
        rows = [r for r in rows if r.windows < 100]
    by_unit: dict[str, list] = {}
    for row in rows:
        by_unit.setdefault(row.unit, []).append(row)
    objects = _objects(sorted(by_unit), build, scorer, rows)
    compiled = 0
    for unit, unit_rows in sorted(by_unit.items()):
        if objects[unit] is None:
            for row in unit_rows:
                row.note = "unit does not compile"
            continue
        compiled += 1
        if not build:
            scorer.score_unit(unit_rows, objects[unit])
    for row in rows:
        if every or (row.score is not None and not row.exact) or below:
            score_text = "-" if row.score is None else f"{row.score:6.2f}"
            print(f"{row.unit:24} 0x{row.win:06x} {row.windows:6.2f} {score_text:>6} "
                  f"{'=' if row.exact else ' '} {(row.symbol or row.note)[:80]}")
    found = [r for r in rows if r.score is not None]
    exact = sum(r.exact for r in found)
    print(f"[loki-game] score: {exact} exact / {len(found)} found / {len(rows)} paired"
          f" ({compiled} of {len(by_unit)} units compile)")
    if readme:
        if units or below:
            print("[loki-game] --readme needs the whole score (no units, no --below-100)", file=sys.stderr)
            return 2
        walls = [r for r in rows if r.windows < 100]
        _write_readme(exact, len(found), len(rows), compiled, len(by_unit),
                      sum(r.exact for r in walls), sum(r.score is not None for r in walls), len(walls))
    return 0


README_BLOCK = ("<!-- loki-game-match-score:start -->", "<!-- loki-game-match-score:end -->")


def _write_readme(exact, found, paired, compiled, units, wall_exact, wall_found, walls) -> None:
    """The Loki game block of README.md (work/loki-game only), after the Mac one."""
    from homm3.loki_game import compile as cc
    path = cc.ROOT / "README.md"
    text = path.read_text()
    body = (f"{README_BLOCK[0]}\n\n**Loki Linux game `heroes3.dynamic` (evidence, GCC 2.95.2): "
            f"{exact:,} / {paired:,} paired functions exact** — {found:,} compiled from {compiled} of "
            f"{units} units; of the {walls} paired functions below 100% on Windows, {wall_exact} "
            f"exact ({wall_found} compiled). `homm3 loki-game score --readme`.\n\n{README_BLOCK[1]}")
    if README_BLOCK[0] in text:
        start = text.index(README_BLOCK[0])
        end = text.index(README_BLOCK[1]) + len(README_BLOCK[1])
        text = text[:start] + body + text[end:]
    else:
        anchor = "<!-- mac-match-score:end -->"
        at = text.index(anchor) + len(anchor)
        text = text[:at] + "\n\n" + body + text[at:]
    path.write_text(text)
    print("[loki-game] README.md Loki game block refreshed")


def _vtables() -> int:
    from homm3.loki_game import rtti
    from homm3.loki_game.image import RETAIL
    rows = ["# GENERATED by `homm3 loki-game vtables`: each vtable the RTTI type_info functions",
            "# anchor (zero word, __tf slot, virtual functions in slot order).",
            "address\ttype_name\ttf\tslots"]
    for v in sorted(rtti.vtables(), key=lambda v: v.address):
        rows.append(f"0x{v.address:08x}\t{v.type_name}\t0x{v.tf:08x}\t" +
                    ",".join(f"0x{s:08x}" for s in v.slots))
    (RETAIL / "vtables.tsv").write_text("\n".join(rows) + "\n")
    print(f"[loki-game] {len(rows) - 3} vtables")
    return 0


def _calls(win: int, loki: int | None) -> int:
    from homm3.loki_game import pair, refs
    from homm3.loki_game.image import LokiGame
    pairs = pair.load()
    W, K, ledger = refs.windows(), refs.loki(), refs.ledger()
    plt = LokiGame().plt
    k2w = {k: w for w, (k, _) in pairs.items()}
    if loki is None:
        if win not in pairs:
            print(f"[loki-game] 0x{win:x} is not paired", file=sys.stderr)
            return 1
        loki = pairs[win][0]

    def name(w):
        return ledger.get(w, {}).get("name", f"sub_{w:x}")
    print(f"WIN 0x{win:x} {name(win)} ({W[win].size} bytes)")
    for c in W[win].calls:
        tail = f"  = loki 0x{pairs[c][0]:x}" if c in pairs else ""
        print(f"  -> 0x{c:06x} {name(c)[:100]}{tail}")
    print(f"LOKI 0x{loki:x} ({K[loki].size} bytes)")
    called = set(W[win].calls)
    for c in K[loki].calls:
        if c in plt:
            print(f"  -> plt {plt[c]}")
            continue
        w = k2w.get(c)
        size = K[c].size if c in K else "?"
        if w is None:
            print(f"  -> 0x{c:x} ({size} bytes) unpaired")
        else:
            seen = "Windows calls it" if w in called else "Windows expands or omits it"
            print(f"  -> 0x{c:x} ({size} bytes) {name(w)[:90]} [{seen}]")
    return 0


def logged_main(argv=None) -> int:
    from homm3.core import paths
    from homm3.core.usage import append, run_logged
    import shlex
    argv = list(sys.argv[1:] if argv is None else argv)
    cmd = shlex.join(["homm3", "loki-game", *argv])
    return run_logged(main, argv,
                      lambda rc, **meta: append(paths.SHARED_BUILD / "homm3_usage.log",
                                                cmd, rc, **meta))


if __name__ == "__main__":
    raise SystemExit(logged_main())
