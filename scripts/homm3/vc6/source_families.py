"""Generate and score reviewable C++ source families, inspired by Gruntz.

Exact, authored JSON edit axes form a Cartesian product. Each generation
samples that product, then breeds from top distinct aggregate/specialist
candidates. This is source reconstruction, NOT random include/parser noise.

Every candidate gets an isolated header/source tree and the manifest's exact
VC6 profiles. All configured functions in every requested TU are scored.
Nothing writes authored source or MAX: different implementations must not be
banked as observations of the old source hash. Adopt a reviewed candidate
with a normal build. Input manifests, source snapshots, objects and results
are retained under build/source-families for inspection and reproduction.

Run: python -m homm3.vc6.source_families FAMILY.json --width 60 --keep 8
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import random
import re
import shutil
import subprocess
import sys
import time

from homm3.core import common
from homm3.match import status
from homm3.vc6 import tu_state_sweep as scoring
from homm3.vc6._unit import flags_for_unit, source_for_unit

VERSION = 4


@dataclass(frozen=True)
class Edit:
    source: str
    start: int
    end: int
    replacement: str


@dataclass(frozen=True)
class Option:
    name: str
    edits: tuple[Edit, ...]


@dataclass(frozen=True)
class Axis:
    name: str
    options: tuple[Option, ...]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def identity_symbol(name):
    # VC6 anonymous-namespace scope contains the absolute input path AND a
    # fresh numeric nonce on each invocation. Preserve the TU basename,
    # semantic name and full suffix; omit only that non-code identity salt.
    # This is a diversity/reproduction metric, never a scoring normalization.
    return re.sub(r'@\?%[^@]*[\\/]([^\\/@]+\.cpp)\d+@', r'@?%\1@', name)


def code_identity(payload):
    """Ignore paths, timestamps and COFF bookkeeping, not bytes/relocations."""
    from homm3.build.canonicalize_data_symbols import CoffObject, MEM_EXECUTE, FUNCTION_TYPE
    coff = CoffObject(payload)
    sections = []
    for section in coff.sections:
        if not section.characteristics & MEM_EXECUTE:
            continue
        names = sorted((identity_symbol(sym.name), sym.value) for sym in coff.symbols.values()
                       if sym.section == section.index and sym.typ == FUNCTION_TYPE)
        relocs = [(rel.site, rel.typ, identity_symbol(coff.symbols[rel.symbol_index].name),
                   coff.symbols[rel.symbol_index].value)
                  for rel in coff.relocations if rel.section == section.index]
        sections.append((names, coff.section_bytes(section).hex(), relocs))
    return digest(json.dumps(sorted(sections)).encode())


def overlap(left, right):
    if left.source != right.source:
        return False
    if left.start == left.end:
        return right.start < left.start < right.end or left.start == right.start == right.end
    if right.start == right.end:
        return left.start < right.start < left.end
    return left.start < right.end and right.start < left.end


def load_manifest(path, root):
    payload = json.loads(path.read_text())
    if payload.get("schema") != 1 or not payload.get("axes"):
        raise ValueError("expected schema 1 and nonempty axes")
    originals = {}

    def parse_edit(raw, default_source):
        relative = raw.get("source", default_source)
        if not isinstance(relative, str):
            raise ValueError("every edit needs a source")
        resolved = (root / relative).resolve()
        if not resolved.is_relative_to(root.resolve()):
            raise ValueError(f"source escapes worktree: {relative}")
        relative = str(resolved.relative_to(root.resolve()))
        if Path(relative).parts[0] not in ("src", "include"):
            raise ValueError("source edits must be under src/ or include/; vendor is immutable")
        if relative not in originals:
            originals[relative] = resolved.read_text()
        original = originals[relative]
        anchors = [key for key in ("find", "insert_before", "insert_after") if key in raw]
        if len(anchors) != 1:
            raise ValueError("edit needs exactly one find/insert_before/insert_after anchor")
        kind = anchors[0]
        needle = raw[kind]
        if not isinstance(needle, str) or not needle or original.count(needle) != 1:
            raise ValueError(f"{relative}: anchor must occur exactly once: {needle!r}")
        start = original.index(needle)
        end = start + len(needle)
        replacement = raw.get("replace", needle) if kind == "find" else raw.get("text")
        if not isinstance(replacement, str):
            raise ValueError("replacement/text must be a string")
        if kind == "insert_before":
            end = start
        elif kind == "insert_after":
            start = end
        return Edit(relative, start, end, replacement)

    axes = []
    names = set()
    for raw_axis in payload["axes"]:
        name = raw_axis.get("name")
        if not isinstance(name, str) or not name or name in names:
            raise ValueError("axis names must be unique nonempty strings")
        names.add(name)
        options = []
        option_names = set()
        for raw_option in raw_axis.get("options", []):
            option_name = raw_option.get("name")
            if not isinstance(option_name, str) or not option_name or option_name in option_names:
                raise ValueError(f"{name}: option names must be unique nonempty strings")
            option_names.add(option_name)
            edit = {key: value for key, value in raw_axis.items() if key not in ("name", "options")}
            edit.update({key: value for key, value in raw_option.items() if key not in ("name", "extra_edits")})
            default = raw_axis.get("source", payload.get("source"))
            edits = [parse_edit(edit, default)]
            edits.extend(parse_edit(extra, default) for extra in raw_option.get("extra_edits", []))
            if any(overlap(a, b) for a, b in itertools.combinations(edits, 2)):
                raise ValueError(f"{name}/{option_name}: overlapping edits")
            options.append(Option(option_name, tuple(edits)))
        if not options:
            raise ValueError(f"{name}: no options")
        axes.append(Axis(name, tuple(options)))
    for a, b in itertools.combinations(axes, 2):
        if any(overlap(x, y) for p in a.options for q in b.options for x in p.edits for y in q.edits):
            raise ValueError(f"overlapping axes: {a.name}, {b.name}")
    return payload, originals, tuple(axes)


def render(originals, axes, choices):
    if len(choices) != len(axes):
        raise ValueError("one choice per axis required")
    edits = [edit for axis, choice in zip(axes, choices) for edit in axis.options[choice].edits]
    result = dict(originals)
    for edit in sorted(edits, key=lambda e: (e.source, e.start, e.end), reverse=True):
        text = result[edit.source]
        result[edit.source] = text[:edit.start] + edit.replacement + text[edit.end:]
    return result


def rank(row):
    scores = row["scores"].values()
    return (sum(value == 100 for value in scores), sum(scores), row["id"])


def select_elites(records, keep, baseline=None):
    unique = {}
    for row in sorted((r for r in records if r.get("scores")), key=rank, reverse=True):
        unique.setdefault(row["object_hash"], row)
    remaining = list(unique.values())
    selected = remaining[:max(1, keep // 2)]
    remaining = remaining[len(selected):]
    names = {name for row in remaining for name in row["scores"]}
    while remaining and len(selected) < keep:
        peaks = {name: max((baseline or {}).get(name, 0),
                          max((r["scores"].get(name, 0) for r in selected), default=0)) for name in names}
        winner = max(remaining, key=lambda row: (
            sum(max(0, row["scores"].get(name, 0) - peaks[name]) for name in names), rank(row)))
        selected.append(winner)
        remaining.remove(winner)
    return selected


def next_population(axes, parents, seen, width, rng):
    sizes = tuple(len(axis.options) for axis in axes)
    total = math.prod(sizes)
    count = min(width, total - len(seen))
    picked = []
    chosen = set(seen)
    if not chosen:
        picked.append((0,) * len(sizes))
        chosen.add(picked[0])
    for _ in range(width * 100):
        if len(picked) >= count:
            break
        if parents and rng.random() < .75:
            left, right = rng.choices(parents, k=2)
            choice = [rng.choice(pair) for pair in zip(left["choices"], right["choices"])]
            for index in rng.sample(range(len(sizes)), rng.randint(1, min(3, len(sizes)))):
                choice[index] = rng.randrange(sizes[index])
            choice = tuple(choice)
        else:
            choice = tuple(rng.randrange(size) for size in sizes)
        if choice not in chosen:
            picked.append(choice)
            chosen.add(choice)
    # Guaranteed exhaustion detection, not endless resampling on a small family.
    if len(picked) < count:
        for choice in itertools.product(*(range(size) for size in sizes)):
            if choice not in chosen:
                picked.append(choice)
            if len(picked) == count:
                break
    return picked


def compile_candidate(candidate_root, unit, output):
    source = source_for_unit(unit)
    flags = flags_for_unit(unit)
    if source is None or not flags:
        raise ValueError(f"unknown unit or missing profile: {unit}")
    output.mkdir(parents=True, exist_ok=True)
    obj = output / "candidate.obj"
    env = dict(os.environ, HOMM3_DIR=str(candidate_root),
               PYTHONPATH=str(common.HOMM3_DIR / "scripts"))
    proc = subprocess.run([
        sys.executable, "-m", "homm3.core.cc_wrap", "--out", str(obj),
        "--src", str(candidate_root / source.relative_to(common.HOMM3_DIR)),
        "--", *flags], capture_output=True, text=True, env=env)
    log = proc.stdout + proc.stderr
    (output / "compile.log").write_text(log)
    if proc.returncode or not obj.is_file():
        raise RuntimeError(log[-6000:])
    return obj


def evaluate(snapshot, output, plans, originals, axes, choices, *, repeat=False):
    sources = render(originals, axes, choices)
    key = digest(json.dumps(sources, sort_keys=True).encode())[:24]
    trial = output / "candidates" / key / ("repeat" if repeat else "first")
    result_path = trial / "result.json"
    if result_path.is_file() and not repeat:
        return json.loads(result_path.read_text())
    trial.mkdir(parents=True, exist_ok=True)
    candidate_root = trial / "tree"
    if not candidate_root.exists():
        # Hard-link only our frozen snapshot, never live user files. Replace
        # edited files atomically so no sibling or snapshot inode is changed.
        shutil.copytree(snapshot, candidate_root, copy_function=os.link, symlinks=True)
    for relative, text in sources.items():
        path = candidate_root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        staged = path.with_name(path.name + ".candidate-new")
        staged.write_text(text)
        staged.replace(path)
    started = time.monotonic()
    row = {"id": key, "choices": choices, "labels": {
        axis.name: axis.options[choice].name for axis, choice in zip(axes, choices)},
        "scores": {}, "source_hashes": {name: digest(text.encode()) for name, text in sources.items()}}
    identities = []
    try:
        for plan in plans:
            directory = trial / plan.unit
            obj = compile_candidate(candidate_root, plan.unit, directory)
            base, target = scoring._normalized_pair(plan, obj.read_bytes())
            scores = scoring._report_scores(plan, base, target, directory)
            row["scores"].update({plan.unit + "|" + symbol: round(value, 4) for symbol, value in scores.items()})
            identities.append(code_identity(base))
        row["object_hash"] = digest(json.dumps(identities).encode())
    except Exception as exc:
        row.update(error=str(exc), scores={})
    row["seconds"] = time.monotonic() - started
    scoring._write_json(result_path, row)
    return row


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--width", type=int, default=60)
    parser.add_argument("--keep", type=int, default=8)
    parser.add_argument("--generations", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--seed", type=int, default=20260908)
    parser.add_argument("--validate-only", action="store_true")
    parser.add_argument("--smoke-only", action="store_true",
                        help="compile baseline and the opposite corner before launching a matrix")
    args = parser.parse_args(argv)
    if not 50 <= args.width <= 60 or not 1 <= args.keep < args.width or min(args.jobs, args.generations) < 1:
        parser.error("width must be 50–60; 1 <= keep < width; jobs/generations positive")
    root = common.HOMM3_DIR
    payload, originals, axes = load_manifest(args.manifest, root)
    units = payload.get("units", ["rmg", "rmg_support", "rmg_terrain"])
    sources = [source_for_unit(unit) for unit in units]
    if not units or len(set(units)) != len(units) or any(path is None for path in sources):
        parser.error("units must be distinct manifest unit names")
    total = math.prod(len(axis.options) for axis in axes)
    print(f"[source-families] {total} combinations; {args.width}/generation; keep {args.keep}; {units}", flush=True)
    if args.validate_only:
        return 0
    inputs = scoring._shared_inputs_digest()
    watched = set(sources) | {root / relative for relative in originals} | {args.manifest.resolve(), status.BASELINE}
    source_digest = scoring._files_digest(watched)
    context = digest(json.dumps([VERSION, payload, inputs, source_digest, args.width, args.keep, args.seed], sort_keys=True).encode())[:20]
    output = root / "build/source-families" / context
    output.mkdir(parents=True, exist_ok=True)
    scoring._write_json(output / "input.json", payload)
    snapshot = output / "snapshot"
    if not snapshot.exists():
        snapshot.mkdir()
        shutil.copytree(root / "include", snapshot / "include")
        shutil.copytree(root / "src", snapshot / "src", ignore=shutil.ignore_patterns("build"))
        (snapshot / "vendor").symlink_to(root / "vendor", target_is_directory=True)
    rows = status.load_baseline()
    plans = []
    for unit, source in zip(units, sources):
        target = scoring.normalize.OBJDIFF / "target" / f"{unit}.c.obj"
        scored = tuple(sorted(key for key in rows if key[0] == unit))
        if not scored or not target.is_file():
            raise ValueError(f"{unit}: run the full build before searching")
        plans.append(scoring.UnitPlan(unit, source, source.read_text(), digest(source.read_bytes()), (),
            scored, scored, scoring._first_pass(unit, target.read_bytes()), context, output / unit, ()))
    checkpoint_path = output / "checkpoint.json"
    checkpoint = json.loads(checkpoint_path.read_text()) if checkpoint_path.exists() else {"generation": 0, "seen": [], "elites": [], "records": []}
    print(f"[source-families] output {output}", flush=True)
    zero = (0,) * len(axes)
    if render(originals, axes, zero) != originals:
        raise ValueError("the first option on every axis must preserve the original source")
    control = evaluate(snapshot, output, plans, originals, axes, zero)
    expected = {"|".join(key): row.cur or 0 for key, row in rows.items() if key[0] in units}
    if control["scores"] != expected:
        raise RuntimeError(f"unchanged-source control failed: {control.get('error', 'scores differ from current build')}")
    corner = tuple(len(axis.options) - 1 for axis in axes)
    smoke = evaluate(snapshot, output, plans, originals, axes, corner)
    if not smoke["scores"]:
        raise RuntimeError(f"opposite-corner compile failed: {smoke.get('error')}")
    repeated = evaluate(snapshot, output, plans, originals, axes, corner, repeat=True)
    if repeated["scores"] != smoke["scores"] or repeated.get("object_hash") != smoke.get("object_hash"):
        raise RuntimeError("opposite-corner code/relocations did not reproduce")
    print("[source-families] unchanged-source control and opposite-corner compile/reproduction passed", flush=True)
    if args.smoke_only:
        return 0
    for generation in range(checkpoint["generation"], args.generations):
        seen = {tuple(choice) for choice in checkpoint["seen"]}
        rng = random.Random(args.seed + generation)
        population = next_population(axes, checkpoint["elites"], seen, args.width, rng)
        if not population:
            print("[source-families] family exhausted; inspect frontier and author the next evidence-based family", flush=True)
            break
        records = []
        attempted = []
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            while population:
                attempted.extend(population)
                futures = [pool.submit(evaluate, snapshot, output, plans, originals, axes, choice) for choice in population]
                for future in as_completed(futures):
                    row = future.result()
                    records.append(row)
                    state = f"{rank(row)[0]} exact" if row["scores"] else "FAILED"
                    valid = sum(bool(record["scores"]) for record in records)
                    print(f"[source-families] g{generation + 1} {valid}/{args.width} scored; {len(records)} attempted; {row['id']} {state}", flush=True)
                valid = sum(bool(row["scores"]) for row in records)
                if valid >= args.width:
                    break
                if len(records) >= args.width * 4:
                    raise RuntimeError("too many failed candidates; inspect compile logs and repair the family")
                population = next_population(axes, checkpoint["elites"], seen | set(attempted), args.width - valid, rng)
        all_records = checkpoint["records"] + records
        elites = select_elites(all_records, args.keep,
                              {"|".join(key): row.cur or 0 for key, row in rows.items()})
        # A second isolated compile validates each retained source candidate.
        for elite in elites:
            reproduced = evaluate(snapshot, output, plans, originals, axes, elite["choices"], repeat=True)
            if (reproduced["scores"] != elite["scores"] or
                    reproduced.get("object_hash") != elite["object_hash"]):
                raise RuntimeError(f"non-reproducible frontier: {elite['id']}")
        if scoring._files_digest(watched) != source_digest or scoring._shared_inputs_digest() != inputs:
            raise RuntimeError("source/toolchain/targets/ledger changed during search; refusing stale checkpoint")
        checkpoint.update(generation=generation + 1, seen=[list(choice) for choice in sorted(seen | set(attempted))], elites=elites, records=all_records)
        scoring._write_json(checkpoint_path, checkpoint)
        frontier = {"context": context, "generation": generation + 1, "source_modified": False,
                    "attempted": len(records), "successful": sum(bool(row["scores"]) for row in records),
                    "distinct_objects": len({row["object_hash"] for row in records if row["scores"]}), "elites": elites,
                    "changes": []}
        for key, old in rows.items():
            if key[0] not in units:
                continue
            name = "|".join(key)
            observations = [row for row in all_records if name in row["scores"]]
            if not observations:
                continue
            best = max(observations, key=lambda row: row["scores"][name])
            low = min(row["scores"][name] for row in observations)
            high = best["scores"][name]
            if low != old.cur or high != old.cur:
                frontier["changes"].append(dict(unit=key[0], symbol=key[1], cur=old.cur, max=old.max, hist=old.hist, low=low, high=high, candidate=best["id"]))
                if high > old.max:
                    print(f"  {key[0]} {key[1]} MAX {old.max:.4f} -> observed {high:.4f} [{best['id']}]", flush=True)
        scoring._write_json(output / f"generation-{generation + 1:04d}.json", frontier)
        print(f"[source-families] {frontier['successful']}/{len(records)} scored, {frontier['distinct_objects']} distinct objects; {len(elites)} reproduced elites; no authored source/MAX changed", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
