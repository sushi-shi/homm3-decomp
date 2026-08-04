#!/usr/bin/env python3
"""homm3.match.status - the objdiff scoreboard and the match ratchet.

  homm3 status           per-unit table + overall (on-demand human view;
                         never printed by the build tail - gruntz doctrine)
  homm3 status update    raise per-function maxima from the current report
                         (monotone: never lowers a recorded best)
  homm3 status check     print every function below its recorded best;
                         --gate exits 1 on any drop (the BUILD runs this -
                         deliberate divergence from both siblings, where the
                         baseline is observational: here a regression fails
                         `homm3 build`. Lowering a max is an explicit hand
                         edit of config/match_baseline.tsv.)

Baseline: config/match_baseline.tsv, `unit<TAB>fn<TAB>max_fuzzy` (homm2's
shape minus the src_hash epoch column - per-function source hashes arrive
with the clang fingerprint path once game TUs compile; until then a max is
tied to the function name alone). The report is regenerated fresh each call
(`objdiff-cli report generate`, sub-second at this unit count; homm2's
input-identity cache is future work).
"""
from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

from homm3.carve import common

OBJDIFF_DIR = common.HOMM3_DIR / "build/objdiff"
REPORT = OBJDIFF_DIR / "report.json"
BASELINE = common.HOMM3_DIR / "config/match_baseline.tsv"
EPS = 0.01

BASELINE_HEADER = """\
# Per-function best-observed fuzzy match (the RATCHET).
# unit<TAB>fn<TAB>max_fuzzy - raised by `homm3 build` / `homm3 status
# update` (monotone); a drop below a recorded max FAILS the build
# (status check --gate). Lowering a max is an explicit hand edit here.
# No src_hash epoch column yet: it arrives with the clang fingerprint
# path when game TUs compile.
"""


def load_report() -> dict:
    executable = shutil.which("objdiff-cli")
    if not executable:
        common.die("objdiff-cli not found - enter the dev shell")
    result = subprocess.run([executable, "report", "generate",
                             "-o", "report.json"],
                            cwd=OBJDIFF_DIR, capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stdout + result.stderr)
        common.die("objdiff-cli report generate failed")
    return json.loads(REPORT.read_text())


def fn_fuzzy(report: dict) -> dict:
    """(unit, fn) -> fuzzy_match_percent, for every function in the report."""
    return {
        (unit.get("name", "?"), fn.get("name", "?")):
            float(fn.get("fuzzy_match_percent") or 0.0)
        for unit in report.get("units", [])
        for fn in (unit.get("functions", []) or [])
    }


def load_baseline() -> dict:
    maxima = {}
    if BASELINE.is_file():
        for line in BASELINE.read_text().splitlines():
            if not line or line.startswith("#"):
                continue
            unit, fn, value = line.split("\t")
            maxima[(unit, fn)] = float(value)
    return maxima


def write_baseline(maxima: dict) -> None:
    rows = [f"{unit}\t{fn}\t{value:.4f}"
            for (unit, fn), value in sorted(maxima.items())]
    BASELINE.write_text(BASELINE_HEADER + "\n".join(rows) + "\n")


def overall_line(report: dict) -> str:
    m = report.get("measures", {})
    total = int(m.get("total_functions") or 0)
    matched = int(m.get("matched_functions") or 0)
    fuzzy = float(m.get("fuzzy_match_percent") or 0.0)
    units = len(report.get("units", []))
    pct = 100.0 * matched / total if total else 0.0
    return (f"{matched}/{total} functions exact ({pct:.1f}%), "
            f"{fuzzy:.2f}% fuzzy across {units} unit(s)")


def cmd_summary(report: dict) -> int:
    print("  Unit        Funcs (exact)   Fuzzy")
    print("  " + "-" * 40)
    for unit in sorted(report.get("units", []),
                       key=lambda u: u.get("name", "")):
        m = unit.get("measures", {})
        total = int(m.get("total_functions") or 0)
        matched = int(m.get("matched_functions") or 0)
        fuzzy = float(m.get("fuzzy_match_percent") or 0.0)
        print(f"  {unit.get('name', '?'):<12} {matched:>3}/{total:<3}"
              f"      {fuzzy:>7.2f}%")
    print("  " + "-" * 40)
    print(f"  Overall: {overall_line(report)}")
    print(f"  Report: {REPORT}")
    return 0


def cmd_update(report: dict) -> int:
    maxima = load_baseline()
    raised = added = 0
    for key, value in fn_fuzzy(report).items():
        value = round(value, 4)  # the file stores 4 decimals - quantize so
        best = maxima.get(key)   # float tails never churn the baseline
        if best is None:
            maxima[key] = value
            added += 1
        elif value > best + 1e-9:
            maxima[key] = value
            raised += 1
    if raised or added:
        write_baseline(maxima)
    print(f"[status] baseline: {added} added, {raised} raised "
          f"-> {BASELINE.name} ({len(maxima)} rows)")
    return 0


