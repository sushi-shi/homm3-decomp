"""Small project CLI for configuring and running the current build graph."""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(os.environ.get("HOMM3_DIR", Path(__file__).resolve().parents[2]))


def run(*command: str) -> int:
    return subprocess.run(command, cwd=ROOT).returncode


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    command = args.pop(0) if args else "help"

    if command == "configure":
        if args:
            print("usage: homm3 configure", file=sys.stderr)
            return 2
        return run("python3", "configure.py")

    if command == "build":
        if run("python3", "configure.py"):
            return 1
        return run("ninja", *(args or ["all"]))

    if command in ("help", "-h", "--help"):
        print("usage: homm3 {configure|build [ninja-target ...]}")
        return 0

    print("unknown command: %s" % command, file=sys.stderr)
    print("usage: homm3 {configure|build [ninja-target ...]}", file=sys.stderr)
    return 2
