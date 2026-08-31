#!/usr/bin/env python3
"""homm3.analysis.dc_srclines - the Dreamcast STATEMENT CENSUS.

The problem this solves: when a reconstruction plateaus, the first
question is always "is my body line-complete, or am I missing
statements?". Guessing at it from a dead-store titration confuses
byte-inert MASS with statement count and invents phantom deficits
(THallWindow's "50-90 missing statements", 2026-08-14).

The dump's `*** SRCLINES ***` section answers it directly. Per module it
lists, per contributing source FILE, a table of `line addr` pairs. Slice
those by a proc's own extent `[offset, offset + cb)` and you have the DC
build's source-line inventory for that body: how many DISTINCT lines of
the owning .cpp its optimizer attributed code to.

WHAT IT IS GOOD FOR - and what it is NOT.

CALIBRATED 2026-08-14 over all 807 exact functions carrying a `dc 0x`
map: the ratio of our statement-line count to the DC distinct-line count
has median 0.933, quartiles 0.733 / 1.083, and only 29% of exact bodies
land within +-10% of 1.0. That makes it a **+-25% instrument** - about
+-18 statements on a 71-line body.

  * USE IT AS A DISQUALIFIER. "Is the DC row even the same function?"
    and "does this body have a whole structural limb my reconstruction
    is missing?" are presence/absence questions, and on those it is
    sharp and free. It retired one plateau in a single query when a
    `dc 0x...` map turned out to name an entirely different
    six-argument function, and it closed THallWindow by showing the
    body was line-COMPLETE (98 spelled against 98 predicted) when a
    dead-store titration claimed it was 50-90 statements short.
  * DO NOT USE IT AS A MASS METER. It cannot price a 20-statement
    effect; anything inside +-25% is noise. An earlier 249-body
    calibration that reported median 1.000 was a small-sample
    artefact - do not resurrect it.
  * The "which header lines were inlined into this body" reading DOES
    NOT WORK. NB11 has no inlinee-line records: inlined code is
    attributed to the CALL SITE's own .cpp line. Header files that do
    appear in a module's SRCLINES are separate out-of-line COMDATs at
    disjoint addresses, which is why they never intersect a proc's
    extent. Absence of a callee's lines is therefore NOT evidence that
    the DC build called it out of line.

Further caveats, all of them real:
  * DC line counts are the DC PRESSING's source. Retail is a later build
    (RoE vs Complete) - a case block added downstream shows up as extra
    retail lines. Scale by the structure you can see (THallWindow: one
    more switch case = +7 lines, one more table row = +2) before
    comparing.
  * A PLATFORM DELTA has to be subtracted the same way the call census
    needs it: SH4-only helpers (and WinCE-only code) contribute lines
    that x86 never had.
  * `cb` is the DC size. It bounds the DC body only; retail sizes come
    from the carve and never from here.
  * Lines are attributed by the DC's own optimizer, so a line that
    survives to zero instructions is absent. The count is therefore a
    LOWER bound on statements, which is exactly how to read it.

`--locals` adds the companion oracle: the proc's S_REGREL32 locals as
extracted into evidence/dreamcast/variables.csv (offset, type, name).
That is retail's source LOCAL inventory - also a lower bound, and needing
the same platform-delta subtraction.

Selectors (any mix):
    NAME                     match a proc by (sub)name across all modules
    module.obj:0xOFF         one proc by DC offset
    0x5dda10                 a RETAIL VA - resolved through the `dc 0x...`
                             tag on that address's VA()/DC_ONLY() claim in
                             src/<unit>.cpp

  python3 -m homm3.analysis.dc_srclines townmgr.obj:0x1793b4
  python3 -m homm3.analysis.dc_srclines 0x5dda10 --lines --locals
  python3 -m homm3.analysis.dc_srclines SetupThievesGuild
  python3 -m homm3.analysis.dc_srclines --unit townmgr.obj --top 20

ANALYSIS OUTPUT over another pressing's debug info: a line count is
evidence about SOURCE SHAPE, never about a retail address or size.
"""
from __future__ import annotations

import argparse
import csv
import re
import sys
from collections import OrderedDict, defaultdict

from homm3.core import common

DUMP = common.HOMM3_DIR.parent / "homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt"
FUNCTIONS = common.EVIDENCE_DIR / "dreamcast/functions.csv"
VARIABLES = common.EVIDENCE_DIR / "dreamcast/variables.csv"
SRC_DIR = common.HOMM3_DIR / "src"

MODULE_RE = re.compile(r"\*\*\* Module .*[\\/]([^\\/]+\.obj) at ")
FILE_RE = re.compile(r"^\s+(\S+), \d{4}:([0-9A-F]{8})-([0-9A-F]{8}), "
                     r"line/addr pairs = (\d+)")
PAIR_RE = re.compile(r"(\d+)\s+([0-9A-F]{8})")
# `VA(0x005dda10, 0x145F)  // <evidence>, dc 0x17f54c`. Evidence may also
# mention a Dreamcast byte size earlier on the same line; greedily consume the
# comment so the final explicit `dc 0x...` identity wins.
CLAIM_RE = re.compile(r"\b(?:VA|VA_COMPGEN|DC_ONLY)\s*\(\s*(0x[0-9a-fA-F]+)"
                      r"[^)]*\)[^\n]*\bdc\s+(0x[0-9a-fA-F]+)")

_srclines: dict[str, list[tuple[str, int, int]]] = {}


def _load_srclines() -> dict[str, list[tuple[str, int, int]]]:
    """module.obj -> [(file, line, addr)], parsed once."""
    if _srclines:
        return _srclines
    text = DUMP.read_text(errors="replace")
    lo = text.index("*** SRCLINES ***")
    hi = text.index("*** SEGMENT MAP", lo)
    module = None
    cur_file = None
    rows: list[tuple[str, int, int]] = []
    for line in text[lo:hi].splitlines():
        m = MODULE_RE.search(line)
        if m:
            module = m.group(1)
            cur_file = None
            rows = _srclines.setdefault(module, [])
            continue
        if module is None:
            continue
        m = FILE_RE.match(line)
        if m:
            cur_file = m.group(1)
            continue
        if cur_file and re.match(r"^\s+\d+ [0-9A-F]{8}", line):
            for number, addr in PAIR_RE.findall(line):
                rows.append((cur_file, int(number), int(addr, 16)))
    return _srclines


def _load_functions() -> list[dict]:
    with FUNCTIONS.open() as fh:
        return list(csv.DictReader(l for l in fh if not l.startswith("#")))


def _load_locals(proc: str, module: str) -> list[dict]:
    out = []
    with VARIABLES.open() as fh:
        for row in csv.DictReader(l for l in fh if not l.startswith("#")):
            if row["proc"] == proc and row["module"] == module:
                out.append(row)
    return out


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
        # A second file inside a proc's extent is a separate out-of-line
        # COMDAT the linker packed adjacently, NOT an inline expansion -
        # NB11 attributes inlined code to the call site's own line.
        tag = "  <-- body" if _basename(f).lower() == own else "  (adjacent)"
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

    if not DUMP.exists():
        common.die(f"Dreamcast dump not found: {DUMP}")
    if not FUNCTIONS.exists():
        common.die(f"{FUNCTIONS} missing - run homm3.analysis.dc_extract")

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
