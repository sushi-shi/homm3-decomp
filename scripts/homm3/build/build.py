#!/usr/bin/env python3
"""homm3.build.build - the `homm3 build` command.

    configure -> ninja (base objs) -> delink (including normalization) -> objdiff report
    -> projected MAX summary -> [normal tier] checkpoint-ledger refresh + MAX loss report
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

from homm3.core import common, inputs

ROOT = common.HOMM3_DIR


def _run(*command: str) -> int:
    return subprocess.run(list(command), cwd=ROOT).returncode


def main(argv=None) -> int:
    argv = list(argv or [])
    fast = "--fast" in argv
    require_data_exact = '--require-data-exact' in argv
    ninja_args = [a for a in argv if a not in ('--fast', '--require-data-exact')]

    from homm3.build import configure, normalize_objs, data_checkpoint
    from homm3.match import status

    if fast and require_data_exact:
        print('[build] --require-data-exact requires a full build', file=sys.stderr)
        return 1

    if fast and not any((ROOT / "build/objdiff/target").glob("*.c.obj")):
        print("[build] retail targets missing; run `homm3 build` before `--fast`",
              file=sys.stderr)
        return 1

    if not fast:
        try:
            for executable in (inputs.RETAIL, inputs.DREAMCAST):
                inputs.stage_executable(executable)
        except inputs.InputError as exc:
            print(f"[build] {exc}", file=sys.stderr)
            return 1

    configure.configure()
    stale = data_checkpoint.schedule(ROOT, targets=ninja_args if fast else None)
    if _run("ninja", *ninja_args, *(u for u in stale if u not in ninja_args)):
        return 1
    if fast:
        normalize_objs.normalize_all()
        configure.configure()
    else:
        print("[build] refreshing retail targets")
        from homm3.build import delink
        delink.run()

    data_failures = []
    if not fast:
        try:
            data_failures = data_checkpoint.run(ROOT, require_exact=require_data_exact)
        except (ValueError, OSError, RuntimeError) as exc:
            data_failures = [f'data comparison unavailable: {exc}']
        for failure in data_failures:
            print(f'[build] {failure}', file=sys.stderr)

    report = status.refresh_report()
    fingerprint_pair = status.source_hash_pair()
    print(f"[build] {status.overall_line(report, fingerprint_pair=fingerprint_pair)}")
    print(f"[build] CUR diagnostic report: {status.REPORT.relative_to(ROOT)}")

    if fast:
        print("[build] fast: delink + data checkpoint + checkpoint ledger + gates + README skipped - "
              "run `homm3 build` before committing")
        return 0

    # A byte score is a checkpoint, not an admissibility invariant. Coherent
    # restoration of a Dreamcast-proven source shape may lower several local
    # scores before the surrounding class/TU reaches the retail lowering.
    # Preserve each implementation's MAX and the all-time HIST, but never make
    # a score regression fatal. Only a function whose own source hash changed
    # and whose new implementation MAX fell below the preceding MAX is worth
    # reporting; unrelated CUR dips leave MAX held and stay silent.
    # Check BEFORE updating the ledger so a changed function is compared with
    # its preceding MAX/source hash. The update then resets MAX for a proven
    # source edit while preserving HIST.
    history_patch = status.baseline_history()
    status.cmd_check(report, fingerprint_pair=fingerprint_pair)
    status.cmd_update(report, fingerprint_pair=fingerprint_pair, history_patch=history_patch)

    # Run every independent evidence/source gate, even after one fails.
    # Report unavailable evidence as fatal; dependent checks cannot certify it.
    # These gates, not a local objdiff maximum, decide admissibility.
    failed = bool(data_failures)

    # banked_rows runs alongside cmd_check, not inside it: the ratchet
    # compares the rows that ARE in the baseline, this one asks whether a
    # row that used to be there still is. Clean ratchet + lost row is
    # exactly how army::can_shoot left the ledger green (2026-08-15).
    from homm3.match import banked_rows, single_view, verify_va_claims, source_ownership, source_inventory
    origins = None
    for gate in (banked_rows, verify_va_claims, single_view, source_ownership, source_inventory):
        try:
            if gate is source_ownership:
                origins = source_ownership.read_dc(include_declarations=True)
                fatal = source_ownership.run_gate(origins=origins)
            elif gate is source_inventory:
                fatal = source_inventory.run_gate(origins=origins)
            elif gate is banked_rows:
                fatal = banked_rows.run_gate(history_patch=history_patch)
            else:
                fatal = gate.run_gate()
        except (inputs.InputError, OSError) as exc:
            fatal = [f'{gate.__name__}: evidence unavailable: {exc}']
        if fatal:
            failed = True
            for line in fatal:
                print(f"[build] {line}", file=sys.stderr)

    # Roll the cleanliness floors down only on an otherwise-green build:
    # a down-only bless recorded off a failing tree would bake in state
    # nobody reviewed. Check always, write only when clean.
    from homm3.cleanliness import board
    if origins is None:
        # Missing evidence must not look like an empty, clean DC inventory.
        violations = ['cleanliness: cannot check without Dreamcast origins; floors unchanged']
    else:
        violations = board.check_and_roll(write=not failed, dc_origins=origins)
    if violations:
        failed = True
        for line in violations:
            print(f"[build] {line}", file=sys.stderr)

    # Scores describe the completed compile/delink, including when an
    # independent source gate fails. Keep README consistent with the ledger.
    try:
        status.write_readme(report)
    except Exception as exc:  # the score block must never fail a build
        print(f"[build] README block skipped: {exc}")

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
