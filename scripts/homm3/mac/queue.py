"""Join the whole Windows game inventory with Mac coverage and call evidence."""
from __future__ import annotations

from collections import Counter
import csv
import hashlib
import io
import json
from pathlib import Path
import tomllib

from homm3 import manifest
from homm3.core import common, inputs
from homm3.mac import build, call_report, references, reports
from homm3.mac.source import candidate_source, compile_scope, load_pairs, source_identity
from homm3.mac import profiles
from homm3.match import status, universe


def _digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def observation_problem(root: Path, pair, row: dict | None, report: dict) -> str | None:
    if row is None:
        return "no call report for this pair"
    if (row.get("executable_sha256", report.get("target_sha256")) != inputs.MAC.sha256
            or row.get("analysis_sha256", report.get("analysis_sha256")) != call_report.analysis_hash(root)):
        return "target, pair inventory or analysis tooling changed"
    if (row.get("mac_section") != pair.mac_section or row.get("mac_offset") != f"0x{pair.mac_offset:x}"
            or row.get("size") != pair.mac_size or row.get("signature") != pair.signature):
        return "pair identity or extent changed"
    try:
        source = candidate_source(pair).encode()
        if row.get("source_hash") != _digest(source_identity(pair, source)):
            return "source, data definition or layout shim changed"
        fingerprint = build._profile_hash(source, pair)
        if row.get("build_hash") != fingerprint:
            return "compiler profile changed"
        if row.get("calls", {}).get("state") == "candidate_unavailable":
            return None  # A current failed attempt has no valid object to hash.
        work = build.object_directory(root, pair)
        profile = profiles.load(root, pair.compile_group) if pair.compile_group else None
        if profile and not build.staged_headers_current(work, profiles.headers(root, profile)):
            return "staged Mac header inputs changed"
        stamp = json.loads((work / "build-stamp.json").read_text())
        obj = (work / "candidate.o").read_bytes()
        listing = (work / "candidate.dis.txt").read_bytes()
        if (stamp["fingerprint"] != fingerprint or stamp["object_sha256"] != _digest(obj)
                or row.get("object_sha256") != _digest(obj)
                or stamp["listing_sha256"] != _digest(listing)
                or (work / "candidate.cpp").read_bytes() != source):
            return "compiled object or disassembly changed"
    except (OSError, ValueError, KeyError) as exc:
        return f"cannot verify observation: {exc}"
    return None


def route(*, va: str, unit: str, paired: bool, deferred: bool, windows_stale: bool,
          problem: str | None, comparison: dict | None, mac_max: float | None,
          comparison_error: str | None = None) -> dict:
    """Explicit next actions; unavailable evidence never means zero differences."""
    state, action, command, ready = "", "", "", False
    if deferred:
        state, action = "deferred", "User deferred this module until a later round."
    elif not paired:
        state, action = "pairing_needed", "Locate and verify the Mac counterpart and its boundaries."
        command = f"homm3 sema disasm {va}"
    elif windows_stale:
        state, action = "windows_checkpoint_needed", "Refresh changed Windows source before assigning matching work."
        command = "homm3 build"
    elif problem:
        state, action = "call_report_needed", problem + "; regenerate the comparison."
        command = f"homm3 mac calls {va}"
    elif comparison is not None and comparison.get("candidate") is None:
        state, action = "compilation_needed", "Fix the reported CodeWarrior compilation or hunk extraction failure."
        command = f"homm3 mac calls {va}"
    elif comparison_error:
        state, action = "comparison_setup_needed", "Resolve the byte-comparison prerequisite: " + comparison_error
        command = f"homm3 mac diff {va}"
    else:
        call_state = comparison["state"]
        if call_state == "unresolved_call_targets":
            state, action = "reference_resolution_needed", "Pair the named unresolved callees before exact comparison."
            command = f"homm3 mac calls {va}"
        elif call_state in ("call_count_difference", "call_target_difference"):
            state, action = "call_difference", "Inspect added, missing or reordered named calls; recover helper boundaries and visibility."
            command, ready = f"homm3 mac calls {va}", True
        elif comparison.get("external_branches_differ"):
            state, action = "external_branch_difference", "Review outgoing branches and possible tail-call targets."
            command, ready = f"homm3 mac disasm {va}", True
        elif call_state == "indirect_targets_unknown":
            state, action = "indirect_call_review", "Resolve function-pointer or virtual-call targets; matching counts do not prove them."
            command, ready = f"homm3 mac disasm {va}", True
        elif mac_max is None:
            state, action = "byte_report_needed", "Compile, resolve references and establish a Mac byte comparison."
            command = f"homm3 build --fast {unit}"
        elif mac_max < 100 - 1e-6:
            state, action = "mac_instruction_difference", "Inspect remaining Mac instructions with the agreeing direct-call sequence."
            command, ready = f"homm3 mac diff {va}", True
        else:
            state, action = "windows_instruction_difference", "Use the exact Mac source observation while diagnosing remaining VC6 differences."
            command, ready = f"homm3 sema diff {va} --summary", True
    return {"state": state, "action": action, "command": command, "dispatchable": ready}


