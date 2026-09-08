"""Compile and score a bounded matrix of source hypotheses for one function.

The JSON manifest describes exact, reviewable source substitutions.  Axes form a
Cartesian product, so five binary axes produce 32 independently compiled states.
Each state uses a disposable source copy and an isolated object directory;
the configured source is never rewritten.
"""

from __future__ import annotations

import concurrent.futures
import hashlib
import itertools
import json
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

from homm3 import manifest
from homm3.core.common import HOMM3_DIR as REPO
from homm3.vc6._unit import compile_text
from homm3.vc6 import tu_state_sweep as scoring
from types import SimpleNamespace
BUILD = REPO / "build"


@dataclass(frozen=True)
class Edit:
    start: int
    end: int
    replacement: bytes


@dataclass(frozen=True)
class Option:
    name: str
    edits: tuple[Edit, ...]


@dataclass(frozen=True)
class Axis:
    name: str
    options: tuple[Option, ...]


@dataclass(frozen=True)
class Variant:
    index: int
    name: str
    labels: dict[str, str]
    source: bytes


def _unique_span(original: bytes, value: object, context: str) -> tuple[int, int]:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{context}: find must be a non-empty string")
    needle = value.encode()
    count = original.count(needle)
    if count != 1:
        raise ValueError(f"{context}: exact find span occurs {count} times, expected 1")
    start = original.index(needle)
    return start, start + len(needle)


def _parse_edits(raw_edits: object, original: bytes, context: str) -> tuple[Edit, ...]:
    if not isinstance(raw_edits, list):
        raise ValueError(f"{context}: edits must be a list")
    edits = []
    for raw in raw_edits:
        if not isinstance(raw, dict):
            raise ValueError(f"{context}: every edit must be an object")
        start, end = _unique_span(original, raw.get("find"), context)
        replacement = raw.get("replace")
        if not isinstance(replacement, str):
            raise ValueError(f"{context}: replace must be a string")
        edits.append(Edit(start, end, replacement.encode()))
    edits.sort(key=lambda edit: (edit.start, edit.end))
    for left, right in zip(edits, edits[1:]):
        if left.end > right.start:
            raise ValueError(f"{context}: edits overlap")
    return tuple(edits)


def parse_manifest(path: Path, *, root: Path = REPO):
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("schema") != 1:
        raise ValueError("manifest schema must be 1")
    unit_name = payload.get("unit")
    function_name = payload.get("function")
    if not isinstance(unit_name, str) or not unit_name:
        raise ValueError("manifest unit must be a non-empty string")
    if not isinstance(function_name, str) or not function_name:
        raise ValueError("manifest function must be a non-empty string")
    unit = manifest.by_unit().get(unit_name)
    if unit is None:
        raise ValueError(f"unknown unit {unit_name!r}")
    source = root / unit["source"]
    original = source.read_bytes()
    raw_axes = payload.get("axes")
    if not isinstance(raw_axes, list) or not raw_axes:
        raise ValueError("manifest axes must be a non-empty list")
    axes = []
    axis_names = set()
    all_axis_edits: list[tuple[str, Edit]] = []
    for raw_axis in raw_axes:
        if not isinstance(raw_axis, dict):
            raise ValueError("every axis must be an object")
        name = raw_axis.get("name")
        if not isinstance(name, str) or not name or name in axis_names:
            raise ValueError("axis names must be unique non-empty strings")
        axis_names.add(name)
        start, end = _unique_span(original, raw_axis.get("find"), f"axis {name}")
        find = original[start:end]
        raw_options = raw_axis.get("options")
        if not isinstance(raw_options, list) or not raw_options:
            raise ValueError(f"axis {name}: options must be a non-empty list")
        options = []
        option_names = set()
        for raw_option in raw_options:
            if not isinstance(raw_option, dict):
                raise ValueError(f"axis {name}: every option must be an object")
            option_name = raw_option.get("name")
            if (not isinstance(option_name, str) or not option_name
                    or option_name in option_names):
                raise ValueError(f"axis {name}: option names must be unique")
            option_names.add(option_name)
            replacement = raw_option.get("replace")
            if replacement is None:
                replacement_bytes = find
            elif isinstance(replacement, str):
                replacement_bytes = replacement.encode()
            else:
                raise ValueError(f"axis {name}/{option_name}: replace must be a string")
            edits = (Edit(start, end, replacement_bytes), *_parse_edits(
                raw_option.get("extra_edits", []), original,
                f"axis {name}/{option_name}",
            ))
            ordered = sorted(edits, key=lambda edit: (edit.start, edit.end))
            for left, right in zip(ordered, ordered[1:]):
                if left.end > right.start:
                    raise ValueError(f"axis {name}/{option_name}: edits overlap")
            options.append(Option(option_name, tuple(edits)))
            all_axis_edits.extend((name, edit) for edit in edits)
        axes.append(Axis(name, tuple(options)))
    for index, (left_name, left) in enumerate(all_axis_edits):
        for right_name, right in all_axis_edits[index + 1:]:
            if left_name != right_name and left.start < right.end and right.start < left.end:
                raise ValueError(f"axes {left_name} and {right_name} overlap")
    return payload, unit, function_name, source, original, tuple(axes)


