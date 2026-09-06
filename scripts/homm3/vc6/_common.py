"""homm3.vc6._common - shared plumbing for the vc6 modelling tools.

Dependency direction: cli -> vc6 -> {core, sema}; nothing here imports a
vc6 tool. Paths come from homm3.core.common.

rc convention (from gruntz, shared with sema): 0 = answered / agrees,
1 = answered-NO (a prediction that disagrees, a census function below its
model), 2 = error. `die()` is the rc-2 exit so "no" stays distinguishable
from "broken" in scripts and in the log.
"""
from __future__ import annotations

import datetime
import sys

from homm3.core import common

REPO = common.HOMM3_DIR
LOG = REPO / "build/homm3_vc6.log"
EVIDENCE = REPO / "evidence/vc6"
DOCS = REPO / "docs/vc6"


def die(msg: str):
    print(f"[homm3 vc6] ERROR: {msg}", file=sys.stderr)
    sys.exit(2)


def provenance(generator: str, extra: list[str] | None = None) -> list[str]:
    """Header for a generated evidence/vc6 TSV.

    The subject here is the TOOLCHAIN, not the game image, so the pinned
    identity we cite is the compiler binary's - see _toolchain.PINNED.
    """
    lines = [
        f"# generator: {generator}",
        f"# date: {datetime.date.today().isoformat()}",
        "# ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit",
    ]
    return lines + list(extra or [])


def log_invocation(rc: int, cmd: str | None = None, **metadata) -> None:
    from homm3.core import usage
    import shlex
    usage.append(LOG, cmd or shlex.join(["homm3", "vc6", *sys.argv[1:]]),
                 rc, **metadata)
