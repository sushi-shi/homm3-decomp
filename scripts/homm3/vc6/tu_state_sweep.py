"""Batch disposable VC6 translation-unit state search.

Discover every configured function whose preserved ``MAX`` is below ``HIST``,
group the functions by translation unit, then compile deterministic random
include sets per TU. One candidate object scores every function in that TU.

Each trial adds one shuffled block of five to ten project headers absent from
that TU's transitive include closure. The block exists only in a source copy under
``build/tu-state-sweep``; authored source is never rewritten. Higher
observations are reproduced with a second compile before
``--bank`` raises MAX (and HIST when a genuinely new all-time peak is found).
CUR always remains the clean-build score.

The include-state strategy follows Gruntz's TU-state search.
"""
from __future__ import annotations

import concurrent.futures
import hashlib
import json
import os
import random
import re
import shutil
import subprocess
import tempfile
from collections import Counter, defaultdict
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

from homm3.build import canonicalize_data_symbols as canon
from homm3.build import normalize_objs as normalize
from homm3.core import common
from homm3.match import status
from homm3.vc6._unit import compile_text, source_for_unit


GENERATOR_VERSION = 7
DEFAULT_SEED = 20260906
DEFAULT_TRIALS = 30
MIN_HEADERS_PER_TRIAL = 5
MAX_HEADERS_PER_TRIAL = 10
_INCLUDE_DIRECTIVE = re.compile(
    r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
_MACRO_DEFINITION = re.compile(
    r'^\s*#\s*define\s+([A-Za-z_]\w*)', re.M)
_IDENTIFIER = re.compile(r'\b[A-Za-z_]\w*\b')
# These headers proved context-dependent or declaration-changing in the
# all-TU compatibility census. They are not general parser-state inputs.
UNSAFE_RANDOM_INCLUDE_HEADERS = frozenset({
    "DC_input.h",
    "DC_precompiledheaders.h",
    "ResSw.h",
    "ai_spellvalue.h",
    "ai_tactical.h",
    "army.h",
    "creaturetype.h",
    "customcampaign_legacy.h",
    "herodefs.h",
    "homm3_minmax.h",
    "pcx.h",
    "quest.h",
    "resourcemanager.h",
    "resourcemanager_sound.h",
    "singleselectionwindow.h",
    "singleselectionwindow_priv.h",
    "timer.h",
    "winmm_thunks.h",
})
# These otherwise useful headers conflict with declarations intentionally kept
# local to one compiland.  The all-TU include compatibility census identified
# each pairing: keep the header available to every other TU.
UNSAFE_RANDOM_INCLUDE_HEADERS_BY_UNIT = {
    "diff": frozenset({"kbwin.h"}),
    "singleselectionwindow": frozenset({"resourcemanager_cache_result.h"}),
    "spells": frozenset({"autostrptr.h"}),
}


@dataclass(frozen=True)
class Variant:
    trial: int
    tag: str
    body: str

    def block(self, logical_line: int) -> str:
        return f"{self.body}#line {logical_line}\n"


@dataclass(frozen=True)
class UnitPlan:
    unit: str
    source: Path
    original: str
    source_digest: str
    insertions: tuple[tuple[int, int, int], ...]
    affected: tuple[tuple[str, str], ...]
    scored: tuple[tuple[str, str], ...]
    target_first: bytes
    context: str
    result_dir: Path
    include_pool: tuple[str, ...]


def _sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def _logical_line_at(text: str, offset: int) -> int:
    directive = re.compile(r'^\s*#\s*line\s+(\d+)(?:\s+"[^"]*")?')
    logical = 1
    for line in text[:offset].splitlines(keepends=True):
        match = directive.match(line)
        logical = int(match.group(1)) if match else logical + 1
    return logical


def _leading_metadata_offset(text: str, marker_offset: int) -> int:
    lines = text[:marker_offset].splitlines(keepends=True)
    offset = marker_offset
    for line in reversed(lines):
        stripped = line.strip()
        if not stripped or stripped.startswith("//"):
            offset -= len(line)
            continue
        break
    return offset


def _top_level_insertion_offset(text: str) -> int:
    offset = 0
    continuation = False
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if continuation or not stripped or stripped.startswith("#"):
            offset += len(line)
            continuation = stripped.endswith("\\")
            continue
        break
    return offset


def insertion_for(text: str, rvas: tuple[int, ...]) -> tuple[int, int]:
    """Insertion before the earliest affected VA/VA_COMPGEN source marker."""
    positions = []
    for rva in rvas:
        va = rva + 0x00400000
        pattern = re.compile(
            rf"^[ \t]*VA(?:_COMPGEN)?\(0x{va:08x},", re.I | re.M)
        positions.extend(match.start() for match in pattern.finditer(text))
    offset = (_leading_metadata_offset(text, min(positions)) if positions
              else _top_level_insertion_offset(text))
    return offset, _logical_line_at(text, offset)


def insertions_for(text: str, rvas: tuple[int, ...]) -> tuple[tuple[int, int, int], ...]:
    """Return one Gruntz-style insertion beside every affected function."""
    insertions = []
    for rva in sorted(set(rvas)):
        offset, line = insertion_for(text, (rva,))
        insertions.append((offset, line, rva))
    return tuple(insertions)


def insert_variant(original: str, insertions: tuple[tuple[int, int, int], ...],
                   variant: Variant) -> str:
    """Insert a variant at each requested site (one site for include sweeps)."""
    candidate = original
    for offset, line, rva in sorted(insertions, reverse=True):
        separator = "\n" if offset and candidate[offset - 1] != "\n" else ""
        candidate = (candidate[:offset] + separator + f"{variant.body}#line {line}\n"
                     + candidate[offset:])
    return candidate


def _initial_include_insertion(text: str) -> tuple[int, int]:
    """Insert at an unconditional boundary in the TU's initial directives.

    Do not evaluate #if expressions: even a currently active branch is an
    unsafe home for a disposable include. Stop before the first declaration,
    and never split comments or backslash-continued directives.
    """
    from homm3.retail_labels.source import mask_lexical_noise

    # Splice logical lines before lexing, retaining a map to physical offsets.
    logical, boundaries = [], []
    pending, physical = "", 0
    for line in text.splitlines(keepends=True):
        physical += len(line)
        if re.search(r'\\\r?\n$', line):
            pending += re.sub(r'\\\r?\n$', '', line)
            continue
        logical.append(pending + line)
        boundaries.append(physical)
        pending = ""
    # An unfinished continued directive is not a safe insertion boundary.
    spliced = "".join(logical) + pending
    masked = mask_lexical_noise(spliced)
    # The shared masker preserves newlines inside comments. Track their spans
    # too, so a blank masked line cannot place headers inside a block comment.
    tokens = re.compile(
        r'/\*[\s\S]*?(?:\*/|\Z)|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
    comments = [(m.start(), m.end()) for m in tokens.finditer(spliced)
                if m.group().startswith("/*")]
    offset = position = depth = 0
    for line, physical in zip(logical, boundaries):
        end = position + len(line)
        stripped = masked[position:end].strip()
        directive = re.match(r'#\s*(\w+)', stripped)
        if stripped and not directive:
            break
        if directive:
            kind = directive.group(1)
            if kind in ("if", "ifdef", "ifndef"):
                depth += 1
            elif kind == "endif":
                if depth == 0:
                    break
                depth -= 1
            elif kind in ("else", "elif") and depth == 0:
                break
        if depth == 0 and not any(start < end < stop for start, stop in comments):
            offset = physical
        position = end
    return offset, _logical_line_at(text, offset)


def _files_digest(paths) -> str:
    identity = hashlib.sha256()
    for path in sorted(set(paths)):
        identity.update(str(path.resolve()).encode())
        identity.update(b"\0")
        identity.update(hashlib.sha256(path.read_bytes()).digest())
    return identity.hexdigest()


def _shared_inputs_digest() -> str:
    """Invalidate observations when compiler, headers or scoring inputs move.

    A conservative superset is deliberate: the random include set can reach
    any project header, and normalization code is itself part of the verdict.
    The same fingerprint is checked again before any bank is written.
    """
    from homm3.core.cc_wrap import msvc_dir

    root = common.HOMM3_DIR
    paths = []
    for relative in ("include", "vendor/zlib-1.1.3"):
        paths.extend(path for path in (root / relative).rglob("*")
                     if path.is_file())
    paths.extend((root / "scripts/homm3").rglob("*.py"))
    paths.extend((root / "src").rglob("*.h"))
    paths.extend((normalize.OBJDIFF / "target").glob("*.c.obj"))
    paths.extend(path for path in (
        root / "config/units.toml", normalize.COMPGEN_MANIFEST,
        normalize.SYMBOL_NAMES) if path.is_file())
    compiler = msvc_dir()
    for relative in ("bin", "include"):
        paths.extend(path for path in (compiler / relative).rglob("*")
                     if path.is_file())
    for command in ("objdiff-cli", "wine"):
        if executable := shutil.which(command):
            paths.append(Path(executable))
    environment = {key: os.environ.get(key) for key in ("CL", "_CL_")}
    return _sha256((_files_digest(paths) + json.dumps(environment)).encode())


@lru_cache(maxsize=None)
def _project_header_closure(header: str) -> frozenset[str]:
    """One project header and all project headers it includes transitively."""
    header_root = common.HOMM3_DIR / "include"
    pending = [header]
    visited = set()
    while pending:
        relative = pending.pop()
        if relative in visited:
            continue
        visited.add(relative)
        path = header_root / relative
        if not path.is_file():
            continue
        contents = path.read_text(errors="replace")
        for child in _INCLUDE_DIRECTIVE.findall(contents):
            child = child.replace("\\", "/")
            if (header_root / child).is_file():
                pending.append(child)
    return frozenset(visited)


@lru_cache(maxsize=None)
def _project_header_macros(header: str) -> frozenset[str]:
    """Macros introduced by one project header's project-header closure."""
    header_root = common.HOMM3_DIR / "include"
    macros = set()
    for relative in _project_header_closure(header):
        path = header_root / relative
        if path.is_file():
            macros.update(_MACRO_DEFINITION.findall(
                path.read_text(errors="replace")))
    return frozenset(macros)


def _project_header_pool(text: str, unit: str = "") -> tuple[str, ...]:
    """Absent, generally compatible headers unable to macro-rewrite the TU."""
    header_root = common.HOMM3_DIR / "include"
    direct = {name.replace("\\", "/")
              for name in _INCLUDE_DIRECTIVE.findall(text)}
    included = {name.lower() for name in direct}
    for header in direct:
        if (header_root / header).is_file():
            included.update(name.lower()
                            for name in _project_header_closure(header))
    source_identifiers = set(_IDENTIFIER.findall(text))
    unit_exclusions = UNSAFE_RANDOM_INCLUDE_HEADERS_BY_UNIT.get(
        unit, frozenset())
    headers = []
    for path in sorted(header_root.rglob("*.h")):
        relative = path.relative_to(header_root).as_posix()
        if (relative.lower() not in included
                and relative not in UNSAFE_RANDOM_INCLUDE_HEADERS
                and relative not in unit_exclusions
                and not (_project_header_macros(relative) & source_identifiers)):
            headers.append(relative)
    return tuple(headers)


def make_variants(
        count: int, seed: int, unit: str,
        headers: tuple[str, ...]) -> tuple[Variant, ...]:
    """Choose 5-10 transitively absent headers in random order per TU/trial."""
    if len(headers) < MIN_HEADERS_PER_TRIAL:
        common.die(f"{unit}: fewer than {MIN_HEADERS_PER_TRIAL} unused headers")
    unit_seed = int.from_bytes(
        hashlib.sha256(f"{seed}:{unit}".encode()).digest()[:8], "big")
    rng = random.Random(unit_seed)
    variants = []
    maximum = min(MAX_HEADERS_PER_TRIAL, len(headers))
    for trial in range(1, count + 1):
        header_count = rng.randint(MIN_HEADERS_PER_TRIAL, maximum)
        selected = rng.sample(headers, header_count)
        rng.shuffle(selected)
        tag = f"{seed:08x}-{trial:04d}-{rng.getrandbits(32):08x}"
        body = "".join(f'#include "{header}"\n' for header in selected)
        variants.append(Variant(trial, tag, body))
    return tuple(variants)


@lru_cache(maxsize=None)
def _claims(unit: str):
    if not normalize.COMPGEN_MANIFEST.is_file():
        return (), frozenset()
    return (
        canon.load_compgen_claims(normalize.COMPGEN_MANIFEST, unit),
        canon.load_compgen_claim_names(normalize.COMPGEN_MANIFEST, unit),
    )


def _first_pass(unit: str, payload: bytes) -> bytes:
    claims, accounted = _claims(unit)
    result = canon.canonicalize_coff(
        payload, claims, compgen_accounted=accounted)
    return normalize._drop_data_sections(result.data)


def _normalized_pair(plan: UnitPlan, candidate: bytes) -> tuple[bytes, bytes]:
    base = _first_pass(plan.unit, candidate)
    target = plan.target_first
    base, _ = normalize._retain_matching_target_padding(base, target)
    base, _ = normalize._canonicalize_except_list_literals(base, target)
    target, _, _ = normalize._canonicalize_equivalent_relocations(
        base, target, normalize._retail_symbol_rvas())
    base, _ = normalize._canonicalize_matching_eh_handler_owners(base, target)
    return base, target


def _report_scores(
        plan: UnitPlan, base: bytes, target: bytes, directory: Path) -> dict[str, float]:
    base_path = directory / "candidate.c.obj"
    target_path = directory / "target.c.obj"
    base_path.write_bytes(base)
    target_path.write_bytes(target)
    project = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "build_base": False,
        "build_target": False,
        "units": [{
            "name": plan.unit,
            "base_path": str(base_path.resolve()),
            "target_path": str(target_path.resolve()),
            "scratch": {"platform": "win32", "compiler": "msvc6.0"},
        }],
    }
    (directory / "objdiff.json").write_text(json.dumps(project) + "\n")
    env = dict(os.environ)
    env["RAYON_NUM_THREADS"] = "1"
    proc = subprocess.run(
        ["objdiff-cli", "-C", str(directory), "-L", "error", "report",
         "generate", "-o", "report.json"],
        capture_output=True, text=True, env=env)
    if proc.returncode:
        raise RuntimeError((proc.stdout + proc.stderr).strip())
    report = json.loads((directory / "report.json").read_text())
    wanted = {key[1] for key in plan.scored}
    observed = {
        fn["name"]: float(fn.get("fuzzy_match_percent") or 0.0)
        for fn in report["units"][0].get("functions", [])
        if fn.get("name") in wanted
    }
    # A header can change whether a retained body is emitted at all. Missing
    # candidate functions are a score change too, never silently omit them.
    return {symbol: observed.get(symbol, 0.0) for symbol in sorted(wanted)}


def _trial_path(plan: UnitPlan, trial: int) -> Path:
    return plan.result_dir / f"trial-{trial:04d}.json"


def _read_cached(plan: UnitPlan, variant: Variant) -> dict | None:
    path = _trial_path(plan, variant.trial)
    if not path.is_file():
        return None
    try:
        payload = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError):
        return None
    if (payload.get("context") != plan.context or
            payload.get("tag") != variant.tag):
        return None
    return payload


