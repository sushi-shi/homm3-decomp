"""homm3.manifest - config/units.toml, the per-TU build manifest.

The thin shared reader (gruntz template shape): parse and shape only, zero
policy. homm3.build.configure owns the manifest GATES (required keys,
duplicate units, known flag profiles, existing sources) and re-checks every
field before a graph is emitted; other consumers read through this module so
the toml shape is spelled in one place.

NOTE the manifest is NOT the label universe: retail_labels extraction sweeps
src/*.c* directly, because carcass/reference TUs carry VA() claims without
being manifest units, and the vendored zlib units are manifest units with no
src/ file (their claims are the config/retail-zlib-map.tsv provider table).
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
    """[{unit, source, flags}] in manifest order."""
    return list(load(path).get("unit", []))


def flag_profiles(path: Path | None = None) -> dict[str, list[str]]:
    return dict(load(path).get("flags", {}))


def by_unit(path: Path | None = None) -> dict[str, dict]:
    return {u["unit"]: u for u in units(path)}
