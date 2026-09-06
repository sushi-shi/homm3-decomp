"""Best-effort command telemetry; preserve the copyable legacy log too."""
from __future__ import annotations

import contextlib
import contextvars
import datetime
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import threading
import time
import traceback
import uuid

from homm3.core import common

_ACTIVE_EVENT = contextvars.ContextVar("homm3_usage_event", default=None)


class _Output:
    def __init__(self, stream):
        self.stream = stream
        self.tail = ""

    def write(self, text):
        self.tail = (self.tail + text)[-16384:]
        return self.stream.write(text)

    def __getattr__(self, name):
        return getattr(self.stream, name)


def classify_error(text):
    """Classify diagnostic evidence, never infer compilation failure from rc alone."""
    lower = text.lower()
    # Environment failures can be wrapped in a Python traceback or Ninja error.
    if any(s in lower for s in ("operation not permitted", "permission denied",
                               "sandbox denied", "sandbox violation")):
        return "environment.permission"
    if any(s in lower for s in ("winepath", "wineserver:", "wine: could not",
                               "wine: failed", "wineboot")):
        return "environment.wine"
    if re.search(r"\b(?:fatal )?error C\d{4}\b", text):
        return "compile.cpp"
    if "no candidate tu" in lower:
        return "candidate.unavailable"
    if "stale normalized comparison object" in lower:
        return "candidate.stale"
    if "symbol " in lower and "not found in" in lower:
        return "candidate.symbol_missing"
    if "candidate object missing" in lower or "comparison object missing" in lower:
        return "candidate.object_missing"
    if "traceback (most recent call last)" in lower:
        return "tool.exception"
    if "unknown target" in lower:
        return "build.target"
    if any(s in lower for s in ("unrecognized arguments", "invalid choice",
                               "usage:", "not a homm3", "expected one argument")):
        return "cli.arguments"
    return "command.failed"


def run_logged(dispatch, argv, log, *, failure_rc=2, scope="module"):
    started = time.monotonic()
    started_at = datetime.datetime.now(datetime.timezone.utc).isoformat()
    event_id = str(uuid.uuid4())
    parent = _ACTIVE_EVENT.get() or os.environ.get("HOMM3_USAGE_PARENT_EVENT_ID")
    token = _ACTIVE_EVENT.set(event_id)
    stderr, stdout = _Output(sys.stderr), _Output(sys.stdout)
    rc, error = 0, None
    try:
        with contextlib.redirect_stderr(stderr), contextlib.redirect_stdout(stdout):
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
        _ACTIVE_EVENT.reset(token)
        failed = rc < 0 or rc >= failure_rc or error is not None
        # VC6 writes errors to stdout. Preserve both bounded tails, without
        # storing routine successful command output (which can be enormous).
        diagnostic = "\n".join(t for t in (stderr.tail.strip(), stdout.tail.strip()) if t)
        error = error or (diagnostic if failed else None)
        category = ("interrupted" if rc == 130 else "process.signal" if rc < 0
                    else classify_error(error or "")) if failed else None
        try:
            log(rc, event_id=event_id, parent_event_id=parent, scope=scope,
                started_at=started_at,
                duration_seconds=round(time.monotonic() - started, 6),
                error=error, error_category=category,
                outcome="error" if failed else "difference" if rc == 1 else "success")
        except Exception:
            pass


def run_process(command, *, cwd):
    """Stream both child pipes unchanged while the outer logger observes errors.

    Nested module logs have their own event IDs and reference the outer CLI
    event, so an audit can use CLI roots without counting children twice.
    """
    env = os.environ.copy()
    if _ACTIVE_EVENT.get():
        env["HOMM3_USAGE_PARENT_EVENT_ID"] = _ACTIVE_EVENT.get()
    with subprocess.Popen(command, cwd=cwd, env=env, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True, errors="replace") as process:
        def forward(pipe, stream):
            try:
                for line in pipe:
                    stream.write(line)
                    stream.flush()
            finally:
                pipe.close()
        threads = [threading.Thread(target=forward, args=(pipe, stream), daemon=True)
                   for pipe, stream in ((process.stdout, sys.stdout),
                                        (process.stderr, sys.stderr))]
        for thread in threads:
            thread.start()
        try:
            rc = process.wait()
        finally:
            # Ctrl-C normally reaches the foreground child too. Do not leave a
            # child running if a Python-only interrupt reaches this process.
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            for thread in threads:
                thread.join()
        return rc


def append(path, cmd, rc, **metadata):
    try:
        now = datetime.datetime.now(datetime.timezone.utc)
        root = common.HOMM3_DIR.resolve()
        git = subprocess.run(
            ["git", "rev-parse", "HEAD", "--absolute-git-dir"], cwd=root,
            capture_output=True, text=True, timeout=2)
        fields = git.stdout.strip().splitlines() if git.returncode == 0 else []
        test_marker = os.environ.get("HOMM3_USAGE_TEST") or None
        record = {"schema": "homm3.usage.v2", "time": now.isoformat(),
                  "event_id": str(uuid.uuid4()), "command": cmd, "rc": rc,
                  "revision": fields[0] if fields else None,
                  "worktree": str(root), "git_dir": fields[1] if len(fields) > 1 else None,
                  "cwd": str(Path.cwd()), "pid": os.getpid(),
                  "is_test": test_marker is not None, "test_marker": test_marker,
                  **metadata}
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("a") as stream:
            stream.write(f"[{now.date()}][{now:%H:%M:%S}][{rc}]: {cmd}\n")
        with path.with_suffix(".jsonl").open("a") as stream:
            stream.write(json.dumps(record, ensure_ascii=True) + "\n")
    except Exception:
        pass