def _write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    # Separate sweep processes may share a resumable cache.  A fixed `.tmp`
    # sibling lets their atomic writes clobber one another before rename.
    with tempfile.NamedTemporaryFile(
            mode="w", encoding="utf-8", dir=path.parent,
            prefix=path.name + ".", suffix=".tmp", delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(json.dumps(payload, sort_keys=True) + "\n")
    temporary.replace(path)


def run_trial(plan: UnitPlan, variant: Variant, *, cache: bool = True) -> dict:
    if cache and (cached := _read_cached(plan, variant)) is not None:
        return cached
    candidate = insert_variant(plan.original, plan.insertions, variant)
    scratch_root = common.HOMM3_DIR / "build/tu-state-sweep/tmp"
    scratch_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
            prefix=f"{plan.unit}-{variant.trial:04d}-", dir=scratch_root) as raw:
        directory = Path(raw)
        obj, log = compile_text(
            candidate, plan.unit, directory, "candidate", with_listing=False)
        if obj is None:
            raise RuntimeError(f"{plan.unit} trial {variant.trial}: {log}")
        base, target = _normalized_pair(plan, obj.read_bytes())
        scores = _report_scores(plan, base, target, directory)
    payload = {
        "context": plan.context,
        "trial": variant.trial,
        "tag": variant.tag,
        "headers": _INCLUDE_DIRECTIVE.findall(variant.body),
        "scores": scores,
    }
    if cache:
        _write_json(_trial_path(plan, variant.trial), payload)
    return payload


