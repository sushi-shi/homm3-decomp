"""Batch disposable VC6 translation-unit state search.

Discover every configured function whose clean ``CUR`` is below ``HIST``, group
the functions by translation unit, then compile thirty deterministic declaration
forests per TU.  One candidate object scores every function in that TU, so a
multi-function unit costs thirty compiles rather than thirty compiles per row.

The declarations are inserted before the earliest affected function and exist
only in a source copy under ``build/tu-state-sweep``.  Authored source is never
rewritten.  Higher observations are reproduced with a second compile before
``--bank`` raises MAX (and HIST when a genuinely new all-time peak is found).
CUR always remains the clean-build score.

The declaration generator is adapted from Gruntz's ``permute state`` search.
"""
from __future__ import annotations

import concurrent.futures
import hashlib
import json
import os
import random
import re
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


GENERATOR_VERSION = 2
DEFAULT_SEED = 20260906
DEFAULT_TRIALS = 30
DEFAULT_MIN_FOREST_WIDTH = 10
DEFAULT_MAX_DECLARATIONS = 64
SAFE_SCALAR_TYPES = (
    "char", "unsigned char", "short", "unsigned short", "int",
    "unsigned long",
)
SAFE_CALLING_CONVENTIONS = ("__cdecl", "__fastcall", "__stdcall")
SAFE_ENUM_VALUES = (
    -32768, -1, 0, 1, 2, 7, 31, 255, 256, 1024, 32767, 65535,
)


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
    """Insert a uniquely named copy of *variant* beside every affected RVA."""
    candidate = original
    ident = f"GRUNTZ_TU_STATE_PROBE_{variant.tag.replace('-', '_').upper()}"
    for offset, line, rva in sorted(insertions, reverse=True):
        body = variant.body.replace(ident, f"{ident}_RVA_{rva:08X}")
        candidate = candidate[:offset] + f"{body}#line {line}\n" + candidate[offset:]
    return candidate


def _make_declaration_forest(
        rng: random.Random, ident: str, width: int) -> str:
    atoms: list[tuple[str, str]] = []
    typedef_shapes = (
        lambda name, scalar, index: f"typedef {scalar} {name};\n",
        lambda name, scalar, index: f"typedef {scalar} *{name};\n",
        lambda name, scalar, index: (
            f"typedef {scalar} {name}[{2 + index % 7}];\n"),
        lambda name, scalar, index: (
            f"typedef {scalar} (__cdecl *{name})(int, unsigned long);\n"),
        lambda name, scalar, index: f"typedef const {scalar} *{name};\n",
    )
    for index in range(width):
        scalar = rng.choice(SAFE_SCALAR_TYPES)
        shape = rng.randrange(len(typedef_shapes))
        name = f"{ident}_FOREST_TYPEDEF_{index}"
        atoms.append((f"typedef:{shape}:{index}",
                      typedef_shapes[shape](name, scalar, index)))

    for index in range(width):
        name = f"{ident}_FOREST_CLASS_{index}"
        scalar = rng.choice(SAFE_SCALAR_TYPES)
        constant = rng.choice((1, 2, 3, 7, 15, 31))
        shape = rng.randrange(8)
        if shape == 0:
            body = f"class {name} {{ public: {scalar} m_value; int ProbeRead(int); }};\n"
        elif shape == 1:
            body = (
                f"class {name} {{ private: {scalar} m_value; public: "
                f"int ProbeIdentity(int value) {{ return value; }} protected: "
                f"unsigned long m_state; }};\n")
        elif shape == 2:
            body = (
                f"class {name} {{ public: typedef {scalar} ProbeValue; "
                f"enum ProbeKind {{ PROBE_ZERO = 0, PROBE_LIMIT = {constant} }}; "
                f"ProbeValue m_values[{2 + index % 4}]; }};\n")
        elif shape == 3:
            body = (
                f"class {name} {{ public: static {scalar} s_value; "
                f"static int ProbeStatic(int); int ProbeMember(unsigned long) const; }};\n")
        elif shape == 4:
            body = (
                f"class {name} {{ public: virtual int ProbeVirtual(int); "
                f"virtual unsigned long ProbeWide(unsigned long); }};\n")
        elif shape == 5:
            body = (
                f"class {name} {{ public: int ProbeOverload(int); "
                f"int ProbeOverload(unsigned long); int ProbeOverload(const char *); }};\n")
        elif shape == 6:
            body = (
                f"class {name} {{ private: unsigned int m_low : {1 + index % 7}; "
                f"unsigned int m_high : {1 + (index + 3) % 7}; "
                f"public: int ProbeBits() const; }};\n")
        else:
            pack = (1, 2, 4, 8)[index % 4]
            body = (
                f"#pragma pack(push, {pack})\nclass {name} {{ public: char m_tag; "
                f"{scalar} m_value; int ProbePacked(int value) "
                f"{{ return value ^ {constant}; }} }};\n#pragma pack(pop)\n")
        atoms.append((f"class:{shape}:{index}", body))

    prototype_shapes = (
        lambda name, convention, scalar: f"{scalar} {convention} {name}({scalar});\n",
        lambda name, convention, scalar: (
            f"int {convention} {name}(int, unsigned long);\n"),
        lambda name, convention, scalar: (
            f"{scalar} *{convention} {name}({scalar} *, unsigned int);\n"),
        lambda name, convention, scalar: (
            f"void {convention} {name}(const {scalar} *, const {scalar} *);\n"),
    )
    for index in range(width):
        name = f"{ident}_FOREST_PROTOTYPE_{index}"
        convention = rng.choice(SAFE_CALLING_CONVENTIONS)
        scalar = rng.choice(SAFE_SCALAR_TYPES)
        shape = rng.randrange(len(prototype_shapes))
        atoms.append((f"prototype:{shape}:{index}",
                      prototype_shapes[shape](name, convention, scalar)))

    function_shapes = (
        lambda name, convention, constant: (
            f"static int {convention} {name}(int value) {{ return value; }}\n"),
        lambda name, convention, constant: (
            f"static int {convention} {name}(int value) "
            f"{{ return value ^ {constant}; }}\n"),
        lambda name, convention, constant: (
            f"static unsigned long {convention} {name}(unsigned long left, "
            f"unsigned long right) {{ return (left + right) ^ {constant}UL; }}\n"),
        lambda name, convention, constant: (
            f"static int {convention} {name}(int left, int right) "
            f"{{ return left < right ? left + {constant} : right - {constant}; }}\n"),
    )
    for index in range(width):
        name = f"{ident}_FOREST_FUNCTION_{index}"
        convention = rng.choice(SAFE_CALLING_CONVENTIONS)
        constant = rng.choice((1, 2, 3, 7, 15, 31, 63, 127))
        shape = rng.randrange(len(function_shapes))
        atoms.append((f"function:{shape}:{index}",
                      function_shapes[shape](name, convention, constant)))
    rng.shuffle(atoms)
    return "".join(body for _label, body in atoms)


