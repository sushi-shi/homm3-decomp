"""Explicit Mac compilation scopes and reproducible project-header inputs."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import tomllib


@dataclass(frozen=True)
class Profile:
    unit: str
    preamble: str
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
    flags = tuple(row.get("flags", ()))
    source_helpers = row.get("source_helpers", [])
    if (not isinstance(source_helpers, list)
            or any(not isinstance(name, str) or not re.fullmatch(
                r'\w+(?:::\w+)*(?:\s*\([^;{}]*\)\s*(?:const)?)?', name)
                   for name in source_helpers)
            or len(set(source_helpers)) != len(source_helpers)):
        raise ValueError(f"{unit}: source_helpers must contain distinct C++ definition selectors")
    if flags and "-nolink" not in flags:
        raise ValueError(f"{unit}: Mac profile must emit an object with -nolink")
    for value in (row["preamble"], *row.get("include_dirs", ())):
        if not value or Path(value).is_absolute() or ".." in Path(value).parts:
            raise ValueError(f"{unit}: Mac input paths must be project-relative")
    return Profile(unit, row["preamble"], tuple(row.get("include_dirs", ())),
                   flags, tuple(row.get("helpers", ())), source_helpers=tuple(source_helpers))


def headers(root: Path, profile: Profile) -> dict[str, bytes]:
    """Capture the complete literal include closure; unsupported includes fail.

    Conditional includes are captured conservatively too. Inputs must be
    project files: an unstaged host SDK cannot silently enter a comparison.
    """
    from homm3.retail_labels.source import mask_lexical_noise
    root = root.resolve()
    result = {}
    shared = {}
    manifest = root / "config/mac/shared-bodies.toml"
    if manifest.is_file():
        for row in tomllib.loads(manifest.read_text()).get("bodies", []):
            name, owner = row["name"], row["owner"]
            if (not re.fullmatch(r"[A-Za-z0-9_]+", name)
                    or not owner.startswith(("include/", "src/"))
                    or ".." in Path(owner).parts):
                raise ValueError("invalid Mac shared-body entry")
            owner_path = root / owner
            source = owner_path.read_text()
            begin = f"// HOMM3_MAC_SHARED_BEGIN {name}\n"
            end = f"// HOMM3_MAC_SHARED_END {name}\n"
            if source.count(begin) != 1 or source.count(end) != 1:
                raise ValueError(f"{owner}: missing or duplicate Mac shared body {name}")
            body = source.split(begin, 1)[1].split(end, 1)[0]
            if not body.strip() or name in shared:
                raise ValueError(f"{owner}: empty or duplicate Mac shared body {name}")
            shared[f"include/mac_shared/{name}.h"] = body.encode()
    include = re.compile(r'^[ \t]*#[ \t]*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
    directive = re.compile(r'^[ \t]*#[ \t]*include\b', re.MULTILINE)

    def visit(path):
        path = path.resolve()
        if not path.is_relative_to(root):
            raise ValueError(f"Mac header is outside project inputs: {path}")
        name = path.relative_to(root).as_posix()
        if name in result:
            return
        data = shared.get(name)
        if data is None:
            data = path.read_bytes()
        result[name] = data
        text = data.decode()
        # Mask comments but preserve string literals for include operands.
        masked = mask_lexical_noise(text)
        live_lines = {text.count("\n", 0, match.start())
                      for match in directive.finditer(masked)}
        matches = [match for match in include.finditer(text)
                   if text.count("\n", 0, match.start()) in live_lines]
        if len(matches) != len(live_lines):
            raise ValueError(f"{name}: macro include needs an explicit Mac header adaptation")
        for match in matches:
            target = match.group(1)
            choices = [path.parent / target, *(root / base / target for base in profile.include_dirs)]
            found = next((candidate for candidate in choices
                          if candidate.is_file()
                          or candidate.resolve().relative_to(root).as_posix() in shared), None)
            if found is None:
                raise ValueError(f"{name}: missing Mac include {target!r}; extend the unit profile")
            visit(found)

    visit(root / profile.preamble)
    return result
