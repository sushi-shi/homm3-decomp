"""Compare existing compiler and delinker objects using the Gruntz workflow.

    homm3 compare [--reference previous-report.json] [--all-units]

Uses the same content-stamped normalization and objdiff project as build.
Reports current scores without compiling, delinking or checkpointing MAX.
Score changes are observations; operational failures return nonzero.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from homm3.core.paths import BUILD
from homm3.tool import ToolError
import json

BASE_DIR = BUILD / "objdiff/base"
TARGET_DIR = BUILD / "objdiff/target"
OUT_DIR = BUILD / "objdiff"


def _pct(measures: dict, key: str = "fuzzy_match_percent") -> float:
    return float(measures.get(key) or 0.0)


def functions(report: dict) -> dict[tuple[str, str], float]:
    """{(unit, symbol): fuzzy%} over a report's units."""
    out: dict[tuple[str, str], float] = {}
    for unit in report.get("units", []):
        name = unit.get("name", "")
        for fn in unit.get("functions", []):
            out[(name, fn.get("name", ""))] = _pct(fn)
    return out


def diff_reports(reference: dict, current: dict) -> dict:
    """Per-function score movement, keyed by unit + symbol name."""
    old, new = functions(reference), functions(current)
    shared = old.keys() & new.keys()
    equal = [k for k in shared if new[k] == old[k]]
    improved = [k for k in shared if new[k] > old[k]]
    regressed = sorted(k for k in shared if new[k] < old[k])
    return {
        "equal": len(equal),
        "improved": sorted(improved, key=lambda k: new[k] - old[k]),
        "regressed": sorted(regressed, key=lambda k: new[k] - old[k]),
        "appeared": sorted(new.keys() - old.keys()),
        "disappeared": sorted(old.keys() - new.keys()),
        "old": old,
        "new": new,
    }


def print_summary(report: dict, *, all_units: bool = False) -> None:
    m = report.get("measures", {})
    units = sorted(report.get("units", []), key=lambda u: (_pct(u["measures"]), u["name"]))
    shown = units if all_units else [u for u in units if _pct(u["measures"]) < 100.0]
    print(f"\n{'unit':<32} {'fuzzy%':>8} {'fns':>6} {'matched':>8} {'code':>9}")
    print("-" * 68)
    for u in shown:
        um = u["measures"]
        print(f"{u['name']:<32} {_pct(um):>8.2f} "
              f"{um.get('total_functions', 0):>6} "
              f"{um.get('matched_functions', 0):>8} "
              f"{um.get('total_code', 0):>9}")
    if not all_units:
        print(f"... and {len(units) - len(shown)} unit(s) at 100.00%")
    print("-" * 68)
    print(f"overall fuzzy {_pct(m):.5f}%  "
          f"functions {m.get('matched_functions', 0)}/{m.get('total_functions', 0)} "
          f"({_pct(m, 'matched_functions_percent'):.2f}%)  "
          f"code {m.get('matched_code', 0)}/{m.get('total_code', 0)} "
          f"({_pct(m, 'matched_code_percent'):.2f}%)  "
          f"units {m.get('total_units', 0)}")


def print_reference_diff(reference: dict, current: dict) -> dict:
    d = diff_reports(reference, current)
    old, new = d["old"], d["new"]
    ref_m, cur_m = reference.get("measures", {}), current.get("measures", {})
    print("\n=== vs reference ===")
    print(f"overall fuzzy   old {_pct(ref_m):.5f}%   new {_pct(cur_m):.5f}%   "
          f"delta {_pct(cur_m) - _pct(ref_m):+.5f}")
    print(f"functions       old {ref_m.get('total_functions', 0)}   "
          f"new {cur_m.get('total_functions', 0)}")
    print(f"equal {d['equal']}  improved {len(d['improved'])}  "
          f"regressed {len(d['regressed'])}  appeared {len(d['appeared'])}  "
          f"disappeared {len(d['disappeared'])}")
    if d["regressed"]:
        print(f"\n--- regressed ({len(d['regressed'])}) ---")
        print(f"{'unit':<28} {'symbol':<58} {'old%':>8} {'new%':>8} {'delta':>8}")
        for key in d["regressed"]:
            unit, symbol = key
            print(f"{unit:<28} {symbol:<58} {old[key]:>8.2f} {new[key]:>8.2f} "
                  f"{new[key] - old[key]:>+8.2f}")
    return d


def run(*, reference: Path | None = None, all_units: bool = False,
        quiet: bool = False) -> dict:
    """Normalize, pair and report the existing objects; never compile or bank MAX."""
    from homm3.build import configure, normalize_objs
    from homm3.match import status
    normalize_objs.normalize_all()
    configure.configure()
    report = status.refresh_report()
    if not quiet:
        print_summary(report, all_units=all_units)
        if reference is not None:
            print_reference_diff(json.loads(reference.read_text()), report)
    return report


def main() -> int:
    import sys
    ap = argparse.ArgumentParser(
        prog="homm3 compare", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--reference", type=Path,
                    help="an earlier report.json to diff per-function scores against")
    ap.add_argument("--all-units", action="store_true",
                    help="list every unit, not only those below 100%%")
    a = ap.parse_args()
    try:
        run(reference=a.reference, all_units=a.all_units)
    except ToolError as e:
        print(f"[compare] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