def make_variants(count: int, seed: int) -> tuple[Variant, ...]:
    """Generate the same forest sequence as Gruntz's seed/family runner."""
    rng = random.Random(seed)
    variants = []
    for trial in range(1, count + 1):
        tag = f"{seed:08x}-{trial:04d}-{rng.getrandbits(32):08x}"
        # Preserve the audited Gruntz spelling too: VC6's state can depend on
        # identifier-table population, not merely declaration shapes.
        ident = f"GRUNTZ_TU_STATE_PROBE_{tag.replace('-', '_').upper()}"
        width = DEFAULT_MIN_FOREST_WIDTH + (
            (trial - 1) %
            (DEFAULT_MAX_DECLARATIONS - DEFAULT_MIN_FOREST_WIDTH + 1))
        forest = _make_declaration_forest(rng, ident, width)

        # Keep RNG consumption identical to the Gruntz multi-family generator;
        # otherwise trial N would not reproduce an audited Gruntz forest.
        repeat = 1 + rng.randrange(4)
        for _ in range(repeat):
            rng.choice(SAFE_SCALAR_TYPES)
        enum_count = 1 + rng.randrange(8)
        enum_values = [rng.choice(SAFE_ENUM_VALUES) for _ in range(enum_count)]
        rng.shuffle(enum_values)
        member_count = 1 + rng.randrange(6)
        for _ in range(member_count):
            rng.choice(SAFE_SCALAR_TYPES)
            if rng.randrange(3) == 0:
                rng.randrange(4)
        rng.choice((1, 2, 4, 8))
        for _ in range(2 + rng.randrange(5)):
            rng.choice(SAFE_SCALAR_TYPES)
        rng.randrange(3)
        rng.choice((0, 1, 3, 7, 15, 31))
        for _ in range(repeat):
            rng.choice(SAFE_SCALAR_TYPES)
        for _ in range(repeat):
            rng.choice(SAFE_ENUM_VALUES)
        for _ in range(repeat):
            rng.choice(SAFE_SCALAR_TYPES)
        rng.choice((0, 1, 3, 7, 15, 31, 63, 127))
        rng.randrange(4)
        include_choices = list(range(10))
        rng.shuffle(include_choices)
        variants.append(Variant(trial, tag, forest))
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
    return {
        fn["name"]: float(fn.get("fuzzy_match_percent") or 0.0)
        for fn in report["units"][0].get("functions", [])
        if fn.get("name") in wanted
    }


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
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, sort_keys=True) + "\n")
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
        "scores": scores,
    }
    if cache:
        _write_json(_trial_path(plan, variant.trial), payload)
    return payload


