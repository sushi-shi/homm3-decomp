"""Mac compiler settings for authored source and ordinary project headers."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import tomllib


@dataclass(frozen=True)
class Profile:
    unit: str
    include_dirs: tuple[str, ...]
    flags: tuple[str, ...]
    helpers: tuple[int, ...]
    mode: str = "paired_bodies"
    source_helpers: tuple[str, ...] = ()


def load(root: Path, unit: str) -> Profile | None:
    if not re.fullmatch(r"[A-Za-z0-9_-]+", unit):
        raise ValueError("unsafe Mac unit name")
    path = root / "config/mac/units.toml"
    row = tomllib.loads(path.read_text()).get("units", {}).get(unit) if path.is_file() else None
    local = root / "config/mac/units" / (unit + ".toml")
    if local.is_file():
        if row is not None:
            raise ValueError(f"{unit}: duplicate Mac unit profiles")
        row = tomllib.loads(local.read_text())
    if row is None:
        return None
    if row.get("mode") != "paired_bodies":
        raise ValueError(f"{unit}: unsupported Mac compilation mode")
    retired = {"preamble", "extra_headers", "native_headers", "include_dirs"} & row.keys()
    if retired:
        raise ValueError(f"{unit}: remove retired header settings {sorted(retired)}; "
                         "Mac uses the source include prefix and project include paths")
    flags = tuple(row.get("flags", ()))
    if flags and "-nolink" not in flags:
        raise ValueError(f"{unit}: Mac profile must emit an object with -nolink")
    source_helpers = row.get("source_helpers", [])
    if (not isinstance(source_helpers, list)
            or any(not isinstance(name, str) or not re.fullmatch(
                r'\w+(?:::\w+)*(?:\s*\([^;{}]*\)\s*(?:const)?)?', name)
                   for name in source_helpers)
            or len(set(source_helpers)) != len(source_helpers)):
        raise ValueError(f"{unit}: source_helpers must contain distinct C++ definition selectors")
    project = tomllib.loads((root / "config/units.toml").read_text())
    directories = tuple(project.get("build", {}).get("includes", ["include"]))
    for value in directories:
        if (not value or Path(value).is_absolute() or ".." in Path(value).parts
                or not (value == "include" or value.startswith(("include/", "vendor/")))):
            raise ValueError(f"{unit}: expected ordinary project/vendor include path: {value!r}")
    return Profile(unit, directories, flags, tuple(row.get("helpers", ())),
                   source_helpers=tuple(source_helpers))
