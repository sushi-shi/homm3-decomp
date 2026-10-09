"""Compile a game unit at the Loki game profile (evidence builds only).

No Windows SDK is parsed: include/loki holds the Win32 platform types and the
vendor (RAD, Miles, Immersion) declarations as the Loki port spells them,
ahead of the shared headers, and include/gcc_prefix.h the compiler and C
library spellings. Both exist on work/loki-game only."""
from __future__ import annotations

import re
import subprocess
from pathlib import Path

from homm3.core import common
from homm3.loki_game.image import BUILD
from homm3.loki_game.scan import driver_command, profile_spec

ROOT = common.HOMM3_DIR


IF_ZERO = re.compile(r"\s*#\s*if\s+0\b")
CONDITIONAL = re.compile(r"\s*#\s*(if|ifdef|ifndef)\b")
ENDIF = re.compile(r"\s*#\s*endif\b")


def without_disabled_text(text: str) -> str:
    """The unit with its `#if 0` regions blanked, line numbers kept. The
    regions hold MSVC special names (`vbase destructor') whose lone quote
    g++ 2.95's preprocessor rejects even while skipping; VC6 never reads
    them either."""
    out, depth = [], 0
    for line in text.split("\n"):
        if depth:
            if CONDITIONAL.match(line):
                depth += 1
            elif ENDIF.match(line):
                depth -= 1
            out.append("")
        elif IF_ZERO.match(line):
            depth = 1
            out.append("")
        else:
            out.append(line)
    return "\n".join(out)


def compile_unit(unit: str, out: Path | None = None) -> tuple[Path, str]:
    """(object, diagnostics) for src/<unit>.cpp; the object is missing on failure."""
    spec = profile_spec()["profile"]
    out = out or BUILD / "objects" / f"{unit}.o"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)
    includes = ["include/loki", "include", "vendor/zlib-1.1.3"]
    source = (ROOT / "src" / f"{unit}.cpp").read_text(errors="surrogateescape")
    command = driver_command(
        *spec["flags"], "-fpermissive", "-w", "-DHOMM3_TARGET_LOKI=1",
        "-include", str(ROOT / "include/gcc_prefix.h"),
        *(f"-I{ROOT / p}" for p in includes),
        "-x", "c++", "-c", "-", "-o", str(out), stl=spec.get("stl", "libstdc++"))
    from homm3.loki import toolchain
    done = subprocess.run(command, cwd=ROOT / "src", env=toolchain.environment(),
                          input=f'#line 1 "{unit}.cpp"\n' + without_disabled_text(source),
                          capture_output=True, text=True, errors="surrogateescape")
    return out, done.stderr