def affected_by_unit(rows: dict) -> dict[str, tuple[tuple[str, str], ...]]:
    grouped: dict[str, list[tuple[str, str]]] = defaultdict(list)
    for key, row in rows.items():
        if row.cur is not None and row.cur < row.hist - 1e-9:
            grouped[key[0]].append(key)
    return {unit: tuple(sorted(keys)) for unit, keys in grouped.items()}


def _plans(rows: dict, units: set[str] | None, seed: int, trials: int) -> list[UnitPlan]:
    affected = affected_by_unit(rows)
    if units is not None:
        unknown = units - set(affected)
        if unknown:
            common.die("requested unit(s) have no CUR < HIST rows: "
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
        rvas = tuple(row.rva for key in keys
                     if (row := rows[key]).rva is not None)
        insertions = insertions_for(original, rvas)
        scored = tuple(sorted(
            key for key, row in rows.items()
            if key[0] == unit and row.cur is not None))
        identity = hashlib.sha256()
        for payload in (
                source_bytes, target_bytes, compgen, symbol_names,
                f"generator={GENERATOR_VERSION};seed={seed};trials={trials};"
                f"insertions={insertions}".encode()):
            identity.update(hashlib.sha256(payload).digest())
        context = identity.hexdigest()[:16]
        result_dir = (common.HOMM3_DIR / "build/tu-state-sweep/results" /
                      unit / context)
        result_dir.mkdir(parents=True, exist_ok=True)
        plans.append(UnitPlan(
            unit, source, original, _sha256(source_bytes), insertions,
            keys, scored, _first_pass(unit, target_bytes), context, result_dir))
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
    rows = status.load_baseline()
    units = ({part.strip() for part in args.unit.split(",") if part.strip()}
             if args.unit else None)
    variants = make_variants(args.trials, args.seed)
    plans = _plans(rows, units, args.seed, args.trials)
    baseline_digest = _sha256(status.BASELINE.read_bytes())
    source_digests = {plan.source: plan.source_digest for plan in plans}
    affected_count = sum(len(plan.affected) for plan in plans)
    print(f"[vc6 state-sweep] {affected_count} CUR < HIST function(s) in "
          f"{len(plans)} TU(s); {args.trials} trials/TU; {args.jobs} worker(s)")

    results: dict[str, list[dict]] = defaultdict(list)
    counts = Counter()
    tasks = [(plan, variant) for plan in plans for variant in variants]
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(run_trial, plan, variant): (plan, variant)
            for plan, variant in tasks
        }
        for future in concurrent.futures.as_completed(futures):
            plan, variant = futures[future]
            try:
                result = future.result()
            except Exception as exc:
                for pending in futures:
                    pending.cancel()
                common.die(str(exc))
            results[plan.unit].append(result)
            counts[plan.unit] += 1
            done = counts[plan.unit]
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
    by_trial = {variant.trial: variant for variant in variants}
    reproduced = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(run_trial, by_unit[unit], by_trial[trial], cache=False):
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

    live_hashes = status.source_hashes()
    updated, changes = bank_rows(rows, reproduced, live_hashes)
    summary = {
        "seed": args.seed,
        "trials_per_tu": args.trials,
        "affected_functions": affected_count,
        "translation_units": len(plans),
        "compiled_trials": len(tasks),
        "reproduced_improvements": [
            {
                "unit": key[0], "function": key[1], "old_max": old_max,
                "new_max": new_max, "old_hist": old_hist,
                "new_hist": new_hist, "trial": best[key][1],
            }
            for key, old_max, new_max, old_hist, new_hist in changes
        ],
    }
    summary_path = (common.HOMM3_DIR / "build/tu-state-sweep" /
                    f"summary-{args.seed}-{args.trials}.json")
    _write_json(summary_path, summary)
    for item in summary["reproduced_improvements"]:
        print(f"[vc6 state-sweep] BANK {item['unit']} {item['function']}: "
              f"MAX {item['old_max']:.4f} -> {item['new_max']:.4f} "
              f"(trial {item['trial']})")
    if args.bank:
        status.write_baseline(updated)
        print(f"[vc6 state-sweep] banked {len(changes)} reproduced improvement(s) "
              f"-> {status.BASELINE}")
    else:
        print(f"[vc6 state-sweep] dry run: {len(changes)} reproducible improvement(s); "
              "pass --bank to update MAX/HIST")
    print(f"[vc6 state-sweep] summary: {summary_path}")
    return 0
