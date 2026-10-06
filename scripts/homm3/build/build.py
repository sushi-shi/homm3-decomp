#!/usr/bin/env python3
"""homm3.build.build - the `homm3 build` command.

    configure -> ninja (VC6 base objs) -> delink (including normalization)
    -> objdiff report -> projected MAX summary -> [full build] checkpoint-
    ledger refresh + MAX loss report (OBSERVATIONAL) + candidate link +
    banked-rows check (FATAL when a previously banked RVA left the baseline
    entirely) + source evidence gates + cleanliness board (FATAL when a
    ratcheted source metric rises above its committed floor - C-style casts
    are banned at 0) + README score block.

Full builds refresh retail targets before comparison and checkpointing.
`homm3 build` is Windows-only: Classic Mac full-TU objects, pair scores and
their checkpoint belong to the separate `homm3 mac build [TU ...]`. `--data`
adds the complete byte accounting (data coverage), which otherwise keeps
its last README line.
`--fast <TU>` keeps the existing targets and stops after the report. It
normalizes and fingerprints only the selected units, so their projected MAX
movements equal the full projection; other units' MAX is held. Run a full
build first to establish those targets.
"""
from __future__ import annotations

import subprocess
import sys
import time

from homm3.build.normalized_freshness import ValidationContext
from homm3.core import common, inputs

ROOT = common.HOMM3_DIR


def _run(*command: str) -> int:
    return subprocess.run(list(command), cwd=ROOT).returncode


def _link() -> int:
    return _run(sys.executable, "-m", "homm3.build.link", "--out",
                "build/exe/HEROES3.candidate.EXE")


def _selected_units(ninja_args: list[str]) -> set[str]:
    """Manifest units named as Ninja targets; options and other targets select none."""
    from homm3 import manifest as units_manifest
    manifest_units = {unit["unit"] for unit in
                      units_manifest.load(ROOT / "config/units.toml")["unit"]}
    return {arg for arg in ninja_args if arg in manifest_units}


def main(argv=None) -> int:
    argv = list(argv or [])
    from homm3.core import worktree_lock
    # One mutating homm3 command per worktree: a second build, delink or
    # status update waits here instead of interleaving writes to build/.
    with worktree_lock.hold(" ".join(["homm3 build", *argv])):
        return _main(argv)


def _main(argv: list[str]) -> int:
    fast = "--fast" in argv
    data = "--data" in argv
    ninja_args = [a for a in argv if a not in ("--fast", "--data")]
    started = time.monotonic()

    def phase(name: str) -> None:
        print(f"[build] {name} ({time.monotonic() - started:.0f}s elapsed)", flush=True)

    from homm3.build import configure, normalize_objs
    from homm3.match import status

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

    selected = _selected_units(ninja_args) if fast else set()
    configure.configure()
    if _run("ninja", *ninja_args):
        return 1
    context = None
    if fast:
        # Ninja rebuilt only the selected units; normalize just those. Any
        # other stale comparison (a tool or manifest change) falls back to
        # the complete pass, exactly as before.
        normalize_objs.normalize_all(selected or None)
        if selected:
            context = ValidationContext()
            if status.comparison_problems(context):
                normalize_objs.normalize_all()
                context = None
        configure.configure()
    else:
        print("[build] refreshing retail targets")
        from homm3.build import delink
        delink.run()
        phase("compiled and delinked")

    report = status.refresh_report(context)
    # A fast build fingerprints only its selected units: their projection is
    # the full one, while unselected functions keep MAX (unknown hash).
    fingerprint_pair = status.source_hash_pair(only_units=selected or None)
    print(f"[build] {status.overall_line(report, fingerprint_pair=fingerprint_pair)}")
    print(f"[build] CUR diagnostic report: {status.REPORT.relative_to(ROOT)}")

    if fast:
        status.fast_max_movements(report, selected or None, fingerprint_pair)
        print("[build] fast: delink + checkpoint ledger + gates + README skipped - "
              "bank measured scores and regenerate README before committing")
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
    # Per-TU comparisons cannot detect duplicate definitions or unresolved
    # references in the complete game. Keep independent evidence gates
    # running after a link failure so the checkpoint remains diagnosable.
    phase("ledger checkpointed")
    # Each entry names one failed gate; the summary at the end lists them all.
    failures = []
    if _link() != 0:
        failures.append("candidate link")
    phase("candidate linked")

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
            failures.append(f"{gate.__name__.rsplit('.', 1)[-1]} ({len(fatal)} finding(s))")
            for line in fatal:
                print(f"[build] {line}", file=sys.stderr)

    # Roll the cleanliness floors down only on an otherwise-green build:
    # a down-only bless recorded off a failing tree would bake in state
    # nobody reviewed. Check always, write only when clean.
    phase("evidence gates checked")
    from homm3.cleanliness import board
    if origins is None:
        # Missing evidence must not look like an empty, clean DC inventory.
        violations = ['cleanliness: cannot check without Dreamcast origins; floors unchanged']
    else:
        violations = board.check_and_roll(write=not failures, dc_origins=origins)
    if violations:
        ratchets = sum(not line.startswith(" ") for line in violations)
        failures.append(f"cleanliness board ({ratchets} violation(s))")
        for line in violations:
            print(f"[build] {line}", file=sys.stderr)
    phase("cleanliness board checked")

    # Scores describe the completed compile/delink, including when an
    # independent source gate fails. Keep README consistent with the ledger.
    # Complete byte accounting takes minutes; it is opt-in (`--data`, or
    # `homm3 verify data-coverage`). Without it README keeps its last line.
    data_accounting = None
    if data:
        try:
            from homm3.verify.byte_accounting import report as account_bytes
            data_accounting = account_bytes()
            totals = data_accounting['totals']['file']
            print(f"[data-coverage] {totals.get('missing', 0):,} unclaimed file bytes; "
                  f"{totals.get('overlap', 0):,} overlapping bytes; "
                  "worklist in build/gen/data_coverage.json")
        except (ValueError, OSError) as exc:
            print(f"[data-coverage] unavailable: {exc}")
            failures.append("byte accounting")
        phase("byte accounting refreshed")
    try:
        status.write_readme(data_accounting=data_accounting)
    except Exception as exc:  # the score block must never fail a build
        print(f"[build] README block skipped: {exc}")

    phase("finished")
    print("[build] Mac pairs are not part of `homm3 build`; run `homm3 mac build` "
          "for the Classic Mac preservation checkpoint")
    if not data:
        print("[build] byte accounting skipped; run `homm3 build --data` "
              "to refresh data coverage")
    if failures:
        print("[build] FAILED gates (details above): " + "; ".join(failures),
              file=sys.stderr)
        return 1
    print("[build] all gates passed")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
