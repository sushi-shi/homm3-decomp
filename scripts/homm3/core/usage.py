"""Best-effort command telemetry; preserve the copyable legacy log too."""
from __future__ import annotations

import contextlib
import datetime
import json
import subprocess
import sys
import time
import traceback

from homm3.core import common


class _Stderr:
    def __init__(self, stream):
        self.stream = stream
        self.tail = ""

    def write(self, text):
        self.tail = (self.tail + text)[-16384:]
        return self.stream.write(text)

    def __getattr__(self, name):
        return getattr(self.stream, name)


def run_logged(dispatch, argv, log):
    started = time.monotonic()
    stderr = _Stderr(sys.stderr)
    rc, error = 0, None
    try:
        with contextlib.redirect_stderr(stderr):
            rc = dispatch(argv) or 0
        return rc
    except SystemExit as exc:
        rc = exc.code if isinstance(exc.code, int) else (0 if exc.code is None else 1)
        error = str(exc.code) if isinstance(exc.code, str) else None
        raise
    except BaseException as exc:
        rc = 130 if isinstance(exc, KeyboardInterrupt) else 2
        error = "".join(traceback.format_exception(type(exc), exc, exc.__traceback__))
        print(error, end="", file=sys.stderr)
        raise SystemExit(rc) from None
    finally:
        try:
            log(rc, duration_seconds=round(time.monotonic() - started, 6),
                error=error or (stderr.tail.strip() if rc >= 2 else None))
        except Exception:
            pass


def append(path, cmd, rc, **metadata):
    try:
        now = datetime.datetime.now().astimezone()
        revision = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=common.HOMM3_DIR,
            capture_output=True, text=True, timeout=2).stdout.strip() or None
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("a") as stream:
            stream.write(f"[{now.date()}][{now:%H:%M:%S}][{rc}]: {cmd}\n")
        record = {"schema": "homm3.usage.v1", "time": now.isoformat(),
                  "command": cmd, "rc": rc, "revision": revision, **metadata}
        with path.with_suffix(".jsonl").open("a") as stream:
            stream.write(json.dumps(record, ensure_ascii=True) + "\n")
    except Exception:
        pass
