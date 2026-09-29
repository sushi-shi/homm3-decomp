"""homm3.retail_labels.providers - the committed claim channels, parse-only.

Hand-admitted config/ tables, each returned as Claim records in a
deterministic order. One intra-table consistency check lives here (two
alias rows disagreeing on one target's owner is a defect of the TABLE, like
a duplicate census row); every cross-channel decision - precedence, the
`in rows` skip guards, enrichment fallbacks - lives in homm3.model.

"""

from __future__ import annotations

from collections import Counter
from pathlib import Path

from homm3.core import common
from homm3.core.tsv import read as read_tsv
from homm3.retail_labels import Claim

ZLIB_MAP = common.HOMM3_DIR / "config/retail/zlib-map.tsv"
RUNTIME_MAP = common.HOMM3_DIR / "config/retail/runtime-map.tsv"
RELOC_ALIASES = common.HOMM3_DIR / "config/retail/reloc-aliases.tsv"
RUNTIME_CONTRIBUTIONS = (common.HOMM3_DIR
                         / "config/retail/runtime-contributions.tsv")
RELOC_EVIDENCE = common.HOMM3_DIR / "config/retail/reloc-evidence.tsv"


def zlib_map(path: Path | None = None) -> list[Claim]:
    """The reviewed vendored-zlib function/data map; old rows default to func."""
    _b, _h, raw = read_tsv(path or ZLIB_MAP)
    claims = []
    for r in raw:
        kind = r.get('kind', 'func')
        if kind not in ('func', 'data'):
            raise ValueError(f'unknown zlib map kind {kind!r} at {r["rva"]}')
        claims.append(Claim(int(r['rva'], 16), r['name'], kind,
                            'zlib-map' if kind == 'func' else 'zlib-data-map',
                            int(r['size']), r['unit'], {}))
    return claims


def runtime_map(path: Path | None = None) -> list[Claim]:
    """MSVC runtime internals: label-only (the model takes the extent from
    the function census); empty unit - vostok buckets these into
    _msvc_internal objects, which is correct."""
    _b, _h, raw = read_tsv(path or RUNTIME_MAP)
    return [Claim(int(r["rva"], 16), r["name"], "func", "runtime-map",
                  None, "", {}) for r in raw]


def runtime_data_symbols(path: Path | None = None) -> list[Claim]:
    """Library data symbols proven by the reviewed runtime placements.

    Each placed data/bss COFF section of a pinned archive member names the
    symbol defined at its start (DXGUID's `_DPAID_ServiceProvider`,
    LIBCPMT's `?_Fpz@std@@3_JB`). Label-only: the model applies these names
    where it would otherwise invent a dense `const_`/`data_`/`bss_` label,
    so a game reference to the library object compares by its real name.
    """
    _b, _h, raw = read_tsv(path or RUNTIME_CONTRIBUTIONS)
    rows = [r for r in raw if r.get("kind") in ("data", "bss")
            and r.get("library") != "zlib"
            and (r.get("symbol") or "-")[:1] in ("_", "?")]
    # Member-local statics ($T, $S) and repeated names identify no one object.
    counts = Counter(r["symbol"] for r in rows)
    return [Claim(int(r["rva"], 16), r["symbol"], "data", "runtime-data",
                  None, "", {}) for r in rows if counts[r["symbol"]] == 1]


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
