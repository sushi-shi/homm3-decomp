"""Score Mac pairs from full-TU CodeWarrior objects against the pinned PEF.

Each pair is a source MAC_ADDRESS claim with a Windows VA whose body its unit's
object emits (`homm3.mac.pairs`). The candidate hunk is linked at the claimed
address with the claim/runtime/glue call targets and the reviewed TOC data; a
reference that cannot be resolved leaves the pair unavailable rather than
masked. Scores are observational; the Mac CUR/MAX/HIST ledger is keyed by VA.
"""
from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
import os
from pathlib import Path
import subprocess

from homm3.core import common, inputs
from homm3.mac import call_report, calls, pairs, reports, symbols, toc, toolchain
from homm3.mac.object import CodeHunk, DataHunk, parse_code_hunks, parse_data_hunks
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, LinkedCode, ResolvedCall, ResolvedData, ResolvedJumpTable, link_code


ROOT = common.HOMM3_DIR
REPORT = ROOT / "build/mac/report.json"
BASELINE = ROOT / "config/mac/match_baseline.tsv"


class MacBuildError(ValueError):
    pass


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
    mac_symbol: str
    resolved_calls: tuple[ResolvedCall, ...]
    removed_reload_slots: tuple[int, ...]
    first_difference: str | None
    resolved_data: tuple[ResolvedData, ...] = ()
    calls: dict | None = None
    restored_reload_slots: tuple[int, ...] = ()
    executable_sha256: str = inputs.MAC.sha256
    comparison_scope: str = "full-TU function code and reviewed jump tables; exception metadata is not scored"
    jump_tables: tuple[ResolvedJumpTable, ...] = ()
    code_matching_bytes: int = 0
    code_score: float = 0.0
    code_exact: bool = False
    compared_bytes: int = 0


def _digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class Listings:
    """Parsed `build/mac/obj/<unit>.dis.txt` listings, read once per unit."""

    def __init__(self, root: Path = ROOT):
        self.root = root
        self._units: dict[str, tuple[dict[str, CodeHunk], tuple[DataHunk, ...]]] = {}

    def get(self, unit: str) -> tuple[dict[str, CodeHunk], tuple[DataHunk, ...]]:
        if unit not in self._units:
            path = self.root / "build/mac/obj" / f"{unit}.dis.txt"
            if not path.is_file():
                raise MacBuildError(f"{unit}: no full-TU listing; run `ninja mac:{unit}`")
            listing = path.read_text()
            self._units[unit] = ({hunk.name: hunk for hunk in parse_code_hunks(listing)},
                                 tuple(parse_data_hunks(listing)))
        return self._units[unit]


def objects(units: set[str] | None = None, root: Path = ROOT) -> None:
    """Refresh objects; only diagnosed source compilation errors are nonfatal.

    The wrapper removes failed outputs before allowing Ninja to continue.
    Every other Ninja failure prevents comparison and checkpointing.
    """
    targets = sorted(f"mac:{unit}" for unit in units) if units else ["mac-objects"]
    completed = subprocess.run(["ninja", "-C", str(root), "-k", "0", *targets],
                               capture_output=True, text=True, errors="replace",
                               env=dict(os.environ, HOMM3_MAC_ALLOW_COMPILE_ERRORS="1"))
    if completed.returncode:
        raise MacBuildError("full-TU object refresh failed; comparison withheld:\n"
                            + (completed.stdout + completed.stderr)[-6000:])


def linked_pair(pair: pairs.Pair, pef: PEF, destinations, listings: Listings) -> tuple[LinkedCode, CodeHunk]:
    hunks, data_hunks = listings.get(pair.unit)
    hunk = hunks.get(pair.mac_symbol)
    if hunk is None:
        raise MacBuildError(f"{pair.unit} listing lacks {pair.mac_symbol}")
    origin = Address(pair.mac_section, pair.mac_offset)
    linked = link_code(hunk, origin, destinations,
                       collapse_reloads=toolchain.specification()["collapse_reloads"],
                       toc=toc.bindings(ROOT, pef, hunk, data_hunks, unit=pair.unit,
                                        retail_va=pair.retail_va, target_origin=origin,
                                        target_size=pair.mac_size))
    return linked, hunk


def compare_pair(pair: pairs.Pair, pef: PEF, destinations, labels, listings: Listings) -> Result:
    target = pef.code(pair.mac_section, pair.mac_offset, pair.mac_size)
    linked, hunk = linked_pair(pair, pef, destinations, listings)
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
    call_comparison = calls.compare(calls.analyze(target, origin, destinations, labels=labels),
                                    calls.analyze(base, origin, destinations, labels=labels))
    return Result(f"0x{pair.retail_va:08x}", pair.unit, pair.signature, pair.mac_section,
                  f"0x{pair.mac_offset:x}", pair.mac_size, len(base), total_equal,
                  100.0 * total_equal / compared_bytes,
                  base == target and all(table.exact for table in linked.jump_tables),
                  pair.source_hash, _digest(target), pair.mac_symbol, linked.calls,
                  linked.removed_reload_slots, first_difference, linked.data_references,
                  call_comparison, linked.restored_reload_slots,
                  jump_tables=linked.jump_tables, code_matching_bytes=equal,
                  code_score=100.0 * equal / code_bytes, code_exact=base == target,
                  compared_bytes=compared_bytes)


