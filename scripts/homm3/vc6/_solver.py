"""Shared compile, symbol slicing and reference selection for VC6 solvers."""
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

from homm3.sema import _asm
from homm3.vc6 import _common

# Game C++ profile: config/units.toml `game_o2_ml_gr_windows` (minus the
# build-plumbing /nologo /c, added at invocation) + /GR- per the catalog's
# profile-under-test (0.6: no COL pointers / no __RTDynamicCast in retail).
GAME_FLAGS = ["/O2", "/Ob2", "/Oy-", "/Op", "/ML", "/Gr", "/GX", "/GR-",
              "/D_WINDOWS"]


def _compile_tu(src: Path, outdir: Path, include_dir: str | None = None):
    """cc_wrap one TU with the game profile + /FAs; (obj|None, error-tail).
    The .asm listing lands beside the obj for human reading."""
    outdir.mkdir(parents=True, exist_ok=True)
    obj = outdir / (src.stem + ".obj")
    flags = ["/c", *GAME_FLAGS]
    if include_dir:
        flags.append(f"/I{include_dir}")
    flags.append("/FAs")
    cmd = [sys.executable, "-m", "homm3.core.cc_wrap",
           "--out", str(obj), "--src", str(src), "--", *flags]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0 or not obj.is_file():
        tail = "\n".join((proc.stdout + proc.stderr).strip().splitlines()[-6:])
        return None, tail
    return obj, ""


def _wine_dir(path: Path) -> str | None:
    try:
        return subprocess.check_output(
            ["winepath", "-w", str(path)], text=True,
            stderr=subprocess.DEVNULL).strip() or None
    except (OSError, subprocess.CalledProcessError):
        return None


def _resolve_symbol(obj: Path, fn: str) -> str:
    from homm3.vc6 import _selection
    return _selection.object_symbol(obj, fn)


def _fn_text(obj: Path, fn: str, ordinal: int = 0) -> tuple:
    """(disassembly text, resolved symbol) for one selected object body."""
    sym = _resolve_symbol(obj, fn)
    return _asm.objdump(obj, sym, ordinal), sym


def _reference_side(args, scratch: Path) -> tuple:
    """(text, label) of the reference F. --against-src compiles a second
    TU (hermetic); --against UNIT:FN goes through the sema machinery -
    delinked target object when present, else capstone over the image."""
    if args.against_src:
        ref_src = Path(args.against_src).resolve()
        if not ref_src.is_file():
            _common.die(f"--against-src missing: {ref_src}")
        obj, tail = _compile_tu(ref_src, scratch / "ref",
                                _wine_dir(ref_src.parent))
        if obj is None:
            _common.die(f"reference TU failed to compile:\n{tail}")
        text, sym = _fn_text(obj, getattr(args, "_source_fn", args.fn))
        return text, f"compiled {ref_src.name} ({sym})"

    from homm3.vc6 import _selection
    return _selection.reference_text(args.against)