def generate(root: Path, *, source_hashes: dict | None = None) -> dict:
    categories, sizes = universe.classify()
    baseline = status.load_baseline(root / "config/match_baseline.tsv")
    hashes = status.source_hashes(source_root=root) if source_hashes is None else source_hashes
    units = manifest.by_unit(root / "config/units.toml")
    with (root / "config/mac/campaign.toml").open("rb") as stream:
        campaign = tomllib.load(stream)
    deferred = set(campaign["deferred_modules"])
    pairs = {pair.retail_va: pair for pair in load_pairs(root)}
    callee_refs = {ref.retail_va: ref for ref in references.load(root)
                  if ref.retail_va is not None and ref.retail_va not in pairs}
    call_path = root / "build/mac/calls.json"
    report = json.loads(call_path.read_text()) if call_path.is_file() else {}
    observations = {int(row["retail_va"], 0): row for row in report.get("pairs", [])}
    byte_path = root / "build/mac/report.json"
    byte_report = json.loads(byte_path.read_text()) if byte_path.is_file() else {}
    current_analysis = call_report.analysis_hash(root)
    byte_rows = {int(row["retail_va"], 0): row for row in byte_report.get("pairs", [])
                 if row.get("analysis_sha256", byte_report.get("analysis_sha256")) == current_analysis
                 and row.get("executable_sha256", byte_report.get("target_sha256")) == inputs.MAC.sha256}
    mac_baseline = {}
    path = root / "config/mac/match_baseline.tsv"
    if path.is_file():
        for line in path.read_text().splitlines():
            if line and not line.startswith("#"):
                va, _, _, _, cur, maximum, historical, fingerprint = line.split("\t")
                mac_baseline[int(va, 0)] = (float(cur), float(maximum), float(historical), fingerprint)

    identities = {}
    names_path = root / "build/gen/symbol_names.csv"
    if names_path.is_file():
        reader = csv.DictReader(io.StringIO("\n".join(
            line for line in names_path.read_text().splitlines() if not line.startswith("#"))))
        for row in reader:
            if row["kind"] == "func":
                identities.setdefault(int(row["rva"], 0), (row["unit"], row["name"]))
    by_rva = {}
    for key, score in baseline.items():
        if score.rva is not None:
            by_rva.setdefault(score.rva, []).append((key, score))
    rows = []
    windows_exact = paired_targets = 0
    disposition = Counter()
    for rva, size in sorted(sizes.items()):
        if categories.get(rva) not in ("target", "zlib"):
            continue
        va = common.IMAGE_BASE + rva
        claimed = by_rva.get(rva, [])
        own_changes = any(score.src_hash and hashes.get(key) and score.src_hash != hashes[key]
                          for key, score in claimed)
        valid = [(key, score) for key, score in claimed
                 if not (score.src_hash and hashes.get(key) and score.src_hash != hashes[key])]
        if valid:
            (unit, name), score = max(valid, key=lambda item: item[1].max)
            win_cur, win_max, win_hist = score.cur, score.max, score.hist
        else:
            unit, name = identities.get(rva, ("", ""))
            win_cur = win_max = None
            win_hist = max((score.hist for _, score in claimed), default=None)
        unit_info = units.get(unit, {})
        module = unit_info.get("module", status.module_of(unit_info.get("source", "")))
        if categories[rva] == "zlib":
            module = "zlib-1.1.3"
        pair = pairs.get(va)
        paired_targets += pair is not None
        is_win_exact = win_max is not None and win_max >= 100 - 1e-6 and not own_changes
        windows_exact += is_win_exact
        mac_cur = mac_max = mac_hist = None
        if pair and va in mac_baseline:
            current_source = _digest(source_identity(pair, candidate_source(pair).encode()))
            cur, maximum, historical, fingerprint = mac_baseline[va]
            mac_hist = historical
            if fingerprint == current_source:
                mac_cur, mac_max = cur, maximum
        observation = observations.get(va)
        problem = observation_problem(root, pair, observation, report) if pair else "no Mac pair"
        byte_row = byte_rows.get(va)
        if (not problem and byte_row and observation
                and byte_row.get("source_hash") == observation.get("source_hash")
                and byte_row.get("object_sha256") == observation.get("object_sha256")
                and byte_row.get("build_hash") == observation.get("build_hash")):
            mac_cur = byte_row["score"]
            mac_max = max(mac_max or 0, mac_cur)
            mac_hist = max(mac_hist or 0, mac_max)
        if is_win_exact and (pair is None or mac_max is not None and mac_max >= 100 - 1e-6):
            disposition["windows_exact_unpaired" if pair is None else "both_banked_exact"] += 1
            continue
        comparison = observation.get("calls") if observation else None
        routing = route(va=f"0x{va:08x}", unit=unit, paired=pair is not None,
                        deferred=module in deferred, windows_stale=own_changes,
                        problem=problem, comparison=comparison, mac_max=mac_max,
                        comparison_error=observation.get("comparison_error") if observation else None)
        if va in callee_refs and routing["state"] == "pairing_needed":
            routing.update(state="compilation_needed",
                           action="Callee identity is reviewed; prepare its Mac declaration view and compile the shared body before admitting a byte target.",
                           command=f"homm3 mac compile 0x{va:08x} --unit {callee_refs[va].unit}")
        # Unresolved calls can coexist with unequal counts: keep their setup
        # requirement visible and do not dispatch an incomplete byte target.
        if comparison and comparison["candidate"] and comparison["candidate"]["unresolved_direct"]:
            routing["dispatchable"] = False
        disposition[routing["state"]] += 1
        fresh = comparison if not problem else None
        rows.append({"retail_va": f"0x{va:08x}", "size": size, "unit": unit, "module": module,
                     "source": str(pair.source.relative_to(root)) if pair else unit_info.get("source"),
                     "function": pair.signature if pair else name,
                     "windows_cur": win_cur, "windows_max": win_max, "windows_hist": win_hist,
                     "windows_source_changed": own_changes,
                     "mac_paired": pair is not None, "mac_cur": mac_cur, "mac_max": mac_max,
                     "mac_callee_reference_only": va in callee_refs,
                     "mac_hist": mac_hist, "compile_scope": compile_scope(pair) if pair else None,
                     "observation_problem": problem,
                     "retail_calls": fresh["retail"]["total"] if fresh else None,
                     "candidate_calls": fresh["candidate"]["total"] if fresh and fresh["candidate"] else None,
                     "call_delta": fresh["delta"] if fresh else None,
                     "call_differences": fresh["differences"] if fresh else None,
                     "evidence_commands": [f"homm3 dreamcast show 0x{va:08x}",
                                           f"homm3 sema diff 0x{va:08x} --summary"],
                     "verification_commands": [f"homm3 build --fast {unit}"] if unit else [],
                     **routing})
    priority = {"call_difference": 0, "external_branch_difference": 1, "indirect_call_review": 2,
                "mac_instruction_difference": 3, "windows_instruction_difference": 4,
                "reference_resolution_needed": 5, "byte_report_needed": 6,
                "comparison_setup_needed": 5,
                "call_report_needed": 7, "compilation_needed": 8,
                "windows_checkpoint_needed": 9, "pairing_needed": 10, "deferred": 11}
    rows.sort(key=lambda row: (priority[row["state"]], row["size"], row["retail_va"]))
    total = sum(category in ("target", "zlib") for category in categories.values())
    return {"schema": 1, "scope": "unfinished_windows_game_and_admitted_mac_targets",
            "target_sha256": inputs.MAC.sha256, "analysis_sha256": call_report.analysis_hash(root),
            "workers_after_tooling": campaign["workers_after_tooling"],
            "coverage": {"windows_targets": total, "windows_banked_exact": windows_exact,
                         "mac_paired_targets": paired_targets, "mac_unpaired_targets": total - paired_targets,
                         "mac_callee_only_references": len(callee_refs),
                         "queued": len(rows), "dispatchable": sum(row["dispatchable"] for row in rows),
                         "dispositions": dict(disposition)}, "rows": rows}


def write(root: Path, report: dict) -> None:
    out = root / "build/mac"
    out.mkdir(parents=True, exist_ok=True)
    reports.atomic_text(out / "queue.json", json.dumps(report, indent=2) + "\n")
    fields = ("state", "dispatchable", "retail_va", "unit", "function", "windows_max", "mac_max",
              "retail_calls", "candidate_calls", "call_delta", "action", "command", "source")
    lines = ["# Generated action queue. Empty fields mean unknown; scores use current-source MAX.",
             "\t".join(fields)]
    for row in report["rows"]:
        lines.append("\t".join("" if row.get(key) is None else str(row[key]).replace("\t", " ").replace("\n", " ")
                               for key in fields))
    reports.atomic_text(out / "queue.tsv", "\n".join(lines) + "\n")


def refresh(root: Path, *, source_hashes: dict | None = None) -> dict:
    report = generate(root, source_hashes=source_hashes)
    write(root, report)
    coverage = report["coverage"]
    print(f"[mac] action queue: {coverage['queued']} tasks, "
          f"{coverage['dispatchable']} with current comparison evidence -> build/mac/queue.tsv")
    return report
