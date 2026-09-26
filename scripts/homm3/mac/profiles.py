"""CodeWarrior flags and platform dispositions from config/mac/units.toml."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import tomllib

PATH = "config/mac/units.toml"
DISPOSITIONS = ("windows_only", "platform_rewritten")


@dataclass(frozen=True)
class Settings:
    flags: tuple[str, ...]
    units: dict[str, dict]


def _read(root: Path) -> Settings:
    data = tomllib.loads((root / PATH).read_text())
    flags = data.get("flags")
    units = data.get("units", {})
    if not isinstance(flags, list) or not flags or "-nolink" not in flags:
        raise ValueError(f"{PATH}: shared flags must be a list including -nolink")
    known = {"flags", "units"}
    if set(data) - known:
        raise ValueError(f"{PATH}: unknown settings {sorted(set(data) - known)}")
    for unit, row in units.items():
        if not re.fullmatch(r"[A-Za-z0-9_-]+", unit) or not isinstance(row, dict):
            raise ValueError(f"{PATH}: invalid unit entry {unit!r}")
        extra = set(row) - {"flags", "disposition", "evidence"}
        if extra:
            raise ValueError(f"{PATH}: {unit}: unknown settings {sorted(extra)}")
        if "flags" in row and (not isinstance(row["flags"], list) or "-nolink" not in row["flags"]):
            raise ValueError(f"{PATH}: {unit}: flags must be a list including -nolink")
        if "disposition" in row and (row["disposition"] not in DISPOSITIONS
                                     or not str(row.get("evidence", "")).strip()):
            raise ValueError(f"{PATH}: {unit}: a disposition needs a known kind and evidence")
    return Settings(tuple(flags), units)


def flags(root: Path, unit: str) -> tuple[str, ...]:
    """The unit's override, else the shared profile."""
    settings = _read(root)
    return tuple(settings.units.get(unit, {}).get("flags", settings.flags))


def dispositions(root: Path) -> dict[str, tuple[str, str]]:
    """unit -> (disposition, evidence) for units with no Mac counterpart."""
    return {unit: (row["disposition"], row["evidence"])
            for unit, row in _read(root).units.items() if "disposition" in row}
