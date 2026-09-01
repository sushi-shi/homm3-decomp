#!/usr/bin/env python3
"""homm3.build.build - the `homm3 build` command.

    configure -> ninja (base objs) -> normalize comparison copies ->
    configure (objdiff.json sees new normalized paths) -> objdiff report
    -> overall line -> [normal tier] checkpoint-ledger refresh + dip report
    (OBSERVATIONAL) + banked-rows check (FATAL when a previously
    banked RVA left the baseline entirely) + cleanliness board (FATAL
    when a ratcheted source metric rises above its committed floor -
    C-style casts are banned at 0) + README score block + stale-delink
    probe.

`--fast` stops after the overall line and says what it skipped (the gruntz
inner-loop tier). The build never RE-delinks (homm2 rule - an existing
comparison target must not move silently under a matcher); a fresh tree
with no targets at all bootstraps the first delink, and afterwards the
probe only warns when the synth-PDB inputs are newer than the PDB - run
`homm3 delink` explicitly.
"""
from __future__ import annotations

import subprocess
import sys

from homm3.core import common

ROOT = common.HOMM3_DIR
PDB = ROOT / "build/pdb/HEROES3.pdb"


def _run(*command: str) -> int:
    return subprocess.run(list(command), cwd=ROOT).returncode


def delink_inputs_stale() -> list[str]:
    if not PDB.is_file():
        return ["build/pdb/HEROES3.pdb missing"]
    stamp = PDB.stat().st_mtime
    stale = []
    probes = (list((ROOT / "src").glob("*.c*"))
              + [ROOT / "config/retail-zlib-map.tsv",
                 ROOT / "config/retail-runtime-map.tsv",
                 ROOT / "config/retail-functions.tsv",
                 ROOT / "config/retail-relocs.tsv",
                 ROOT / "config/retail-vtables.tsv"])
    for path in probes:
        if path.is_file() and path.stat().st_mtime > stamp:
            stale.append(str(path.relative_to(ROOT)))
    return stale


def main(argv=None) -> int:
    argv = list(argv or [])
    fast = "--fast" in argv
    ninja_args = [a for a in argv if a != "--fast"]

    from homm3.build import configure, normalize_objs
    from homm3.match import status

    configure.main()
    if _run("ninja", *ninja_args):
        return 1
    normalize_objs.main([])
    configure.main()

    # a fresh tree has no delinked targets: every unit would pair against
    # dummy.obj and the ratchet would report 68 bogus MISSING regressions.
    # Bootstrap the FIRST delink instead - the never-delink rule protects an
    # EXISTING comparison target from moving silently; from nothing there is
    # nothing to protect. Later builds only warn (the probe below).
    if not any((ROOT / "build/objdiff/target").glob("*.c.obj")):
        print("[build] no delinked targets yet - bootstrapping the first "
              "delink")
        from homm3.build import delink
        if delink.main([]):
            return 1

    report = status.load_report()
    print(f"[build] {status.overall_line(report)}")
    print(f"[build] report: {status.REPORT.relative_to(ROOT)}")

    if fast:
        print("[build] fast: checkpoint ledger + gates + README + delink "
              "probe skipped - "
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

    stale = delink_inputs_stale()
    if stale:
        print(f"[build] delink inputs changed since the last synth PDB "
              f"({', '.join(stale[:4])}"
              + (f", +{len(stale) - 4} more" if len(stale) > 4 else "")
              + ") - run `homm3 delink`")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
