"""homm3.sema._common - shared plumbing for the sema tools.

Dependency direction: cli -> sema -> {core, match, build}; nothing in
here imports a sema tool. Paths come from homm3.core.common - no
per-module repo-walk duplicates (the regression homm2's flatten grew).

rc convention (from gruntz): 0 = answered, 1 = answered-NO (a diff that
differs, a function below 100), 2 = error. `die()` is the rc-2 exit so
"no" stays distinguishable from "broken" in scripts and in the log.
"""
from __future__ import annotations

import sys

from homm3.core import common

REPO = common.HOMM3_DIR
LOG = REPO / "build/homm3_sema.log"


def die(msg: str):
    print(f"[homm3 sema] ERROR: {msg}", file=sys.stderr)
    sys.exit(2)


def log_invocation(rc: int, cmd: str | None = None, **metadata) -> None:
    from homm3.core import usage
    import shlex
    usage.append(LOG, cmd or shlex.join(["homm3", "sema", *sys.argv[1:]]),
                 rc, **metadata)
