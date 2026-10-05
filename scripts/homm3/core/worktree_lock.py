"""One writer at a time per worktree for the commands that mutate build/.

`homm3 build` (full and --fast), `homm3 delink`, `homm3 status update`
(and `--write-readme`), `homm3 mac build` and the interactive candidate
refresh all rewrite shared outputs: objects under build/objdiff, the
normalized copies, build/objdiff/report.json and the ledger. Two of them
running in one worktree slow each other down and can interleave their
writes. The second one now waits, saying which command holds the lock:

    [homm3] another `homm3 build --fast cursor` (pid 1234, started 12:01:05)
    is running in this worktree; waiting...

The lock is an flock(2) on build/.homm3-worktree.lock, so it is released
by the kernel when its holder exits, however it exits. The holder's pid,
command and start time are recorded beside it (build/.homm3-worktree.lock.json)
for that message; a record naming a dead pid is reported as stale and
ignored. Child processes of the holder inherit HOMM3_WORKTREE_LOCK_PID and
re-enter without waiting (a full build's in-process delink, `homm3 build`
spawning helper modules), so the lock never deadlocks against its owner.
"""
from __future__ import annotations

import contextlib
import datetime
import errno
import fcntl
import json
import os
import sys
import time
from pathlib import Path

ENV = "HOMM3_WORKTREE_LOCK_PID"
POLL_SECONDS = 0.5

_depth = 0


def _root() -> Path:
    from homm3.core import common
    return common.HOMM3_DIR


LOCK_NAME = ".homm3-worktree.lock"


def lock_path(root: Path | None = None) -> Path:
    return (root or _root()) / "build" / LOCK_NAME


def _record_path(lock: Path) -> Path:
    return lock.with_name(lock.name + ".json")


def pid_alive(pid: int) -> bool:
    if pid <= 0:
        return False
    try:
        os.kill(pid, 0)
    except OSError as exc:
        return exc.errno == errno.EPERM
    return True


def holder(lock: Path | None = None) -> dict | None:
    """The recorded holder, or None when no live process is recorded."""
    try:
        record = json.loads(_record_path(lock or lock_path()).read_text())
        pid = int(record["pid"])
    except (OSError, ValueError, KeyError, TypeError):
        return None
    return record if pid_alive(pid) else None


def _inherited(lock: Path) -> bool:
    """True when an ancestor process holds this worktree's lock."""
    value = os.environ.get(ENV)
    if not value:
        return False
    try:
        pid = int(value)
        record = json.loads(_record_path(lock).read_text())
    except (OSError, ValueError, TypeError):
        return False
    return record.get("pid") == pid and pid_alive(pid)


def _describe(record: dict | None) -> str:
    if not record:
        return "another homm3 command"
    started = record.get("started", "")
    try:
        started = datetime.datetime.fromisoformat(started).astimezone().strftime("%H:%M:%S")
    except (TypeError, ValueError):
        pass
    return f"another `{record.get('command', 'homm3')}` (pid {record.get('pid')}, started {started})"


@contextlib.contextmanager
def hold(command: str, *, lock: Path | None = None, stream=None, poll: float = POLL_SECONDS):
    """Hold the worktree lock for `command`, waiting for any other holder.

    Re-entrant in-process and for children of the holder (see module doc).
    `lock` overrides build/.homm3-worktree.lock (tests, relocated build dirs).
    """
    global _depth
    lock = lock or lock_path()
    if _depth or _inherited(lock):
        _depth += 1
        try:
            yield
        finally:
            _depth -= 1
        return
    stream = stream or sys.stderr
    lock.parent.mkdir(parents=True, exist_ok=True)
    with open(lock, "a+") as handle:
        waited_since = None
        announced = None
        while True:
            try:
                fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except BlockingIOError:
                pass
            record = holder(lock)
            key = (record or {}).get("pid")
            if waited_since is None or key != announced:
                if waited_since is None:
                    waited_since = time.monotonic()
                announced = key
                if record is None:
                    message = ("another homm3 command holds this worktree's lock "
                               "(its holder record is missing or names a process that "
                               "has exited); waiting for it to be released...")
                else:
                    message = f"{_describe(record)} is running in this worktree; waiting..."
                print(f"[homm3] {message}", file=stream, flush=True)
            time.sleep(poll)
        if waited_since is not None:
            print(f"[homm3] worktree lock acquired after "
                  f"{time.monotonic() - waited_since:.0f}s", file=stream, flush=True)
        record = {"pid": os.getpid(), "command": command,
                  "started": datetime.datetime.now(datetime.timezone.utc).isoformat()}
        record_path = _record_path(lock)
        temporary = record_path.with_name(f".{record_path.name}.{os.getpid()}.tmp")
        temporary.write_text(json.dumps(record))
        os.replace(temporary, record_path)
        previous = os.environ.get(ENV)
        os.environ[ENV] = str(os.getpid())
        _depth += 1
        try:
            yield
        finally:
            _depth -= 1
            if previous is None:
                os.environ.pop(ENV, None)
            else:
                os.environ[ENV] = previous
            with contextlib.suppress(OSError):
                current = json.loads(record_path.read_text())
                if current.get("pid") == os.getpid():
                    record_path.unlink()
            fcntl.flock(handle, fcntl.LOCK_UN)