def affected_by_unit(rows: dict) -> dict[str, tuple[tuple[str, str], ...]]:
    grouped: dict[str, list[tuple[str, str]]] = defaultdict(list)
    for key, row in rows.items():
        if row.cur is not None and row.max < row.hist - 1e-9:
            grouped[key[0]].append(key)
    return {unit: tuple(sorted(keys)) for unit, keys in grouped.items()}


def _plans(rows: dict, units: set[str] | None, seed: int, trials: int,
           inputs_digest: str) -> list[UnitPlan]:
    affected = affected_by_unit(rows)
    if units is not None:
        unknown = units - set(affected)
        if unknown:
            common.die("requested unit(s) have no MAX < HIST rows: "
                       + ", ".join(sorted(unknown)))
        affected = {unit: keys for unit, keys in affected.items() if unit in units}
    plans = []
    compgen = (normalize.COMPGEN_MANIFEST.read_bytes()
               if normalize.COMPGEN_MANIFEST.is_file() else b"")
    symbol_names = (normalize.SYMBOL_NAMES.read_bytes()
                    if normalize.SYMBOL_NAMES.is_file() else b"")
    for unit, keys in sorted(affected.items()):
        source = source_for_unit(unit)
        target = normalize.OBJDIFF / "target" / f"{unit}.c.obj"
        if source is None or not target.is_file():
            common.die(f"{unit}: configured source or retail target object missing")
        original = source.read_text()
        source_bytes = source.read_bytes()
        target_bytes = target.read_bytes()
        insertion_offset, insertion_line = _initial_include_insertion(original)
        insertions = ((insertion_offset, insertion_line, 0),)
        include_pool = _project_header_pool(original, unit)
        scored = tuple(sorted(
            key for key, row in rows.items()
            if key[0] == unit and row.cur is not None))
        identity = hashlib.sha256()
        for payload in (
                source_bytes, target_bytes, compgen, symbol_names,
                inputs_digest.encode(), repr(scored).encode(),
                f"generator={GENERATOR_VERSION};seed={seed};trials={trials};"
                f"insertions={insertions};headers={include_pool}".encode()):
            identity.update(hashlib.sha256(payload).digest())
        context = identity.hexdigest()[:16]
        result_dir = (common.HOMM3_DIR / "build/tu-state-sweep/results" /
                      unit / context)
        result_dir.mkdir(parents=True, exist_ok=True)
        plans.append(UnitPlan(
            unit, source, original, _sha256(source_bytes), insertions,
            keys, scored, _first_pass(unit, target_bytes), context, result_dir,
            include_pool))
    return plans


