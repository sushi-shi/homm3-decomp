#!/usr/bin/env python3
"""Browse Dreamcast source-line attributions and local declarations.

Recorded lines describe an older optimized build. Gaps do not count missing
statements and line counts must not be compared with candidate /Z7 output.
Use declarations, line groups and file switches as positive source evidence.
See docs/matching/dc-line-tables.md for interpretation.
"""
from __future__ import annotations

import argparse
import re
import sys
from collections import OrderedDict, defaultdict

from homm3.core import common, inputs

SRC_DIR = common.HOMM3_DIR / "src"

# `VA(0x005dda10, 0x145F)  // <evidence>, dc 0x17f54c`. Evidence may also
# mention a Dreamcast byte size earlier on the same line; greedily consume the
# comment so the final explicit `dc 0x...` identity wins.
CLAIM_RE = re.compile(r"\b(?:VA|VA_COMPGEN)\s*\(\s*(0x[0-9a-fA-F]+)"
                      r"[^)]*\)[^\n]*\bdc\s+(0x[0-9a-fA-F]+)")

_srclines: dict[str, list[tuple[str, int, int]]] = {}


def _load_srclines() -> dict[str, list[tuple[str, int, int]]]:
    """module.obj -> [(file, line, addr)], parsed once."""
    if _srclines:
        return _srclines
    _srclines.update(inputs.dreamcast_symbols().source_lines)
    return _srclines


def _load_functions() -> list[dict]:
    from homm3.analysis.dc_extract import corpus_rows
    return corpus_rows()[0]


def _load_locals(proc: str, module: str) -> list[dict]:
    from homm3.analysis.dc_extract import corpus_rows
    return [row for row in corpus_rows()[1]
            if row["proc"] == proc and row["module"] == module]


def _va_index() -> dict[int, tuple[str, int]]:
    """retail VA -> (module.obj, dc offset), harvested from the claims."""
    index = {}
    for path in sorted(SRC_DIR.glob("*.cpp")):
        module = path.stem + ".obj"
        for va, dc in CLAIM_RE.findall(path.read_text(errors="replace")):
            index[int(va, 16)] = (module, int(dc, 16))
    return index


def census(module: str, lo: int, cb: int) -> "OrderedDict[str, list[tuple[int, int]]]":
    """file -> sorted [(line, first addr)] inside [lo, lo+cb)."""
    rows = [r for r in _load_srclines().get(module, ())
            if lo <= r[2] < lo + cb]
    rows.sort(key=lambda r: r[2])
    first: dict[tuple[str, int], int] = {}
    order: "OrderedDict[str, None]" = OrderedDict()
    for f, ln, addr in rows:
        order.setdefault(f, None)
        first.setdefault((f, ln), addr)
    out: "OrderedDict[str, list[tuple[int, int]]]" = OrderedDict()
    for f in order:
        out[f] = sorted((ln, a) for (ff, ln), a in first.items() if ff == f)
    return out


def _basename(path: str) -> str:
    return path.rsplit("\\", 1)[-1].rsplit("/", 1)[-1]


def _report(row: dict, args) -> None:
    module, off, cb = row["module"], int(row["offset"], 16), int(row["cb"])
    by_file = census(module, off, cb)
    own = _basename(row["file"]).lower()
    total = sum(len(v) for v in by_file.values())
    own_lines = sum(len(v) for f, v in by_file.items()
                    if _basename(f).lower() == own)
    print(f"{row['name']}")
    print(f"  {module} dc {off:#x} cb={cb} ({cb} B SH4)  "
          f"boundary-line {_basename(row['file'])}:{row['line']}  "
          f"params={row['params']} locals={row['locals']}")
    print(f"  LINES: {own_lines} in the owning .cpp, {total} total "
          f"across {len(by_file)} file(s)")
    for f, v in by_file.items():
        # The S_GPROC32 extent is the boundary.  A different source file that
        # intersects it is inline-expansion residue; disjoint header ranges
        # elsewhere in the module remain ordinary out-of-line COMDATs.
        tag = ("  <-- body" if _basename(f).lower() == own
               else "  <-- inline source inside body")
        print(f"      {_basename(f):<30} {len(v):>4} lines "
              f"[{v[0][0]}..{v[-1][0]}]{tag}")
        if args.lines:
            for ln, a in v:
                print(f"          {ln:>6}  {a:#010x}")
    if args.locals:
        rows = _load_locals(row["name"], module)
        params = [r for r in rows if r["kind"] == "param"]
        locs = [r for r in rows if r["kind"] != "param"]
        print(f"  LOCALS: {len(params)} param(s), {len(locs)} local(s)")
        for r in params + locs:
            print(f"      {r['kind']:<6} {r['sp_offset']:>10}  "
                  f"{r['type']:<28} {r['name']}")
    print()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm3.analysis.dc_srclines",
                                 description=__doc__.split("\n")[0])
    ap.add_argument("selector", nargs="*",
                    help="NAME | module.obj:0xOFF | retail 0xVA")
    ap.add_argument("--unit", help="report a whole module.obj")
    ap.add_argument("--top", type=int, default=0,
                    help="with --unit: only the N largest bodies")
    ap.add_argument("--lines", action="store_true",
                    help="print every line number with its first address")
    ap.add_argument("--locals", action="store_true",
                    help="also print the proc's CodeView locals")
    args = ap.parse_args(argv)


    fns = _load_functions()
    by_key = {(r["module"], int(r["offset"], 16)): r for r in fns}
    by_name = defaultdict(list)
    for r in fns:
        by_name[r["name"]].append(r)

    selected: list[dict] = []
    if args.unit:
        rows = [r for r in fns if r["module"] == args.unit]
        rows.sort(key=lambda r: -int(r["cb"]))
        selected += rows[:args.top] if args.top else rows

    va_index = None
    for spec in args.selector:
        if ":" in spec:
            module, off = spec.split(":", 1)
            row = by_key.get((module, int(off, 16)))
            if row is None:
                print(f"{spec}: no roster row", file=sys.stderr)
                continue
            selected.append(row)
            continue
        if re.fullmatch(r"0x[0-9a-fA-F]+", spec):
            if va_index is None:
                va_index = _va_index()
            hit = va_index.get(int(spec, 16))
            if hit is None:
                print(f"{spec}: no claim in src/ carries this VA with a "
                      f"`dc 0x...` tag", file=sys.stderr)
                continue
            row = by_key.get(hit)
            if row is None:
                print(f"{spec}: claim names {hit[0]} dc {hit[1]:#x}, "
                      f"absent from the DC roster", file=sys.stderr)
                continue
            selected.append(row)
            continue
        hits = by_name.get(spec) or [r for r in fns if spec in r["name"]]
        if not hits:
            print(f"{spec}: no proc matches", file=sys.stderr)
            continue
        selected += hits

    if not selected:
        ap.print_usage(sys.stderr)
        return 2
    for row in selected:
        _report(row, args)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
