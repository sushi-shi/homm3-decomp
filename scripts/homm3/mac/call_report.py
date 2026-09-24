"""Fresh Mac call reports, including candidates that cannot yet be linked."""
from __future__ import annotations

import hashlib
from dataclasses import dataclass
from pathlib import Path

from homm3.core import inputs
from homm3.mac import calls, references, reports, symbols
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, CallTarget
from homm3.mac.source import Pair, load_pairs
from homm3.mac.source import candidate_source, compile_scope, source_identity


def analysis_hash(root: Path) -> str:
    paths = sorted((root / "config/mac").rglob("*.toml"))
    paths += sorted(path for path in (root / "scripts/homm3/mac").glob("*.py")
                    if not path.name.startswith("test_"))
    digest = hashlib.sha256()
    for path in paths:
        digest.update(str(path.relative_to(root)).encode() + b"\0" + path.read_bytes() + b"\0")
    return digest.hexdigest()


def labels(root: Path) -> dict[str, str]:
    return {pair.mac_symbol: pair.signature for pair in [*references.load(root), *load_pairs(root)]
            if pair.mac_symbol is not None}


@dataclass(frozen=True)
class InspectionContext:
    analysis: str
    addresses: dict[str, CallTarget]
    names: dict[str, str]


def inspection_context(root: Path, pef: PEF) -> InspectionContext:
    return InspectionContext(analysis_hash(root), symbols.targets(root, pef), labels(root))


def inspect(root: Path, pair: Pair, pef: PEF, tools_dir: Path, *,
            context: InspectionContext | None = None) -> dict:
    from homm3.mac import build
    context = context or inspection_context(root, pef)
    origin = Address(pair.mac_section, pair.mac_offset)
    target = pef.code(pair.mac_section, pair.mac_offset, pair.mac_size)
    retail = calls.analyze(target, origin, context.addresses, labels=context.names)
    source_hash = build_hash = object_hash = None
    error = None
    candidate = None
    try:
        source = candidate_source(pair).encode()
        source_hash = hashlib.sha256(source_identity(pair, source)).hexdigest()
        build_hash = build._profile_hash(source, pair)
        compiled = build.compile_pair(pair, tools_dir)
        source_hash, build_hash, object_hash = compiled.source_hash, compiled.build_hash, compiled.object_hash
        candidate = calls.analyze(compiled.hunk.data, origin, context.addresses,
                                  xrefs=compiled.hunk.xrefs, labels=context.names)
    except (OSError, ValueError) as exc:
        error = str(exc)
    return {"retail_va": f"0x{pair.retail_va:08x}", "unit": pair.unit,
            "analysis_sha256": context.analysis, "executable_sha256": inputs.MAC.sha256,
            "signature": pair.signature, "mac_section": pair.mac_section,
            "compile_scope": compile_scope(pair),
            "mac_offset": f"0x{pair.mac_offset:x}", "size": pair.mac_size,
            "source_hash": source_hash, "build_hash": build_hash, "object_sha256": object_hash,
            "target_sha256": hashlib.sha256(target).hexdigest(),
            "calls": calls.compare(retail, candidate, error=error)}


def write(root: Path, rows: list[dict], *, units: list[str] | None = None) -> dict:
    report = {"schema": 2, "scope": "recorded_admitted_function_observations", "refreshed_units": units,
              "target_sha256": inputs.MAC.sha256,
              "analysis_sha256": analysis_hash(root),
              "configured_pairs": len(load_pairs(root)), "pairs": list(rows),
              "freshness": "Queue verifies current inputs for every observation; other units retain their original provenance."}
    out = root / "build/mac"
    def finish(merged):
        merged["reported_pairs"] = len(merged["pairs"])
        merged["totals"] = calls.totals([row["calls"] for row in merged["pairs"]])
        reports.atomic_text(out / "calls.tsv", _tsv(merged["pairs"]))
    return reports.publish(out / "calls.json", report, units=units, finish=finish)


def _tsv(rows):
    header = ["windows_va", "unit", "function", "retail_calls", "candidate_calls",
              "delta", "retail_direct", "candidate_direct", "retail_indirect",
              "candidate_indirect", "state"]
    lines = ["# Generated call-site census; admitted pairs only. Blank candidate fields mean unavailable.",
             "\t".join(header)]
    for row in rows:
        comparison = row["calls"]
        retail, candidate = comparison["retail"], comparison["candidate"]
        values = [row["retail_va"], row["unit"], row["signature"], retail["total"],
                  candidate["total"] if candidate else "", comparison["delta"] if candidate else "",
                  retail["direct"], candidate["direct"] if candidate else "",
                  retail["indirect"], candidate["indirect"] if candidate else "", comparison["state"]]
        lines.append("\t".join(str(value).replace("\t", " ").replace("\n", " ") for value in values))
    return "\n".join(lines) + "\n"
