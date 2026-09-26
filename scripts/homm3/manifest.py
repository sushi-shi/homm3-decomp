"""homm3.manifest - config/units.toml, the per-TU build manifest.

The thin shared reader (gruntz template shape): parse and shape only, zero
policy. homm3.build.configure owns the manifest GATES (required keys,
duplicate units, known flag profiles, existing sources) and re-checks every
field before a graph is emitted; other consumers read through this module so
the toml shape is spelled in one place.

NOTE the manifest is NOT the label universe: retail_labels extraction sweeps
src/*.c* directly, because carcass/reference TUs carry VA() claims without
being manifest units, and the vendored zlib units are manifest units with no
src/ file (their claims are the config/retail/zlib-map.tsv provider table).
"""

from __future__ import annotations

import tomllib
from pathlib import Path

from homm3.core import common

MANIFEST = common.HOMM3_DIR / "config/units.toml"


def load(path: Path | None = None) -> dict:
    with open(path or MANIFEST, "rb") as fh:
        return tomllib.load(fh)


def units(path: Path | None = None) -> list[dict]:
    """[{unit, source, flags, module?}] in manifest order."""
    return list(load(path).get("unit", []))


def flag_profiles(path: Path | None = None) -> dict[str, list[str]]:
    return dict(load(path).get("flags", {}))


def by_unit(path: Path | None = None) -> dict[str, dict]:
    return {u["unit"]: u for u in units(path)}


def header_comparisons(path: Path | None = None) -> dict[int, str]:
    """Reviewed VA-to-object bindings; names still come from header annotations."""
    data = load(path)
    units = {unit["unit"] for unit in data.get("unit", [])}
    rows = data.get("header_comparisons", {})
    if not isinstance(rows, dict):
        raise ValueError("header_comparisons must be a table")
    result = {}
    for value, unit in rows.items():
        try:
            address = int(value, 0)
        except (TypeError, ValueError) as exc:
            raise ValueError(f"invalid header comparison VA: {value!r}") from exc
        if (not common.IMAGE_BASE <= address <= 0xffffffff
                or not isinstance(unit, str) or unit not in units):
            raise ValueError(f"invalid header comparison: {value!r} = {unit!r}")
        rva = address - common.IMAGE_BASE
        if rva in result:
            raise ValueError(f"duplicate header comparison VA: {value!r}")
        result[rva] = unit
    return result
