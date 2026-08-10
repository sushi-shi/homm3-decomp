"""homm3.vc6._unit - map a matching unit to its EXACT build compile.

The solvers' v1 gap was that they compiled the target's TU with a hardcoded
game profile plus `/I<src-dir>`, which perturbed the include closure (the C1
sensitivity) so the standalone obj diverged from - or omitted functions
present in - the real in-unit build obj. This module closes that: it reads the
unit's exact flag profile from config/units.toml (the same manifest the build
uses) so a mutant compiles the way the ratchet compiles, and it points at the
already-built base obj so DIAGNOSIS needs no recompile at all.

Proven 2026-08-09: compiling src/town.cpp with the unit's exact flags yields an
obj byte-identical to build/objdiff/base/town.obj outside the COFF timestamp.
"""
from __future__ import annotations

import subprocess
import sys
import tomllib
from functools import lru_cache
from pathlib import Path

from homm3.sema import _asm
from homm3.vc6 import _common

_UNITS_TOML = _common.REPO / "config/units.toml"


@lru_cache(maxsize=1)
def _manifest() -> dict:
    return tomllib.loads(_UNITS_TOML.read_text())


def flags_for_unit(unit: str) -> list[str] | None:
    """The unit's exact cl flag list (from its named profile), or None."""
    m = _manifest()
    profiles = m.get("flags", {})
    for u in m.get("unit", []):
        if u.get("unit") == unit:
            return list(profiles.get(u.get("flags"), []) or []) or None
    return None


def source_for_unit(unit: str) -> Path | None:
    for u in _manifest().get("unit", []):
        if u.get("unit") == unit:
            return _common.REPO / u["source"]
    return None


def unit_for_source(src: Path) -> str | None:
    """The unit whose manifest source is *src* (by resolved path)."""
    src = src.resolve()
    for u in _manifest().get("unit", []):
        if (_common.REPO / u["source"]).resolve() == src:
            return u.get("unit")
    return None


def base_obj(unit: str) -> Path | None:
    """The already-built in-unit base obj the ratchet scored, if present."""
    obj = _asm.BASE / f"{unit}.obj"
    return obj if obj.is_file() else None


def compile_text(text: str, unit: str, out_dir: Path, stem: str,
                 with_listing: bool = True) -> tuple[Path | None, str]:
    """Compile *text* as unit's TU with its EXACT build flags (+/FAs).

    A mutant is a copy of the real source compiled the way the build compiles
    it - so its codegen is faithful to the ratchet, not a standalone artifact.
    The real src/ file is never touched; the copy is written under out_dir.
    """
    flags = flags_for_unit(unit)
    if flags is None:
        return None, f"no flag profile for unit {unit!r} in units.toml"
    out_dir.mkdir(parents=True, exist_ok=True)
    src_copy = out_dir / f"{stem}.cpp"
    src_copy.write_text(text)
    obj = out_dir / f"{stem}.obj"
    cl_flags = list(flags)
    if with_listing and "/FAs" not in cl_flags:
        cl_flags.append("/FAs")
    cmd = [sys.executable, "-m", "homm3.core.cc_wrap",
           "--out", str(obj), "--src", str(src_copy), "--", *cl_flags]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0 or not obj.is_file():
        tail = "\n".join(
            (proc.stdout + proc.stderr).strip().splitlines()[-6:])
        return None, tail
    return obj, ""
