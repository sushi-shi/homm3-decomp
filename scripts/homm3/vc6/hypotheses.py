"""Compile a Cartesian batch of reviewed source hypotheses with the TU's VC6 profile.

Manifest parsing/rendering is adapted from King's Field scripts/kf/hypotheses.py.
Scoring uses HoMM3's normal COFF normalization and objdiff report path. Neither
source nor the match ledger is rewritten; winners need a canonical build.
"""
from __future__ import annotations

import concurrent.futures
import hashlib
import itertools
import json
import shutil
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

from homm3.core import common
from homm3.match import status
from homm3.vc6._unit import compile_text, source_for_unit
from homm3.vc6 import tu_state_sweep as scoring

REPO = common.HOMM3_DIR

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
    unit = unit_name
    configured = source_for_unit(unit)
    if configured is None:
        raise ValueError(f"unknown unit {unit!r}")
    source = root / configured.relative_to(REPO)
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


@dataclass(frozen=True)
class ScoreContext:
    # The subset of the TU-state scorer's context used for a source batch.
    unit: str
    target_first: bytes
    scored: tuple[tuple[str, str], ...]


def run(args) -> int:
    if min(args.jobs, args.limit, args.keep_top) < 1:
        raise ValueError("--jobs, --limit, and --keep-top must be positive")
    payload, unit, selector, source, original, axes = parse_manifest(Path(args.manifest))
    count = 1
    for axis in axes:
        count *= len(axis.options)
    if count > args.limit:
        raise ValueError(f"manifest expands to {count} states, above --limit {args.limit}")
    # A newly admitted target can be in the focused report before its first
    # full checkpoint. Include those rows so the runner can finish admissions.
    report = json.loads((REPO / "build/objdiff/report.json").read_text())
    scored = tuple((unit, fn["name"]) for entry in report["units"]
                   if entry["name"] == unit for fn in entry.get("functions", []))
    symbols = [name for _, name in scored]
    matches = [name for name in symbols if selector == name]
    if not matches:
        matches = [name for name in symbols if selector in name]
    if len(matches) != 1:
        raise ValueError(f"{unit}:{selector}: expected one scored symbol, found {matches}")
    symbol = matches[0]
    target = REPO / "build/objdiff/target" / f"{unit}.c.obj"
    target_bytes = target.read_bytes()
    context = ScoreContext(unit, scoring._first_pass(unit, target_bytes), scored)
    fingerprint = scoring._shared_inputs_digest()
    ledger_bytes = status.BASELINE.read_bytes()
    candidates = variants(original, axes)
    output = Path(args.output) if args.output else Path(tempfile.mkdtemp(
        prefix=f"{unit}-", dir=_output_root()))
    if args.output:
        output.mkdir(parents=True, exist_ok=False)
    (output / "manifest.json").write_text(json.dumps(payload, indent=2) + "\n")
    print(f"[hypotheses] {unit}:{symbol}; {len(candidates)} unique/{count} states; "
          f"{args.jobs} compile jobs", flush=True)

    def trial(variant, scratch):
        directory = scratch / str(variant.index)
        started = time.monotonic()
        obj, error = compile_text(variant.source.decode(), unit, directory,
                                  source.stem, with_listing=False)
        scores = {}
        if obj is not None:
            try:
                base, reference = scoring._normalized_pair(context, obj.read_bytes())
                scores = scoring._report_scores(context, base, reference, directory)
            except Exception as exc:
                error = str(exc)
        return dict(index=variant.index, name=variant.name, labels=variant.labels,
                    score=scores.get(symbol) if not error else None,
                    scores=scores, error=error, seconds=time.monotonic() - started)

    with tempfile.TemporaryDirectory(prefix="compile-", dir=output) as temporary:
        scratch = Path(temporary)
        baseline = trial(Variant(-1, "canonical baseline", {}, original), scratch)
        if baseline["error"] or baseline["score"] is None:
            raise RuntimeError(f"baseline failed: {baseline}")
        print(f"[hypotheses] baseline {baseline['score']:.9f}%", flush=True)
        rows = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futures = [pool.submit(trial, candidate, scratch) for candidate in candidates]
            for future in concurrent.futures.as_completed(futures):
                row = future.result()
                rows.append(row)
                if len(rows) % 10 == 0 or len(rows) == len(candidates):
                    print(f"[hypotheses] {len(rows)}/{len(candidates)} compiled", flush=True)
        # Reject observations made across a moving compiler/header/target context.
        if (source.read_bytes() != original or target.read_bytes() != target_bytes
                or status.BASELINE.read_bytes() != ledger_bytes
                or scoring._shared_inputs_digest() != fingerprint):
            raise RuntimeError("source, ledger, headers, compiler, or scoring inputs changed during batch")
        ranked = sorted(rows, key=lambda row: (
            -(row["score"] if row["score"] is not None else -1), row["index"]))
        by_index = {candidate.index: candidate for candidate in candidates}
        for rank, row in enumerate(ranked[:args.keep_top], 1):
            if row["error"]:
                continue
            stem = f"rank-{rank:02d}-{row['index']:04d}"
            (output / f"{stem}.cpp").write_bytes(by_index[row["index"]].source)
            for name in (f"{source.stem}.obj", "candidate.c.obj", "target.c.obj"):
                shutil.copyfile(scratch / str(row["index"]) / name, output / f"{stem}-{name}")
            print(f"{row['score']:12.9f}% [{row['index']:04d}] {row['name']}", flush=True)
        result = dict(schema=1, unit=unit, function=symbol, states=count,
                      unique_states=len(candidates), context=fingerprint,
                      source_sha256=hashlib.sha256(original).hexdigest(),
                      target_sha256=hashlib.sha256(target_bytes).hexdigest(),
                      baseline=baseline, results=ranked)
        (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    exact = sum(row["score"] == 100.0 for row in rows)
    print(f"[hypotheses] {exact} exact; results: {output}", flush=True)
    return 0 if exact else 1


def _output_root():
    path = REPO / "build/hypotheses"
    path.mkdir(parents=True, exist_ok=True)
    return path
