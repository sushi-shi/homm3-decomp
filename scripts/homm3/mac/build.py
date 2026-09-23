"""Compile admitted shared C++ bodies with CodeWarrior and compare Mac PEF bytes."""
from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import fcntl
import shutil

from homm3.core import common, inputs
from homm3.mac.object import CodeHunk, DataHunk, parse_code_hunks, parse_data_hunks, parse_metadata_hunks, select_hunk
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, LinkedCode, ResolvedCall, ResolvedData, ResolvedJumpTable, link_code
from homm3.mac.source import Pair, candidate_source, compile_scope, load_pairs, source_identity
from homm3.mac import call_report, calls, profiles, reports, symbols, toc, toolchain


ROOT = common.HOMM3_DIR
REPORT = ROOT / "build/mac/report.json"
BASELINE = ROOT / "config/mac/match_baseline.tsv"


class MacBuildError(ValueError):
    pass


@dataclass(frozen=True)
class CompiledCode:
    hunk: CodeHunk | None
    source_hash: str
    build_hash: str
    object_hash: str
    data_hunks: tuple[DataHunk, ...] = ()


@dataclass(frozen=True)
class Result:
    retail_va: str
    unit: str
    signature: str
    mac_section: int
    mac_offset: str
    size: int
    candidate_size: int
    matching_bytes: int
    score: float
    exact: bool
    source_hash: str
    target_sha256: str
    object_sha256: str
    build_hash: str
    resolved_calls: tuple[ResolvedCall, ...]
    removed_reload_slots: tuple[int, ...]
    first_difference: str | None
    resolved_data: tuple[ResolvedData, ...] = ()
    calls: dict | None = None
    restored_reload_slots: tuple[int, ...] = ()
    compile_scope: str = "isolated_function_with_layout_shim"
    analysis_sha256: str | None = None
    executable_sha256: str = inputs.MAC.sha256
    comparison_scope: str = "function_code_and_reviewed_jump_tables; exception metadata and external initializers are not scored"
    jump_tables: tuple[ResolvedJumpTable, ...] = ()
    code_matching_bytes: int = 0
    code_score: float = 0.0
    code_exact: bool = False
    compared_bytes: int = 0


def _digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _profile_hash(source: bytes, pair: Pair) -> str:
    spec = toolchain.specification()
    profile = profiles.load(ROOT, pair.compile_group) if pair.compile_group else None
    header_inputs = profiles.headers(ROOT, profile) if profile else {}
    flags = tuple(profile.flags or spec["flags"] if profile else spec["flags"])
    if profile and profile.native_headers:
        flags += ("-msext", "on", "-DHOMM3_TARGET_MAC=1")
    data = json.dumps({"source": _digest(source), "flags": flags,
                       "profile": asdict(profile) if profile else None,
                       "headers": {name: _digest(data) for name, data in header_inputs.items()},
                       "tools": spec["files"], "collapse_reloads": spec["collapse_reloads"],
                       "wine": _wine_version(), "pair": pair.compile_group or {
                           "retail_va": pair.retail_va,
                           "mac_section": pair.mac_section,
                           "mac_offset": pair.mac_offset,
                           "mac_size": pair.mac_size,
                           "mac_symbol": pair.mac_symbol,
                       }}, sort_keys=True).encode()
    return _digest(data)


def _wine_version() -> str:
    completed = subprocess.run(["wine", "--version"], capture_output=True, text=True)
    if completed.returncode or not completed.stdout.strip():
        raise MacBuildError("cannot identify Wine version for Mac CodeWarrior")
    return completed.stdout.strip()


def _run(command: list[str], cwd: Path, env: dict[str, str]) -> str:
    try:
        completed = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                                   text=True, errors="replace", timeout=180)
    except subprocess.TimeoutExpired as exc:
        raise MacBuildError(f"Mac compiler/linker timed out in {cwd}") from exc
    if completed.returncode:
        raise MacBuildError(f"{' '.join(command)} failed ({completed.returncode}):\n"
                            f"{completed.stdout}{completed.stderr}")
    return completed.stdout + completed.stderr