def _best_results(plans: list[UnitPlan], results: dict) -> dict:
    best = {}
    for plan in plans:
        for variant_result in results[plan.unit]:
            for symbol, score in variant_result["scores"].items():
                key = (plan.unit, symbol)
                previous = best.get(key)
                candidate = (round(score, 4), variant_result["trial"])
                if previous is None or candidate > previous:
                    best[key] = candidate
    return best


def _observed_score_changes(plans: list[UnitPlan], results: dict, rows: dict) -> list[dict]:
    """Summarize every function whose score moved in any candidate TU."""
    changes = []
    for plan in plans:
        observations = results.get(plan.unit, ())
        for key in plan.scored:
            row = rows[key]
            scored = [
                (round(result["scores"][key[1]], 4), result["trial"],
                 result.get("headers", []))
                for result in observations if key[1] in result["scores"]
            ]
            if not scored:
                continue
            low = min(scored)
            high = max(scored)
            if low[0] == row.cur and high[0] == row.cur:
                continue
            changes.append({
                "unit": key[0],
                "function": key[1],
                "cur": row.cur,
                "max": row.max,
                "hist": row.hist,
                "lowest": low[0],
                "lowest_trial": low[1],
                "lowest_headers": low[2],
                "highest": high[0],
                "highest_trial": high[1],
                "highest_headers": high[2],
            })
    return changes


