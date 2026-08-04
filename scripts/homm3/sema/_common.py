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


def log_invocation(rc: int, cmd: str | None = None) -> None:
    """One line per `homm3 sema` invocation (ALL subcommands); must NEVER
    break the tool. Metadata first, the command after the `: `
    (shell-quoted) so it copies straight back into a shell:

        [2026-08-04][19:55:01][0]: homm3 sema xref 0x0005a340 --flat

    This is a usage-analysis feed: what actually gets run decides which
    tools grow and which retire (gruntz dropped two tools at 0 uses in
    9,771 logged calls). `cmd` overrides the reconstructed argv so a
    future batch mode can log one line per batched query.
    """
    try:
        import datetime
        import shlex
        now = datetime.datetime.now()
        if cmd is None:
            cmd = shlex.join(["homm3", "sema", *sys.argv[1:]])
        LOG.parent.mkdir(parents=True, exist_ok=True)
        with LOG.open("a") as fh:
            fh.write("[{}][{}][{}]: {}\n".format(
                now.date(), now.strftime("%H:%M:%S"), rc, cmd))
    except Exception:
        pass  # logging is best-effort by design
