"""Compile a game unit at the Loki game profile (evidence builds only).

The shared source imports the real Windows SDK declarations through
platform.h's HOMM3_TARGET_LOKI branch. The SDK tree is VC6's include
directory as `homm3 mac sdk` stages it (build/mac/sdk/windows); its upper-case
file names are exposed in lower case through a symlink farm after glibc and
libstdc++ (`-idirafter`), so the C library headers stay the Linux ones."""
from __future__ import annotations

import subprocess
from pathlib import Path

from homm3.core import common
from homm3.loki_game.image import BUILD
from homm3.loki_game.scan import driver_command, profile_spec

ROOT = common.HOMM3_DIR
FARM = BUILD / "winsdk"


def sdk_farm() -> Path:
    source = ROOT / "build/mac/sdk/windows"
    if not source.is_dir():
        raise RuntimeError("the Windows SDK tree is not staged; run `homm3 mac sdk PATH`")
    FARM.mkdir(parents=True, exist_ok=True)
    for path in source.iterdir():
        name = path.name.lower()
        if not name.endswith((".h", ".inl")):
            continue
        link = FARM / name
        if not link.is_symlink():
            link.symlink_to(path)
    return FARM


def compile_unit(unit: str, out: Path | None = None) -> tuple[Path, str]:
    """(object, diagnostics) for src/<unit>.cpp; the object is missing on failure."""
    spec = profile_spec()["profile"]
    out = out or BUILD / "objects" / f"{unit}.o"
    out.parent.mkdir(parents=True, exist_ok=True)
    includes = ["include", "vendor/bink-0.5a/include", "vendor/ifc-2.0.3/include",
                "vendor/miles-5.0e/include", "vendor/smacker-3.2h/include", "vendor/zlib-1.1.3"]
    command = driver_command(
        *spec["flags"], "-fpermissive", "-w", "-DHOMM3_TARGET_LOKI=1",
        "-include", str(ROOT / "include/gcc_prefix.h"),
        *(f"-I{ROOT / p}" for p in includes), "-idirafter", str(sdk_farm()),
        "-c", f"{unit}.cpp", "-o", str(out), stl=spec.get("stl", "libstdc++"))
    from homm3.loki import toolchain
    done = subprocess.run(command, cwd=ROOT / "src", env=toolchain.environment(),
                          capture_output=True, text=True)
    return out, done.stderr
