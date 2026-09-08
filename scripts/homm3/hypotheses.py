"""Compatibility entry point for the shared VC6 source-hypothesis runner."""
from pathlib import Path
from types import SimpleNamespace

# Preserve the original manifest helpers and run(path, ...) API for callers.
from homm3 import manifest
from homm3.vc6.hypotheses import (
    Axis, Edit, Option, Variant, parse_manifest, render, variants,
    run as run_batch,
)


def run(path: Path, jobs: int, limit: int, keep_top: int, output: Path | None) -> int:
    return run_batch(SimpleNamespace(
        manifest=path, jobs=jobs, limit=limit, keep_top=keep_top, output=output))


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("-j", "--jobs", type=int, default=8)
    parser.add_argument("--limit", type=int, default=256)
    parser.add_argument("--keep-top", type=int, default=8)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if min(args.jobs, args.limit, args.keep_top) < 1:
        parser.error("jobs, limit, and keep-top must be positive")
    try:
        return run(args.manifest, args.jobs, args.limit, args.keep_top, args.output)
    except (ValueError, OSError, RuntimeError) as exc:
        parser.exit(1, f"hypotheses: {exc}\n")


if __name__ == "__main__":
    raise SystemExit(main())
