#!/usr/bin/env python3
"""Negative controls for the normalized-freshness gate.

CLAUDE.md contract: every fatal gate ships with a control proving it
still detects its defect. The gate here is the stale-comparison-object
refusal (`homm3 sema diff` dies rc=2 through
homm3.build.normalized_freshness.freshness_problems); these controls
prove each refusal path fires. Runnable standalone
(`python3 -m homm3.build.test_normalized_freshness`, rc!=0 on any
failure) and as plain pytest test functions.
"""
from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

from homm3.build.normalized_freshness import (
    freshness_problems, stamp_path, write_stamp,
)


def _tree(base: Path):
    """A minimal raw->normalized pair with a current stamp."""
    raw = base / "base" / "unit.obj"
    raw.parent.mkdir(parents=True)
    raw.write_bytes(b"raw object bytes")
    normalized = base / "normalized" / "base" / "unit.obj"
    normalized.parent.mkdir(parents=True)
    normalized.write_bytes(b"normalized bytes")
    write_stamp(normalized, {"raw": raw})
    return raw, normalized


def test_fresh_pair_passes():
    with tempfile.TemporaryDirectory() as tmp:
        _raw, normalized = _tree(Path(tmp))
        assert freshness_problems(normalized) == []


def test_missing_stamp_is_refused():
    with tempfile.TemporaryDirectory() as tmp:
        _raw, normalized = _tree(Path(tmp))
        stamp_path(normalized).unlink()
        problems = freshness_problems(normalized)
        assert problems and "no provenance stamp" in problems[0]


def test_changed_raw_input_is_refused():
    with tempfile.TemporaryDirectory() as tmp:
        raw, normalized = _tree(Path(tmp))
        raw.write_bytes(b"REBUILT raw object bytes")
        problems = freshness_problems(normalized)
        assert problems and "is stale" in problems[0]


def test_missing_raw_input_is_refused():
    with tempfile.TemporaryDirectory() as tmp:
        raw, normalized = _tree(Path(tmp))
        raw.unlink()
        problems = freshness_problems(normalized)
        assert problems and "is missing" in problems[0]


def test_unknown_schema_is_refused():
    with tempfile.TemporaryDirectory() as tmp:
        _raw, normalized = _tree(Path(tmp))
        stamp = stamp_path(normalized)
        payload = json.loads(stamp.read_text())
        payload["schema"] = 9999
        stamp.write_text(json.dumps(payload))
        problems = freshness_problems(normalized)
        assert problems and "unknown schema" in problems[0]


_CONTROLS = (
    test_fresh_pair_passes,
    test_missing_stamp_is_refused,
    test_changed_raw_input_is_refused,
    test_missing_raw_input_is_refused,
    test_unknown_schema_is_refused,
)


def main() -> int:
    failed = 0
    for control in _CONTROLS:
        try:
            control()
            print(f"  ok    {control.__name__}")
        except AssertionError as exc:
            failed += 1
            print(f"  FAIL  {control.__name__}: {exc}")
    print(f"[test_normalized_freshness] {len(_CONTROLS) - failed}/"
          f"{len(_CONTROLS)} controls passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