def cmd_check(report: dict, gate: bool) -> int:
    maxima = load_baseline()
    if not maxima:
        print("[status] no baseline yet - run `homm3 status update`")
        return 0
    current = fn_fuzzy(report)
    drops = []
    for key, best in sorted(maxima.items()):
        value = current.get(key)
        if value is None:
            drops.append((key, best, None))
        elif value < best - EPS:
            drops.append((key, best, value))
    for (unit, fn), best, value in drops:
        now = f"{value:.2f}%" if value is not None else "MISSING"
        print(f"[status] REGRESSION {unit} {fn}: best {best:.2f}% -> {now}")
    if drops:
        print(f"[status] {len(drops)} function(s) below their recorded "
              "best. A genuine intentional drop is a hand edit of "
              f"{BASELINE.name}.")
        return 1 if gate else 0
    print(f"[status] ratchet clean ({len(maxima)} functions at or above "
          "their recorded best)")
    return 0


RM_START, RM_END = "<!-- match-score:start -->", "<!-- match-score:end -->"
README_PATH = common.HOMM3_DIR / "README.md"


def module_of(source: str) -> str:
    parts = Path(source).parts
    if parts and parts[0] == "vendor" and len(parts) > 1:
        return parts[1]
    if parts and parts[0] == "src":
        return "game"
    return parts[0] if parts else "?"


def _md_table(rows, align):
    widths = [max(len(r[i]) for r in rows) for i in range(len(rows[0]))]
    def fmt(row):
        cells = [c.ljust(w) if a == "l" else c.rjust(w)
                 for c, w, a in zip(row, widths, align)]
        return "| " + " | ".join(cells) + " |"
    sep = ["-" * w if a == "l" else "-" * (w - 1) + ":"
           for w, a in zip(widths, align)]
    out = [fmt(rows[0]), "| " + " | ".join(
        (":" + s[1:] if a == "l" else s)
        for s, a in zip(sep, align)) + " |"]
    out += [fmt(r) for r in rows[1:]]
    return out


def write_readme(report: dict) -> None:
    """Splice the per-module score table between the README sentinels
    (gruntz shape: Module | Units | Functions exact | Fuzzy | Fuzzy Max;
    Fuzzy Max weighs each function's best-ever from the baseline)."""
    from homm3.build.configure import load_manifest
    _build, _profiles, units = load_manifest()
    unit_module = {u["unit"]: module_of(u["source"]) for u in units}
    maxima = load_baseline()

    per_module = {}
    for unit in report.get("units", []):
        name = unit.get("name", "?")
        module = unit_module.get(name, name)
        agg = per_module.setdefault(
            module, {"units": 0, "fns": 0, "exact": 0, "wsum": 0.0,
                     "wmax": 0.0, "code": 0})
        agg["units"] += 1
        for fn in (unit.get("functions", []) or []):
            size = int(fn.get("size") or 0)
            fuzzy = float(fn.get("fuzzy_match_percent") or 0.0)
            best = max(fuzzy, maxima.get((name, fn.get("name", "?")), 0.0))
            agg["fns"] += 1
            agg["exact"] += fuzzy >= 100.0 - 1e-6
            agg["wsum"] += fuzzy * size
            agg["wmax"] += best * size
            agg["code"] += size

    rows = [["Module", "Units", "Functions exact", "Fuzzy", "Fuzzy Max"]]
    for module in sorted(per_module, key=lambda m: -per_module[m]["fns"]):
        a = per_module[module]
        pct = 100.0 * a["exact"] / a["fns"] if a["fns"] else 0.0
        fuzzy = a["wsum"] / a["code"] if a["code"] else 0.0
        fmax = a["wmax"] / a["code"] if a["code"] else 0.0
        rows.append([f"`{module}`", str(a["units"]),
                     f"{a['exact']} / {a['fns']} ({pct:.1f}%)",
                     f"{fuzzy:.2f}%", f"{fmax:.2f}%"])
    block = [RM_START, "", f"**Match score** — {overall_line(report)}.", ""]
    block += _md_table(rows, "lrrrr")
    block += ["", "_Function universe: the linked units only; the "
              "full-engine denominator and excluded-category tables arrive "
              "with the universe classifier._", "", RM_END]

    text = README_PATH.read_text()
    if RM_START in text and RM_END in text:
        head, rest = text.split(RM_START, 1)
        _old, tail = rest.split(RM_END, 1)
        new = head + "\n".join(block) + tail
    else:
        lines = text.splitlines(keepends=True)
        at = next((i for i, l in enumerate(lines)
                   if l.startswith("## ")), len(lines))
        new = "".join(lines[:at]) + "\n".join(block) + "\n\n" \
            + "".join(lines[at:])
    if new != text:
        README_PATH.write_text(new)
        print("[status] README match-score block refreshed")


def main(argv=None) -> int:
    argv = list(argv if argv is not None else sys.argv[1:])
    gate = "--gate" in argv
    readme = "--write-readme" in argv
    argv = [a for a in argv if a not in ("--gate", "--write-readme")]
    command = argv[0] if argv else "summary"
    report = load_report()
    if readme:
        write_readme(report)
    if command == "summary":
        return cmd_summary(report)
    if command == "update":
        return cmd_update(report)
    if command == "check":
        return cmd_check(report, gate)
    print(f"usage: homm3 status [update|check [--gate]] [--write-readme] "
          f"(got {command!r})", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
