#!/usr/bin/env python3
"""homm3.build.build - the `homm3 build` command.

    configure -> ninja (base objs) -> normalize comparison copies ->
    configure (objdiff.json sees new normalized paths) -> objdiff report
    -> overall line -> [normal tier] baseline raise + ratchet check
    (FATAL on regression) + README score block + stale-delink probe.

`--fast` stops after the overall line and says what it skipped (the gruntz
inner-loop tier). The build NEVER delinks (homm2 rule): the probe only
warns when the synth-PDB inputs are newer than the PDB - run
`homm3 delink` explicitly.
"""
from __future__ import annotations

import subprocess
import sys

from homm3.carve import common

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

    report = status.load_report()
    print(f"[build] {status.overall_line(report)}")
    print(f"[build] report: {status.REPORT.relative_to(ROOT)}")

    if fast:
        print("[build] fast: ratchet + README + delink probe skipped - "
              "run `homm3 build` before committing")
        return 0

    status.cmd_update(report)
    if status.cmd_check(report, gate=True):
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
