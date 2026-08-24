"""homm3.retail_labels.providers - the committed claim channels, parse-only.

Hand-admitted config/ tables, each returned as Claim records in a
deterministic order. One intra-table consistency check lives here (two
alias rows disagreeing on one target's owner is a defect of the TABLE, like
a duplicate census row); every cross-channel decision - precedence, the
`in rows` skip guards, enrichment fallbacks - lives in homm3.model.

The two evidence/ readers at the bottom are ENRICHMENT ONLY: evidence/ is
generated scaffolding slated for removal (user decision 2026-08-04), so a
missing file degrades names, never labels.
"""

from __future__ import annotations

import csv
from pathlib import Path

from homm3.core import common
from homm3.core.tsv import read as read_tsv
from homm3.retail_labels import Claim

ZLIB_MAP = common.HOMM3_DIR / "config/retail-zlib-map.tsv"
RUNTIME_MAP = common.HOMM3_DIR / "config/retail-runtime-map.tsv"
RELOC_ALIASES = common.HOMM3_DIR / "config/delink-reloc-aliases.tsv"
RELOC_EVIDENCE = common.HOMM3_DIR / "config/retail-reloc-evidence.tsv"
EVIDENCE_SYMBOLS = common.EVIDENCE_DIR / "retail-symbols.csv"
EVIDENCE_VTABLE_SYMBOLS = common.EVIDENCE_DIR / "retail-vtable-symbols.csv"


def _load_csv(path: Path) -> list[dict]:
    with path.open() as fh:
        return list(csv.DictReader(
            line for line in fh if not line.startswith("#")))


def zlib_map(path: Path | None = None) -> list[Claim]:
    """The reviewed vendored-zlib map: rva, size, name, owning TU."""
    _b, _h, raw = read_tsv(path or ZLIB_MAP)
    return [Claim(int(r["rva"], 16), r["name"], "func", "zlib-map",
                  int(r["size"]), r["unit"], {}) for r in raw]


def runtime_map(path: Path | None = None) -> list[Claim]:
    """MSVC runtime internals: label-only (the model takes the extent from
    the function census); empty unit - vostok buckets these into
    _msvc_internal objects, which is correct."""
    _b, _h, raw = read_tsv(path or RUNTIME_MAP)
    return [Claim(int(r["rva"], 16), r["name"], "func", "runtime-map",
                  None, "", {}) for r in raw]


def reloc_aliases(path: Path | None = None) -> list[Claim]:
    """Reviewed relocation-alias OWNERS, anchored at their symbol bases.

    ``target_rva`` is the concrete stripped-image operand, while ``addend``
    is the displacement from the source-level owner symbol.  Therefore the
    PDB owner belongs at ``target_rva - addend``.  Many exact sites and many
    interior targets may legitimately collapse onto that one owner/base.
    """
    _b, _h, raw = read_tsv(path or RELOC_ALIASES)
    owner_by_target: dict[int, str] = {}
    owner_by_base: dict[int, str] = {}
    base_by_owner: dict[str, int] = {}
    for r in raw:
        target = int(r["target_rva"], 16)
        addend = int(r["addend"], 0)
        base = target - addend
        owner = r["owner"]
        prior = owner_by_target.get(target)
        if prior and prior != owner:
            common.die(f"reloc aliases disagree at data rva 0x{target:x}: "
                       f"{prior!r} vs {owner!r}")
        owner_by_target[target] = owner
        prior = owner_by_base.get(base)
        if prior and prior != owner:
            common.die(f"reloc aliases disagree at owner base 0x{base:x}: "
                       f"{prior!r} vs {owner!r}")
        owner_by_base[base] = owner
        prior_base = base_by_owner.get(owner)
        if prior_base is not None and prior_base != base:
            common.die(f"reloc alias owner {owner!r} has two bases: "
                       f"0x{prior_base:x} vs 0x{base:x}")
        base_by_owner[owner] = base
    return [Claim(base, owner, "data", "reloc-alias", None, "", {})
            for base, owner in sorted(owner_by_base.items())]


def reloc_targets(path: Path | None = None) -> list[int]:
    """Absolute-relocation target rvas (data / literal classes) in file
    order, duplicates preserved - dense naming is the model's job."""
    _b, header, raw = read_tsv(path or RELOC_EVIDENCE)
    return [int(r["value"], 16) - common.IMAGE_BASE for r in raw
            if r["target_class"] in ("data", "literal-start",
                                     "literal-interior")]


def evidence_symbols() -> dict[int, str]:
    """{rva: name} enrichment for working labels; {} when the scaffolding
    file is absent."""
    if not EVIDENCE_SYMBOLS.is_file():
        return {}
    return {int(r["rva"], 16): r["name"]
            for r in _load_csv(EVIDENCE_SYMBOLS)}


def evidence_vtable_classes() -> dict[int, str]:
    """{vtable rva: candidate class} enrichment (first row wins, offset-0
    attributions only); {} when the scaffolding file is absent."""
    out: dict[int, str] = {}
    if EVIDENCE_VTABLE_SYMBOLS.is_file():
        for r in _load_csv(EVIDENCE_VTABLE_SYMBOLS):
            if r["class"] and r["class_addr_offset"] == "0":
                out.setdefault(int(r["vtable_rva"], 16), r["class"])
    return out