def render(original: bytes, edits: tuple[Edit, ...]) -> bytes:
    result = original
    for edit in sorted(edits, key=lambda item: item.start, reverse=True):
        result = result[:edit.start] + edit.replacement + result[edit.end:]
    return result


def variants(original: bytes, axes: tuple[Axis, ...]) -> list[Variant]:
    result = []
    seen = {}
    for choices in itertools.product(*(axis.options for axis in axes)):
        labels = {axis.name: option.name for axis, option in zip(axes, choices)}
        candidate = render(original, tuple(edit for option in choices for edit in option.edits))
        digest = hashlib.sha256(candidate).hexdigest()
        if digest in seen:
            continue
        seen[digest] = labels
        name = ",".join(f"{key}={value}" for key, value in labels.items())
        result.append(Variant(len(result), name, labels, candidate))
    return result


def run(path: Path, jobs: int, limit: int, keep_top: int, output: Path | None) -> int:
    payload, unit, function, source, original, axes = parse_manifest(path)
    import math
    count = math.prod(len(axis.options) for axis in axes)
    if count > limit:
        raise ValueError(f"{count} combinations exceed --limit {limit}")
    unit_name = unit["unit"]
    baseline = scoring.status.load_baseline()
    scored = tuple(key for key, row in baseline.items()
                   if key[0] == unit_name and row.cur is not None)
    if (unit_name, function) not in scored:
        raise ValueError("function must be an exact scored symbol from match_baseline.tsv")
    target_path = BUILD / "objdiff/target" / f"{unit_name}.c.obj"
    target = target_path.read_bytes()
    inputs_digest = scoring._shared_inputs_digest()
    ledger_path = REPO / "config/match_baseline.tsv"
    ledger = ledger_path.read_bytes()
    plan = SimpleNamespace(unit=unit_name, scored=scored,
                           target_first=scoring._first_pass(unit_name, target))
    states = variants(original, axes)
    parent = BUILD / "hypotheses"
    parent.mkdir(parents=True, exist_ok=True)
    output = output or Path(tempfile.mkdtemp(prefix=path.stem + "-", dir=parent))
    output.mkdir(parents=True, exist_ok=True)
    (output / "manifest.json").write_text(json.dumps(payload, indent=2) + "\n")
    print(f"{len(states)} unique candidates; {jobs} VC6 jobs; {output}", flush=True)

    def evaluate(state):
        directory = output / f"state-{state.index:04d}"
        directory.mkdir()
        row = {"index": state.index, "labels": state.labels,
               "source_sha256": hashlib.sha256(state.source).hexdigest()}
        start = time.monotonic()
        obj, error = compile_text(state.source.decode(), unit_name, directory,
                                  "candidate", with_listing=False)
        if obj is None:
            row.update(error=error, score=None)
        else:
            base, target_copy = scoring._normalized_pair(plan, obj.read_bytes())
            scores = scoring._report_scores(plan, base, target_copy, directory)
            row.update(score=scores[function], scores=scores)
        row["seconds"] = time.monotonic() - start
        (directory / "result.json").write_text(json.dumps(row, indent=2) + "\n")
        return row

    rows = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(evaluate, state): state for state in states}
        for future in concurrent.futures.as_completed(futures):
            row = future.result()
            rows.append(row)
            print(f"[{len(rows)}/{len(states)}] {row['index']}: {row['score']}", flush=True)
    if (source.read_bytes() != original or target_path.read_bytes() != target
            or ledger_path.read_bytes() != ledger
            or scoring._shared_inputs_digest() != inputs_digest):
        raise RuntimeError("source, headers, compiler, ledger or scoring inputs changed during batch; results invalid")
    rows.sort(key=lambda row: (-(row['score'] if row['score'] is not None else -1), row['index']))
    (output / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
    for rank, row in enumerate(rows[:keep_top]):
        (output / f"top-{rank:02d}-state-{row['index']:04d}.cpp").write_bytes(states[row['index']].source)
    print(f"Best: {rows[0]['score']}; results: {output / 'results.json'}")
    return 0 if any(row['score'] is not None for row in rows) else 1


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
