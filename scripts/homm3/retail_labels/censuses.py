"""homm3.retail_labels.censuses - the admitted retail inventories: structure only.

config/retail-functions.tsv is the function universe (starts + admitted
sizes - unlike gruntz, extents are hand-admitted alongside the starts, not
derived to the next row). config/retail-vtables.tsv contributes vtable
starts and slot counts; its hand-admitted `class` column is IDENTITY the
model consumes (a deliberate divergence from gruntz's structure/identity
split: source VTBL() macros are retired by user decision 2026-08-06, so the
census file is where vtable identity is admitted).

Parse-only: columns, hex spelling, duplicate starts. No joins, no naming.
"""

from __future__ import annotations

from pathlib import Path

from homm3.core import common
from homm3.core.tsv import read as read_tsv

FUNCTIONS = common.HOMM3_DIR / "config/retail-functions.tsv"
VTABLES = common.HOMM3_DIR / "config/retail-vtables.tsv"


def functions(path: Path | None = None) -> list[dict]:
    """[{rva, size}] sorted by rva - the admitted function universe."""
    _b, _h, raw = read_tsv(path or FUNCTIONS)
    rows = [{"rva": int(r["rva"], 16), "size": int(r["size"])} for r in raw]
    rows.sort(key=lambda r: r["rva"])
    for a, b in zip(rows, rows[1:]):
        if a["rva"] == b["rva"]:
            raise ValueError(f"{path or FUNCTIONS}: duplicate function row "
                             f"0x{a['rva']:08x}")
    return rows


def vtables(path: Path | None = None) -> list[dict]:
    """[{rva, count, class}] in file order - vtable starts, slot counts, and
    the hand-admitted class (empty string = not yet identified).

    Local parse, not core.tsv: the admitted file predates the tracked-table
    convention and simply OMITS the trailing class field on unidentified
    rows, which the strict reader rejects as ragged."""
    rows = []
    for line in (path or VTABLES).read_text().splitlines():
        if line.startswith("#") or not line.strip() or line.startswith("rva\t"):
            continue
        fields = line.split("\t")
        rows.append({"rva": int(fields[0], 16), "count": int(fields[1]),
                     "class": fields[2] if len(fields) > 2 else ""})
    return rows
