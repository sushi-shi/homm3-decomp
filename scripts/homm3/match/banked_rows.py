#!/usr/bin/env python3
"""homm3.match.banked_rows - a banked function may never leave the ledger.

THE DEFECT THIS EXISTS FOR (2026-08-15, army::can_shoot). A clean ratchet
is necessary but not sufficient: it only compares rows that are IN
`config/match_baseline.tsv` against the current report. It cannot see a
row that left the file. Two ways one leaves:

  * `homm3 status update` retires it - `update_rows` drops a previous row
    whose RVA is claimed by the current label map, which is the right
    behaviour for a rename and the wrong one for a disappearance;
  * a MERGE of the generated TSV resolves to a side that never had it.
    That is what actually happened: lane/army-6 forked before
    lane/townmgr-14 banked `?can_shoot@army@@QBEEPBV1@@Z` at 83.2927, the
    integration took army-6's baseline, and the row was gone. Every gate
    stayed green, because after the merge there was nothing left to
    compare against.

THE IDENTITY IS THE RETAIL RVA, NEVER THE LABEL. Const-qualification and
source-label promotion rename functions while the body stays exactly
where it is - fifteen such renames landed in that one integration - so a
label-keyed check would report a flood of false losses and be turned off
within a day. An RVA-keyed check reports none of them: the renamed row
still carries the same address.

WHAT IT DOES. It walks the tracked revisions of the baseline (read-only
`git log -p`; the status writer stays the sole TSV writer), collects
every retail RVA that has ever carried a positive banked score, and
fails when one of them is no longer represented by ANY row in the
current baseline. A deliberate withdrawal - a claim proven wrong, an
address corrected - is admitted by hand in
`config/match-banked-waivers.tsv` with its reason, the same shape as the
VA-claim backlog.

Runs its embedded negative control on every invocation: the gate proves
it can still detect a deliberately removed row before it judges the tree.
"""
from __future__ import annotations

import subprocess
import sys

from homm3.core import common
from homm3.match.status import BASELINE, MatchRow, load_baseline

WAIVERS = common.HOMM3_DIR / "config/match-banked-waivers.tsv"


def parse_history(patch_text: str) -> dict[int, tuple[float, str, str]]:
    """rva -> (best banked score ever, unit, label) from `git log -p` output.

    Only added lines in the six-column cur/max/hist/rva format count. Rows
    without an RVA (the legacy flat-name format) have no stable identity and
    are skipped - the ratchet's own retirement rule already covers them.
    `git log -p` prints newest first, so the first label seen for an RVA is
    the most recent one and is the one worth naming in a failure.
    """
    history: dict[int, tuple[float, str, str]] = {}
    for line in patch_text.splitlines():
        if not line.startswith("+") or line.startswith("+++"):
            continue
        cols = line[1:].split("\t")
        if len(cols) != 6 or cols[5] == "-":
            continue
        try:
            # max OR hist: a row accepted down to zero still had an
            # achievement, and losing it is the same defect.
            best = max(float(cols[3]), float(cols[4]))
            rva = int(cols[5], 0)
        except ValueError:
            continue
        previous = history.get(rva)
        if previous is None:
            history[rva] = (best, cols[0], cols[1])
        elif best > previous[0]:
            history[rva] = (best, previous[1], previous[2])
    return history


def missing_rows(history: dict[int, tuple[float, str, str]],
                 current: dict[tuple[str, str], MatchRow],
                 waived: set[int]) -> list[tuple[int, float, str, str]]:
    """Historically banked RVAs with no row left in the current baseline."""
    present = {row.rva for row in current.values() if row.rva is not None}
    return sorted((rva, best, unit, label)
                  for rva, (best, unit, label) in history.items()
                  if best > 0.0 and rva not in present and rva not in waived)


def history_from_git() -> dict[int, tuple[float, str, str]]:
    relative = BASELINE.relative_to(common.HOMM3_DIR)
    result = subprocess.run(
        ["git", "log", "-p", "--format=", "--", str(relative)],
        cwd=common.HOMM3_DIR, capture_output=True, text=True)
    if result.returncode != 0:
        return {}
    return parse_history(result.stdout)


