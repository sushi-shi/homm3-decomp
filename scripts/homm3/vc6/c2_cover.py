"""Function-entry coverage of C2 for locating optimizer passes.

`homm3 vc6 cover A.cpp B.cpp` compiles each scratch TU through the trace
shim with an INT3 on every known C2 function entry and prints the entries
whose hit counts differ. Two sources that differ in one optimization
opportunity (for example a mergeable tail) isolate the code that acts on it.
Counts are capped per entry (COVER_CAP in the shim), so heavily used
utilities saturate equally on both sides and drop out of the difference.
"""
from __future__ import annotations

import contextlib
import sys
from pathlib import Path

from homm3.core import cc_wrap
from homm3.vc6 import _common
from homm3.vc6.shim import build

COVER_ROOT = _common.REPO / "build/vc6/cover"
DEFAULT_FLAGS = ["/nologo", "/c", "/O2", "/Ob2", "/Oy-", "/Gr", "/GX"]


def parse_cover(text: str) -> dict[int, int]:
    counts = {}
    for line in text.splitlines():
        if line.startswith("cover "):
            _, rva, count = line.split()
            counts[int(rva, 16)] = int(count)
    return counts


def function_entries(path: Path) -> list[int]:
    entries = []
    for line in path.read_text().splitlines()[1:]:
        fields = line.split("\t")
        if fields and fields[0].startswith("0x"):
            entries.append(int(fields[0], 16))
    return sorted(set(entries))


def _default_functions() -> Path:
    from homm3.vc6.ghidra_scripts import import_c2
    local = import_c2.RAW_DIR / "c2_funcs_raw.tsv"
    if local.exists():
        return local
    _common.die(f"C2 function list {local} is missing; run `homm3 vc6 atlas` "
                "or pass --functions")


def cover(source: Path, entries: list[int], flags: list[str] | None = None) -> dict[int, int]:
    work = COVER_ROOT / source.stem
    work.mkdir(parents=True, exist_ok=True)
    table = work / "entries.txt"
    table.write_text("".join(f"{rva:x}\n" for rva in entries))
    log = work / "cover.log"
    log.unlink(missing_ok=True)
    out = work / f"{source.stem}.obj"
    process = build._cc_wrap(out, source, flags or DEFAULT_FLAGS, {
        "MSVC_DIR": str(build.OVERLAY_MSVC),
        "HOMM3_VC6_INLINE_TRACE": "",
        "HOMM3_VC6_COVER": cc_wrap.winepath_w(table),
        "HOMM3_VC6_SHIM_LOG": cc_wrap.winepath_w(log),
    })
    if process.returncode or not out.exists():
        _common.die(f"cover compile of {source} failed:\n{process.stdout}{process.stderr}")
    return parse_cover(log.read_text(encoding="latin1") if log.exists() else "")


def difference(a: dict[int, int], b: dict[int, int]) -> list[tuple[int, int, int]]:
    rows = [(rva, a.get(rva, 0), b.get(rva, 0)) for rva in sorted(set(a) | set(b))]
    return [row for row in rows if row[1] != row[2]]


def run(args) -> int:
    functions = Path(args.functions) if args.functions else _default_functions()
    entries = function_entries(functions)
    build._ensure_wine_env()
    with contextlib.redirect_stdout(sys.stderr):
        build.ensure_overlay()
    results = []
    try:
        with contextlib.redirect_stdout(sys.stderr):
            build.compile_shim(inlineTrace=True)
        for source in args.sources:
            results.append(cover(Path(source).resolve(), entries))
    finally:
        with contextlib.redirect_stdout(sys.stderr):
            build.compile_shim()
    print(f"[cover] {len(entries)} C2 entries; hit: "
          + ", ".join(f"{Path(s).name}={len(r)}" for s, r in zip(args.sources, results)))
    if len(results) < 2:
        for rva, count in sorted(results[0].items()):
            print(f"  0x{rva:05x} {count}")
        return 0
    base = results[0]
    for source, other in zip(args.sources[1:], results[1:]):
        print(f"[diff] {Path(args.sources[0]).name} -> {Path(source).name}")
        for rva, left, right in difference(base, other):
            print(f"  0x{rva:05x} {left:>6} -> {right:>6}")
    return 0