def _previous() -> dict[str, list[str]]:
    rows = {}
    if BASELINE.is_file():
        for line in BASELINE.read_text().splitlines():
            if line and not line.startswith("#"):
                fields = line.split("\t")
                rows[fields[0]] = fields
    return rows


def _checkpoint(results: list[Result]) -> None:
    """CUR/MAX/HIST by VA. MAX follows the pair's own source hash; a VA not
    scored this time keeps its previous row."""
    previous = _previous()
    rows = {va: fields for va, fields in previous.items()}
    for row in results:
        old = previous.get(row.retail_va)
        old_max, old_hist, old_hash = ((float(old[5]), float(old[6]), old[7]) if old
                                       else (0.0, 0.0, ""))
        maximum = max(old_max, row.score) if old_hash == row.source_hash else row.score
        historical = max(old_hist, maximum)
        rows[row.retail_va] = [row.retail_va, row.unit, str(row.mac_section), row.mac_offset,
                               f"{row.score:.4f}", f"{maximum:.4f}", f"{historical:.4f}",
                               row.source_hash]
    lines = ["# GENERATED by full `homm3 build`. Do not hand-edit.",
             "# retail_va\tunit\tmac_section\tmac_offset\tcur\tmax\thist\tsource_hash"]
    lines += ["\t".join(fields) for _va, fields in sorted(rows.items())]
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
    replacement = (f"{begin}\n\n**Lightly optimized Classic Mac PowerPC reference (last full checkpoint):** "
                   f"{exact} / {len(results)} scored functions exact; "
                   f"{100 * scored / total if total else 0:.2f}% of {total:,} compared bytes match. "
                   "Scored functions are source claims whose full-TU body links; "
                   "this is coverage, not the whole Mac game.\n\n"
                   f"{end}")
    first = current.index(begin)
    last = current.index(end, first) + len(end)
    updated = current[:first] + replacement + current[last:]
    if updated != current:
        path.write_text(updated)


def run(units: set[str] | None = None, *, checkpoint: bool = False) -> list[Result]:
    objects(units)
    with toc.caching():
        return _run(units, checkpoint)


def _run(units: set[str] | None, checkpoint: bool) -> list[Result]:
    inventory = pairs.load(ROOT)
    selected = [pair for pair in inventory.pairs if units is None or pair.unit in units]
    executable = inputs.stage_executable(inputs.MAC)
    toolchain.stage()
    pef = PEF(inputs.read_verified(inputs.MAC, executable))
    destinations = symbols.targets(ROOT, pef, inventory)
    labels = dict(inventory.labels)
    listings = Listings()
    results, rows, unavailable = [], [], []
    for pair in selected:
        try:
            result = compare_pair(pair, pef, destinations, labels, listings)
        except (ValueError, OSError) as exc:
            unavailable.append(dict(retail_va=f"0x{pair.retail_va:08x}", unit=pair.unit,
                                    signature=pair.signature, reason=str(exc)))
            continue
        results.append(result)
        rows.append(asdict(result))
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    reports.publish(REPORT, {"target_sha256": inputs.MAC.sha256,
                             "analysis_sha256": call_report.analysis_hash(ROOT),
                             "unavailable": unavailable,
                             "unscored_claims": list(inventory.unscored),
                             "pairs": rows},
                    units=sorted(units) if units is not None else None)
    call_report.write(ROOT, rows, units=sorted(units) if units is not None else None)
    for row in sorted(results, key=lambda item: item.retail_va):
        if units is not None:
            print(f"[mac] {row.unit} {row.retail_va} {row.signature}: {row.score:.4f}% "
                  f"{'EXACT' if row.exact else 'first difference ' + str(row.first_difference)}",
                  flush=True)
    if checkpoint:
        if units is not None:
            raise MacBuildError("cannot checkpoint a partial Mac unit selection")
        _checkpoint(results)
        write_readme(results)
    print(f"[mac] {sum(row.exact for row in results)}/{len(results)} scored pairs exact; "
          f"{len(unavailable)} emitted pairs unavailable (unresolved references); "
          f"{sum(1 for row in inventory.unscored if units is None or row['unit'] in units)} "
          f"Windows-VA claims without a full-TU body; report {REPORT.relative_to(ROOT)}", flush=True)
    return results