def load_waivers() -> dict[int, str]:
    """rva -> reason, for rows withdrawn on purpose (hand-maintained)."""
    waivers: dict[int, str] = {}
    if not WAIVERS.is_file():
        return waivers
    for line in WAIVERS.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        cols = line.split("\t")
        if len(cols) < 2:
            common.die(f"malformed {WAIVERS.name} row: {line!r}")
        try:
            waivers[int(cols[0], 0)] = cols[1]
        except ValueError:
            common.die(f"malformed {WAIVERS.name} rva: {line!r}")
    return waivers


def selftest() -> list[str]:
    """The negative control: prove the gate still detects a removed row."""
    failures = []
    patch = (
        "+++ b/config/match_baseline.tsv\n"
        "+army\t?can_shoot@army@@QBEEPBV1@@Z\t83.2927\t83.2927\t83.2927"
        "\t0x428f0\n"
        "+army\t?is_enemy@army@@QBEEPBV1@@Z\t100.0000\t100.0000\t100.0000"
        "\t0x42880\n"
        "+army\tflat_never_matched\t0.0000\t0.0000\t0.0000\t0x99999\n"
        "+army\tlegacy_no_rva\t50.0000\t50.0000\t50.0000\t-\n"
        "-army\tsomething_removed\t1.0\t1.0\t1.0\t0x11111\n")
    history = parse_history(patch)
    if set(history) != {0x428F0, 0x42880, 0x99999}:
        failures.append(f"history parse wrong: {sorted(history)}")
    if history.get(0x428F0, (0,))[0] != 83.2927:
        failures.append("banked score not carried")

    kept = {("army", "?is_enemy@army@@QBEEPBV1@@Z"):
            MatchRow(100.0, 100.0, 100.0, 0x42880)}

    # 1. THE DEFECT ITSELF: can_shoot's row is gone from the baseline
    #    while its RVA is claimed by nobody. Must be reported.
    lost = missing_rows(history, kept, set())
    if [row[0] for row in lost] != [0x428F0]:
        failures.append(f"removed row not detected: {lost}")
    if lost and lost[0][1] != 83.2927:
        failures.append("removed row reported with the wrong score")

    # 2. NEGATIVE CONTROL A - a label rename at the SAME RVA is not a
    #    loss. This is the case that would make a label-keyed gate useless.
    renamed = dict(kept)
    renamed[("army", "?can_shoot@army@@QBEEPBV1@@Z_const_promoted")] = \
        MatchRow(92.0, 92.0, 92.0, 0x428F0)
    if missing_rows(history, renamed, set()):
        failures.append("same-RVA rename wrongly reported as a loss")

    # 3. NEGATIVE CONTROL B - a row that never scored above zero carries
    #    no achievement, and a legacy row with no RVA has no identity.
    if any(row[0] in (0x99999,) for row in lost):
        failures.append("never-matched row wrongly reported")

    # 4. NEGATIVE CONTROL C - an explicit waiver silences exactly one RVA.
    if missing_rows(history, kept, {0x428F0}):
        failures.append("waiver did not silence the row")
    if not missing_rows(history, kept, {0x12345}):
        failures.append("unrelated waiver wrongly silenced the row")
    return failures


def run_gate() -> list[str]:
    broken = selftest()
    if broken:
        return [f"banked-rows SELFTEST BROKEN: {b}" for b in broken]

    history = history_from_git()
    if not history:
        print("[build] banked-rows: no tracked baseline history - skipped")
        return []
    waivers = load_waivers()
    lost = missing_rows(history, load_baseline(), set(waivers))
    if lost:
        fatal = ["banked-rows: a previously banked function left the ledger "
                 "without its RVA being reclaimed:"]
        for rva, best, unit, label in lost:
            fatal.append(f"  LOST 0x{rva:06x} {unit} {label} "
                         f"(banked {best:.4f}%)")
        fatal.append("  Restore the row, or admit the withdrawal in "
                     f"{WAIVERS.relative_to(common.HOMM3_DIR)} with a reason.")
        return fatal
    banked = sum(1 for value in history.values() if value[0] > 0.0)
    summary = f"[build] banked-rows: {banked} historically banked RVA(s)"
    if waivers:
        summary += f", {len(waivers)} waived"
    print(summary + " - none lost")
    return []


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--selftest" in argv:
        broken = selftest()
        for line in broken:
            print(f"SELFTEST BROKEN: {line}", file=sys.stderr)
        print("selftest OK" if not broken else "selftest FAILED")
        return 2 if broken else 0
    fatal = run_gate()
    for line in fatal:
        print(line, file=sys.stderr)
    return 1 if fatal else 0


if __name__ == "__main__":
    sys.exit(main())
