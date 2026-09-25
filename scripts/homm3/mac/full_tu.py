"""Compare admitted Mac pairs using hunks from complete CodeWarrior TU objects.

This is the migration control for the full-TU build: every exact pair of the
selected-body path must still compare exactly when its candidate body is
taken from `build/mac/obj/<unit>.o` (built by `ninja mac:<unit>`). The same
relocation, TOC and jump-table linking is used; unresolved references are never
masked into an exact verdict. Scores here are observational and do not touch
the Mac CUR/MAX/HIST ledger.
"""
from __future__ import annotations

from pathlib import Path

from homm3.core import common
from homm3.mac import symbols, toc, toolchain
from homm3.mac.object import parse_code_hunks, parse_data_hunks
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, link_code
from homm3.mac.source import load_pairs

ROOT = common.HOMM3_DIR


def _baseline(root: Path) -> dict[str, float]:
    path = root / "config/mac/match_baseline.tsv"
    rows = {}
    if path.is_file():
        for line in path.read_text().splitlines():
            if line and not line.startswith("#"):
                fields = line.split("\t")
                rows[fields[0]] = float(fields[4])
    return rows


def compare(unit: str, pef: PEF, root: Path = ROOT) -> list[dict]:
    listing_path = root / "build/mac/obj" / f"{unit}.dis.txt"
    if not listing_path.is_file():
        raise ValueError(f"{unit}: no full-TU listing; run `ninja mac:{unit}`")
    listing = listing_path.read_text()
    hunks = {hunk.name: hunk for hunk in parse_code_hunks(listing)}
    data_hunks = tuple(parse_data_hunks(listing))
    targets = symbols.targets(root, pef)
    collapse = toolchain.specification()["collapse_reloads"]
    legacy = _baseline(root)
    rows = []
    for pair in load_pairs(root):
        if pair.unit != unit:
            continue
        va = f"0x{pair.retail_va:08x}"
        row = dict(retail_va=va, symbol=pair.mac_symbol, legacy_cur=legacy.get(va),
                   score=None, exact=False, first_difference=None, error=None)
        hunk = hunks.get(pair.mac_symbol)
        if hunk is None:
            row["error"] = "symbol not emitted by the full TU"
            rows.append(row)
            continue
        origin = Address(pair.mac_section, pair.mac_offset)
        try:
            linked = link_code(hunk, origin, targets, collapse_reloads=collapse,
                               toc=toc.bindings(root, pef, hunk, data_hunks, unit=unit,
                                                retail_va=pair.retail_va, target_origin=origin,
                                                target_size=pair.mac_size))
        except ValueError as exc:
            row["error"] = str(exc)
            rows.append(row)
            continue
        target = pef.code(pair.mac_section, pair.mac_offset, pair.mac_size)
        base = linked.data
        equal = sum(a == b for a, b in zip(base, target))
        first = next((index for index, (a, b) in enumerate(zip(base, target)) if a != b), None)
        if first is None and len(base) != len(target):
            first = min(len(base), len(target))
        compared = max(len(base), len(target)) + sum(table.size for table in linked.jump_tables)
        equal += sum(table.matching_bytes for table in linked.jump_tables)
        row.update(score=100.0 * equal / compared,
                   exact=base == target and all(table.exact for table in linked.jump_tables),
                   first_difference=None if first is None else f"+0x{first:x}")
        rows.append(row)
    return rows