def compile_pair(pair: Pair, tools_dir: Path) -> CompiledCode:
    profile = profiles.load(ROOT, pair.compile_group) if pair.compile_group else None
    if profile and profile.native_headers:
        from homm3.mac import sdk
        sdk.stage(root=ROOT)
    work = object_directory(ROOT, pair)
    work.mkdir(parents=True, exist_ok=True)
    # Multiple selectors of a shared group reuse one object. Concurrent build
    # and inspection commands must never observe a half-written listing.
    with (work / "compile.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        return _compile_locked(pair, tools_dir, work)


def object_directory(root: Path, pair: Pair) -> Path:
    name = ("probe-" + f"{pair.retail_va:08x}" if not pair.mac_symbol else
            "unit-" + pair.compile_group if pair.compile_group else f"{pair.retail_va:08x}")
    return root / "build/mac/objects" / name


def staged_headers_current(work: Path, inputs: dict[str, bytes]) -> bool:
    folder = work / "_inputs"
    actual = {path.relative_to(folder).as_posix(): path.read_bytes()
              for path in folder.rglob("*") if path.is_file()} if folder.exists() else {}
    return actual == inputs


def _compile_locked(pair: Pair, tools_dir: Path, work: Path) -> CompiledCode:
    source = candidate_source(pair).encode()
    own_source = source_identity(pair, source)
    profile = profiles.load(ROOT, pair.compile_group) if pair.compile_group else None
    header_inputs = profiles.headers(ROOT, profile) if profile else {}
    fingerprint = _profile_hash(source, pair)
    generated = work / "candidate.cpp"
    obj = work / "candidate.o"
    disassembly = work / "candidate.dis.txt"
    stamp = work / "build-stamp.json"
    try:
        saved = json.loads(stamp.read_text())
        object_bytes = obj.read_bytes()
        listing_bytes = disassembly.read_bytes()
        cached = (saved["fingerprint"] == fingerprint
                  and generated.read_bytes() == source
                  and object_bytes.startswith(b"MWOBPPC ")
                  and saved["object_sha256"] == _digest(object_bytes)
                  and saved["listing_sha256"] == _digest(listing_bytes)
                  and staged_headers_current(work, header_inputs))
    except (OSError, ValueError, KeyError, TypeError):
        cached = False
    if not cached:
        stamp.unlink(missing_ok=True)
        generated.write_bytes(source)
        obj.unlink(missing_ok=True)
        disassembly.unlink(missing_ok=True)
        header_folder = work / "_inputs"
        if header_folder.exists():
            shutil.rmtree(header_folder)
        for name, data in header_inputs.items():
            path = header_folder / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        env = dict(os.environ)
        version = re.sub(r"[^A-Za-z0-9._-]", "_", _wine_version())
        env["WINEPREFIX"] = os.environ.get(
            "HOMM3_MAC_WINEPREFIX", str(ROOT / "build/mac/wineprefix" / version))
        env["MWCIncludes"] = ";".join([".", "_inputs", *(
            "_inputs/" + directory for directory in profiles.include_dirs(ROOT, profile))]) if profile else "."
        env.setdefault("WINEDEBUG", "-all")
        Path(env["WINEPREFIX"]).mkdir(parents=True, exist_ok=True)
        flags = profile.flags or toolchain.specification()["flags"] if profile else toolchain.specification()["flags"]
        if profile and profile.native_headers:
            flags = (*flags, "-msext", "on", "-DHOMM3_TARGET_MAC=1")
        _run(["wine", str(tools_dir / "MWCPPC.exe"), *flags,
              "-o", obj.name, generated.name], work, env)
        if not obj.is_file() or not obj.read_bytes().startswith(b"MWOBPPC "):
            raise MacBuildError(f"CodeWarrior did not emit an MWOBPPC object for {pair.retail_va:#x}")
        output = _run(["wine", str(tools_dir / "MWLinkPPC.exe"), "-dis", obj.name], work, env)
        disassembly.write_text(output)
        stamp.write_text(json.dumps({"fingerprint": fingerprint,
                                     "object_sha256": _digest(obj.read_bytes()),
                                     "listing_sha256": _digest(disassembly.read_bytes())},
                                    sort_keys=True) + "\n")
    object_bytes = obj.read_bytes()
    listing = disassembly.read_text()
    hunk = select_hunk(listing, pair.mac_symbol) if pair.mac_symbol else None
    if candidate_source(pair).encode() != source or _profile_hash(source, pair) != fingerprint:
        raise MacBuildError(f"source/shim changed during Mac compilation of {pair.signature}")
    peers = [p for p in load_pairs(ROOT) if p.compile_group == pair.compile_group] if profile else [pair]
    hunks = parse_code_hunks(listing)
    data_hunks = tuple(parse_data_hunks(listing))
    metadata = parse_metadata_hunks(listing)
    emitted = {h.name for h in hunks}
    (work / "compilation.json").write_text(json.dumps({
        "scope": compile_scope(pair), "unit": pair.unit, "build_hash": fingerprint,
        "paired_bodies": [f"0x{p.retail_va:08x}" for p in peers],
        "additional_source_helpers": [f"0x{va:08x}" for va in profile.helpers] if profile else [],
        "helpers_without_windows_va": list(profile.source_helpers) if profile else [],
        "native_headers": bool(profile and profile.native_headers),
        "emitted_symbols": sorted(emitted),
        "emitted_hunks": [{"symbol": h.name, "size": len(h.data), "references": h.xrefs} for h in hunks],
        "metadata_hunks": [asdict(h) for h in metadata],
        "comparison_scope": "function_code_only; exception metadata is not compared",
        "unpaired_probe": f"0x{pair.retail_va:08x}" if hunk is None else None,
        "missing_paired_symbols": [p.mac_symbol for p in peers if p.mac_symbol not in emitted],
        "headers": sorted(header_inputs)}, indent=2) + "\n")
    return CompiledCode(hunk, _digest(own_source), fingerprint, _digest(object_bytes),
                        data_hunks)


def linked_pair(pair: Pair, pef: PEF, tools_dir: Path) -> tuple[LinkedCode, CompiledCode]:
    compiled = compile_pair(pair, tools_dir)
    if compiled.hunk is None:
        raise MacBuildError("compile-only probe has no admitted Mac target or selected code hunk")
    linked = link_code(compiled.hunk, Address(pair.mac_section, pair.mac_offset),
                       symbols.targets(ROOT, pef),
                       collapse_reloads=toolchain.specification()["collapse_reloads"],
                       toc=toc.bindings(ROOT, pef, compiled.hunk, compiled.data_hunks,
                                        unit=pair.unit, retail_va=pair.retail_va,
                                        target_origin=Address(pair.mac_section, pair.mac_offset),
                                        target_size=pair.mac_size))
    return linked, compiled


def compare_pair(pair: Pair, pef: PEF, tools_dir: Path) -> Result:
    analysis = call_report.analysis_hash(ROOT)
    target = pef.code(pair.mac_section, pair.mac_offset, pair.mac_size)
    linked, compiled = linked_pair(pair, pef, tools_dir)
    base = linked.data
    common_bytes = min(len(base), len(target))
    equal = sum(a == b for a, b in zip(base[:common_bytes], target[:common_bytes]))
    first = next((index for index, (a, b) in enumerate(zip(base, target)) if a != b), None)
    if first is None and len(base) != len(target):
        first = common_bytes
    first_difference = f"+0x{first:x}" if first is not None else None
    for table in linked.jump_tables:
        if not table.exact and first_difference is None:
            entry = next(i for i, (a, b) in enumerate(zip(table.candidate_entries, table.target_entries)) if a != b)
            first_difference = f"jump_table {table.section}+0x{table.target_offset:x} entry {entry}"
    code_bytes = max(len(base), len(target))
    compared_bytes = code_bytes + sum(table.size for table in linked.jump_tables)
    total_equal = equal + sum(table.matching_bytes for table in linked.jump_tables)
    origin = Address(pair.mac_section, pair.mac_offset)
    destinations = symbols.targets(ROOT, pef)
    labels = call_report.labels(ROOT)
    call_comparison = calls.compare(calls.analyze(target, origin, destinations, labels=labels),
                                    calls.analyze(base, origin, destinations, labels=labels))
    if call_report.analysis_hash(ROOT) != analysis:
        raise MacBuildError("Mac pairing/tooling changed during comparison; rebuild this unit")
    return Result(f"0x{pair.retail_va:08x}", pair.unit, pair.signature, pair.mac_section,
                  f"0x{pair.mac_offset:x}", pair.mac_size, len(base), total_equal,
                  100.0 * total_equal / compared_bytes,
                  base == target and all(table.exact for table in linked.jump_tables), compiled.source_hash, _digest(target),
                  compiled.object_hash, compiled.build_hash, linked.calls,
                  linked.removed_reload_slots,
                  first_difference, linked.data_references,
                  call_comparison, linked.restored_reload_slots, compile_scope(pair), analysis,
                  jump_tables=linked.jump_tables, code_matching_bytes=equal,
                  code_score=100.0 * equal / code_bytes, code_exact=base == target,
                  compared_bytes=compared_bytes)


def _previous() -> dict[str, tuple[float, float, str]]:
    if not BASELINE.is_file():
        return {}
    rows = {}
    for line in BASELINE.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        va, _, _, _, _, maximum, historical, fingerprint = line.split("\t")
        rows[va] = float(maximum), float(historical), fingerprint
    return rows


def _checkpoint(results: list[Result]) -> None:
    previous = _previous()
    lines = ["# GENERATED by full `homm3 build`. Do not hand-edit.",
             "# retail_va\tunit\tmac_section\tmac_offset\tcur\tmax\thist\tsource_hash"]
    for row in sorted(results, key=lambda item: item.retail_va):
        old_max, old_hist, old_hash = previous.get(row.retail_va, (0.0, 0.0, ""))
        maximum = max(old_max, row.score) if old_hash == row.source_hash else row.score
        historical = max(old_hist, maximum)
        lines.append(f"{row.retail_va}\t{row.unit}\t{row.mac_section}\t{row.mac_offset}\t"
                     f"{row.score:.4f}\t{maximum:.4f}\t{historical:.4f}\t{row.source_hash}")
    BASELINE.write_text("\n".join(lines) + "\n")


def write_readme(results: list[Result]) -> None:
    path = ROOT / "README.md"
    current = path.read_text()
    begin = "<!-- mac-match-score:start -->"
    end = "<!-- mac-match-score:end -->"
    if current.count(begin) != 1 or current.count(end) != 1:
        raise MacBuildError("README lacks one Mac score block")
    exact = sum(row.exact for row in results)
    scored = sum(row.matching_bytes for row in results)
    total = sum(row.compared_bytes or max(row.size, row.candidate_size) for row in results)
    replacement = (f"{begin}\n\n**Classic Mac PowerPC second target (last full checkpoint):** "
                   f"{exact} / {len(results)} admitted functions exact; "
                   f"{100 * scored / total if total else 0:.2f}% of {total:,} compared bytes match. "
                   "The admitted-pair count is coverage, not the whole Mac game.\n\n"
                   f"{end}")
    first = current.index(begin)
    last = current.index(end, first) + len(end)
    updated = current[:first] + replacement + current[last:]
    if updated != current:
        path.write_text(updated)


def run(units: set[str] | None = None, *, checkpoint: bool = False) -> list[Result]:
    pairs = [pair for pair in load_pairs(ROOT) if units is None or pair.unit in units]
    if not pairs and units is not None:
        print("[mac] 0/0 admitted functions in selected units", flush=True)
        return []
    executable = inputs.stage_executable(inputs.MAC)
    tools_dir = toolchain.stage()
    pef = PEF(inputs.read_verified(inputs.MAC, executable))
    results = []
    call_rows, errors = [], []
    for pair in pairs:
        observation = call_report.inspect(ROOT, pair, pef, tools_dir)
        call_rows.append(observation)
        try:
            if observation["calls"]["candidate"] is None:
                raise MacBuildError(observation["calls"]["error"])
            result = compare_pair(pair, pef, tools_dir)
        except (ValueError, OSError) as exc:
            message = f"{pair.unit} 0x{pair.retail_va:08x}: {exc}"
            errors.append(message)
            observation["comparison_error"] = str(exc)
            print(f"[mac] ERROR: {message}", flush=True)
            print(f"[mac]   {calls.summary(observation['calls'])}", flush=True)
            continue
        results.append(result)
        call_rows[-1] = asdict(result)
        print(f"[mac] {pair.unit} {result.retail_va} section {pair.mac_section}+{pair.mac_offset:#x}: "
              f"{result.score:.4f}% {'EXACT' if result.exact else 'first difference ' + str(result.first_difference)}",
              flush=True)
        print(f"[mac]   {calls.summary(result.calls)}", flush=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    reports.publish(REPORT, {"target_sha256": inputs.MAC.sha256,
                             "analysis_sha256": call_report.analysis_hash(ROOT),
                             "errors": errors, "pairs": [asdict(row) for row in results]},
                    units=sorted(units) if units is not None else None)
    call_report.write(ROOT, call_rows,
                      units=sorted(units) if units is not None else None)
    if errors:
        raise MacBuildError(f"{len(errors)}/{len(pairs)} Mac pairs unavailable; reports retain all call observations")
    if checkpoint:
        if units is not None:
            raise MacBuildError("cannot checkpoint a partial Mac unit selection")
        from homm3.mac.queue import observation_problem
        current = {pair.retail_va: pair for pair in load_pairs(ROOT)}
        if set(current) != {int(row.retail_va, 0) for row in results}:
            raise MacBuildError("Mac pair inventory changed during full build; checkpoint withheld")
        for row in results:
            problem = observation_problem(ROOT, current[int(row.retail_va, 0)], asdict(row), {})
            if problem:
                raise MacBuildError(f"{row.retail_va}: {problem}; checkpoint withheld")
        _checkpoint(results)
        write_readme(results)
    print(f"[mac] {sum(row.exact for row in results)}/{len(results)} admitted functions exact", flush=True)
    return results