def bank_rows(rows: dict, reproduced: dict, live_hashes: dict) -> tuple[dict, list]:
    """Pure bank step: retain CUR, raise MAX/HIST for reproduced observations."""
    updated = dict(rows)
    changes = []
    for key, score in sorted(reproduced.items()):
        old = rows.get(key)
        if old is None or score <= old.max + 1e-9:
            continue
        if old.src_hash is not None and live_hashes.get(key) != old.src_hash:
            continue
        maximum = round(score, 4)
        historical = max(old.hist, maximum)
        updated[key] = status.MatchRow(
            old.cur, maximum, historical, old.rva, old.src_hash)
        changes.append((key, old.max, maximum, old.hist, historical))
    return updated, changes


def run(args) -> int:
    if args.trials < 1 or args.jobs < 1:
        common.die("state-sweep requires positive --trials and --jobs")
    inputs_digest = _shared_inputs_digest()
    rows = status.load_baseline()
    units = ({part.strip() for part in args.unit.split(",") if part.strip()}
             if args.unit else None)
    plans = _plans(rows, units, args.seed, args.trials, inputs_digest)
    variants_by_unit = {
        plan.unit: make_variants(
            args.trials, args.seed, plan.unit, plan.include_pool)
        for plan in plans
    }
    baseline_digest = _sha256(status.BASELINE.read_bytes())
    source_digests = {plan.source: plan.source_digest for plan in plans}
    affected_count = sum(len(plan.affected) for plan in plans)
    print(f"[vc6 state-sweep] {affected_count} MAX < HIST function(s) in "
          f"{len(plans)} TU(s); {args.trials} trials/TU; {args.jobs} worker(s)")

    results: dict[str, list[dict]] = defaultdict(list)
    counts = Counter()
    tasks = [(plan, variant)
             for plan in plans for variant in variants_by_unit[plan.unit]]
    failures = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(run_trial, plan, variant): (plan, variant)
            for plan, variant in tasks
        }
        for future in concurrent.futures.as_completed(futures):
            plan, variant = futures[future]
            counts[plan.unit] += 1
            done = counts[plan.unit]
            try:
                result = future.result()
            except Exception as exc:
                failures.append({
                    "unit": plan.unit, "trial": variant.trial,
                    "headers": _INCLUDE_DIRECTIVE.findall(variant.body),
                    "error": str(exc),
                })
                first_error = str(exc).splitlines()[0]
                print(f"[vc6 state-sweep] SKIP {plan.unit} "
                      f"trial {variant.trial}: {first_error}", flush=True)
            else:
                results[plan.unit].append(result)
            if done == args.trials or done % 10 == 0:
                print(f"[vc6 state-sweep] {plan.unit}: {done}/{args.trials}",
                      flush=True)

    best = _best_results(plans, results)
    winners: dict[tuple[str, int], list[tuple[str, str]]] = defaultdict(list)
    for plan in plans:
        for key in plan.scored:
            candidate = best.get(key)
            if candidate is not None and candidate[0] > rows[key].max + 1e-9:
                winners[(plan.unit, candidate[1])].append(key)
    print(f"[vc6 state-sweep] {len(winners)} winning TU/trial pair(s) require reproduction")

    by_unit = {plan.unit: plan for plan in plans}
    reproduced = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(
                run_trial, by_unit[unit],
                variants_by_unit[unit][trial - 1], cache=False):
                (unit, trial, keys)
            for (unit, trial), keys in winners.items()
        }
        for future in concurrent.futures.as_completed(futures):
            unit, trial, keys = futures[future]
            result = future.result()
            for key in keys:
                observed = best[key][0]
                repeated = round(result["scores"].get(key[1], 0.0), 4)
                if repeated == observed:
                    reproduced[key] = repeated
                else:
                    print(f"[vc6 state-sweep] NOT REPRODUCIBLE {unit} {key[1]}: "
                          f"{observed:.4f} -> {repeated:.4f}")

    source_changed = [
        str(path) for path, digest in source_digests.items()
        if _sha256(path.read_bytes()) != digest
    ]
    if source_changed:
        common.die("authored source changed during sweep: " + ", ".join(source_changed))
    if _sha256(status.BASELINE.read_bytes()) != baseline_digest:
        common.die("match_baseline.tsv changed during sweep; refusing stale bank")
    if _shared_inputs_digest() != inputs_digest:
        common.die("compiler, headers or scoring inputs changed during sweep; "
                   "refusing stale bank")

    live_hashes = status.source_hashes()
    updated, changes = bank_rows(rows, reproduced, live_hashes)
    observed_changes = _observed_score_changes(plans, results, rows)
    successful_trials = sum(len(unit_results)
                            for unit_results in results.values())
    score_observations = sum(
        len(result["scores"])
        for unit_results in results.values() for result in unit_results)
    summary = {
        "generator": "random-project-includes",
        "generator_version": GENERATOR_VERSION,
        "inputs_digest": inputs_digest,
        "contexts": {plan.unit: plan.context for plan in plans},
        "seed": args.seed,
        "trials_per_tu": args.trials,
        "affected_functions": affected_count,
        "translation_units": len(plans),
        "attempted_trials": len(tasks),
        "successful_trials": successful_trials,
        "score_observations": score_observations,
        "failed_trials": sorted(
            failures, key=lambda item: (item["unit"], item["trial"])),
        "observed_score_changes": observed_changes,
        "reproduced_improvements": [
            {
                "unit": key[0], "function": key[1], "old_max": old_max,
                "new_max": new_max, "old_hist": old_hist,
                "new_hist": new_hist, "trial": best[key][1],
            }
            for key, old_max, new_max, old_hist, new_hist in changes
        ],
    }
    scope = hashlib.sha256(
        ",".join(f"{plan.unit}:{plan.context}" for plan in plans).encode()
    ).hexdigest()[:8]
    summary_path = (common.HOMM3_DIR / "build/tu-state-sweep" /
                    f"summary-includes-{args.seed}-{args.trials}-{scope}.json")
    _write_json(summary_path, summary)
    for item in summary["reproduced_improvements"]:
        action = "BANK" if args.bank else "WOULD BANK"
        print(f"[vc6 state-sweep] {action} {item['unit']} {item['function']}: "
              f"MAX {item['old_max']:.4f} -> {item['new_max']:.4f} "
              f"(trial {item['trial']})")
    print(f"[vc6 state-sweep] captured {len(observed_changes)} function(s) "
          "whose score changed in at least one candidate TU")
    print(f"[vc6 state-sweep] {successful_trials}/{len(tasks)} candidate TU(s) "
          f"scored; {score_observations} function-score observation(s)")
    if args.bank:
        status.write_baseline(updated)
        status.write_readme(status.load_report())
        print(f"[vc6 state-sweep] banked {len(changes)} reproduced improvement(s) "
              f"-> {status.BASELINE}")
    else:
        print(f"[vc6 state-sweep] dry run: {len(changes)} reproducible improvement(s); "
              "pass --bank to update MAX/HIST")
    print(f"[vc6 state-sweep] summary: {summary_path}")
    return 0
