#!/usr/bin/env python3
"""homm3.build.build - the `homm3 build` command.

    configure -> ninja (base objs) -> delink (including normalization) -> objdiff report
    -> overall line -> [normal tier] checkpoint-ledger refresh + dip report
    (OBSERVATIONAL) + banked-rows check (FATAL when a previously
    banked RVA left the baseline entirely) + cleanliness board (FATAL
    when a ratcheted source metric rises above its committed floor -
    C-style casts are banned at 0) + README score block.

Full builds refresh retail targets before comparison and checkpointing.
`--fast <TU>` keeps the existing targets, normalizes comparison copies, and
stops after the report. Run a full build first to establish those targets.
"""
from __future__ import annotations

import subprocess
import sys

from homm3.core import common

ROOT = common.HOMM3_DIR


def _run(*command: str) -> int:
    return subprocess.run(list(command), cwd=ROOT).returncode


def main(argv=None) -> int:
    argv = list(argv or [])
    fast = "--fast" in argv
    ninja_args = [a for a in argv if a != "--fast"]

    from homm3.build import configure, normalize_objs
    from homm3.match import status

    if fast and not any((ROOT / "build/objdiff/target").glob("*.c.obj")):
        print("[build] retail targets missing; run `homm3 build` before `--fast`",
              file=sys.stderr)
        return 1

    configure.main()
    if _run("ninja", *ninja_args):
        return 1
    if fast:
        if normalize_objs.main([]):
            return 1
        configure.main()
    else:
        print("[build] refreshing retail targets")
        from homm3.build import delink
        if delink.main([]):
            return 1

    report = status.load_report()
    print(f"[build] {status.overall_line(report)}")
    print(f"[build] report: {status.REPORT.relative_to(ROOT)}")

    if fast:
        print("[build] fast: delink + checkpoint ledger + gates + README skipped - "
              "run `homm3 build` before committing")
        return 0

    # A byte score is a checkpoint, not an admissibility invariant. Coherent
    # restoration of a Dreamcast-proven source shape may lower several local
    # scores before the surrounding class/TU reaches the retail lowering.
    # Preserve the peaks, but never make a score regression fatal or recommend
    # lowering the checkpoint to get a green build. Only a function whose own
    # source hash changed and whose score fell from its preceding current score
    # is worth reporting.
    # Check BEFORE updating the ledger so a changed function is compared with
    # its preceding current score/source hash. The update records this build,
    # ensuring an unchanged below-MAX function is not reported again.
    status.cmd_check(report)
    status.cmd_update(report)

    # EVERY evidence/source gate runs, even after one fails. Collect, report
    # everything, fail once; these gates, not a local objdiff maximum, decide
    # whether the reconstruction is admissible.
    failed = False

    # banked_rows runs alongside cmd_check, not inside it: the ratchet
    # compares the rows that ARE in the baseline, this one asks whether a
    # row that used to be there still is. Clean ratchet + lost row is
    # exactly how army::can_shoot left the ledger green (2026-08-15).
    from homm3.match import banked_rows, single_view, verify_va_claims
    for gate in (banked_rows, verify_va_claims, single_view):
        fatal = gate.run_gate()
        if fatal:
            failed = True
            for line in fatal:
                print(f"[build] {line}", file=sys.stderr)

    # Roll the cleanliness floors down only on an otherwise-green build:
    # a down-only bless recorded off a failing tree would bake in state
    # nobody reviewed. Check always, write only when clean.
    from homm3.cleanliness import board
    violations = board.check_and_roll(write=not failed)
    if violations:
        failed = True
        for line in violations:
            print(f"[build] {line}", file=sys.stderr)

    if failed:
        return 1
    try:
        status.write_readme(report)
    except Exception as exc:  # the score block must never fail a build
        print(f"[build] README block skipped: {exc}")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
