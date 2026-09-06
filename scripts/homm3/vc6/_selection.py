"""Shared retail selectors and candidate-source inference for VC6 solvers.

Retail identity is resolved by sema's SymbolDb. Explicit source experiments
can also select symbols emitted only in their objects, without retail data.
"""
from __future__ import annotations

import contextlib
from dataclasses import dataclass
from pathlib import Path
import re
import sys

from homm3.core import undname
from homm3.sema import _asm
from homm3.sema.context import SymbolDb, get_context
from homm3.vc6 import _common, _unit


@dataclass(frozen=True)
class Function:
    name: str
    unit: str
    rva: int
    size: int
    ordinal: int


def split(selector: str) -> tuple[str | None, str]:
    """Split UNIT:SELECTOR without treating a C++ scope as a unit prefix."""
    match = re.match(r"^([^:]+):(?!:)(.+)$", selector)
    if match:
        unit, name = match.groups()
        if unit == "dc" or unit.endswith(".obj"):
            _common.die("Dreamcast offsets are not retail selectors; use the "
                        "retail VA/RVA or function name")
        return unit, name
    return None, selector


def retail(selector: str, unit: str | None = None) -> Function:
    prefix, name = split(selector)
    if prefix and unit and prefix != unit:
        _common.die(f"selector unit {prefix!r} disagrees with source unit {unit!r}")
    unit = prefix or unit
    if unit:
        _check_unit(unit)
    ctx = get_context()
    db = (SymbolDb(extent_of=lambda: ctx.image.image_size, unit=unit)
          if unit else ctx.symbols)
    # Resolution notes must not corrupt a solver's --json stdout.
    with contextlib.redirect_stdout(sys.stderr):
        resolved = Function(*db.resolve_fn(name))
    return resolved


def _check_unit(unit: str) -> None:
    if (_unit.source_for_unit(unit) is None and
            not any(row[1] == unit for row in get_context().symbols.funcs.values())):
        _common.die(f"unknown retail unit {unit!r}")


def object_symbol(obj: Path, selector: str) -> str:
    """Resolve a candidate name, including demangled names in scratch TUs."""
    unit, name = split(selector)
    if unit or name.lower().startswith("0x"):
        name = retail(selector).name
    names = _asm._function_text_symbols(obj)
    if name in names:
        return name
    wanted = undname.strip_signature(name)
    qualified = undname.qualified_names(names)
    matches = [n for n, q in qualified.items() if q == wanted]
    if not matches:
        matches = [n for n, q in qualified.items()
                   if q.lower() == wanted.lower()]
    if not matches and "::" not in wanted:
        matches = [n for n, q in qualified.items() if undname.bare(q) == wanted]
    if not matches:
        matches = [n for n in names
                   if re.fullmatch(rf"[@_]?{re.escape(name)}(@\d+)?", n)]
    if not matches and "::" not in name and not name.startswith("?"):
        matches = [n for n in names if name in n]
    if len(matches) == 1:
        return matches[0]
    if matches:
        _common.die(f"selector {selector!r} is ambiguous in {obj.name}: "
                    + ", ".join(sorted(matches)[:12]))
    _common.die(f"selector {selector!r} matches no emitted function of "
                f"{obj.name}; symbols: " + ", ".join(sorted(names)[:12]))


def prepare(args) -> None:
    """Normalize SELECTOR or SOURCE --fn SELECTOR to the solver interface."""
    if getattr(args, "_selection_prepared", False):
        return
    positional = getattr(args, "src", None)
    fn = getattr(args, "fn", None)
    against = getattr(args, "against", None)
    against_src = getattr(args, "against_src", None)
    selected = None
    if against and against_src:
        _common.die("use only one of --against and --against-src")
    if fn and positional:
        src = Path(positional).resolve()
        if not src.is_file():
            src = _unit.source_for_unit(positional) or src
        if not src.is_file():
            _common.die(f"source missing: {src}")
        unit = _unit.unit_for_source(src)
        prefix, name = split(fn)
        if prefix or name.lower().startswith("0x"):
            selected = retail(fn, unit)
            fn = selected.name
            args._fn_ordinal = selected.ordinal
    else:
        selector = fn or positional
        if not selector:
            _common.die("provide a retail selector, or SOURCE --fn SELECTOR")
        if selector.endswith((".cpp", ".c", ".h")):
            _common.die("a source path requires --fn SELECTOR")
        selected = retail(selector)
        src = _unit.source_for_unit(selected.unit)
        if src is None:
            _common.die(f"{selector!r} has no manifest source; reconstruct its "
                        "owning TU before running a VC6 solver")
        unit, fn = selected.unit, selected.name
        args._fn_ordinal = selected.ordinal
        if not src.is_file():
            _common.die(f"source missing: {src}")
    if not against and not against_src:
        if not unit:
            _common.die("source is not a manifest unit; pass --against SELECTOR "
                        "or --against-src FILE")
        against = (f"{unit}:0x{selected.rva:x}" if selected
                   else f"{unit}:{fn}")
    args.src, args.fn = str(src), fn
    args._source_fn = fn
    args.against, args.against_src = against, against_src
    args._selection_prepared = True


def reference_unit(args) -> str | None:
    """Compile an explicit candidate with its own profile when available."""
    if getattr(args, "against_src", None):
        return None
    source_unit = _unit.unit_for_source(Path(args.src))
    if source_unit:
        return source_unit
    if not getattr(args, "against", None):
        return None
    unit, _ = split(args.against)
    if unit:
        _check_unit(unit)
        return unit if _unit.flags_for_unit(unit) else None
    unit = retail(args.against).unit
    return unit if _unit.flags_for_unit(unit) else None


def reference_text(selector: str) -> tuple[str, str]:
    """Resolve a retail reference, retaining explicit object-only identities."""
    unit, fn = split(selector)
    if unit and not fn.lower().startswith("0x"):
        _check_unit(unit)
        obj = _asm.TARGET / f"{unit}.c.obj"
        if obj.is_file():
            # Exact object identities may not have entered the retail ledger.
            # Never fall through on ambiguity and silently pick another body.
            name = object_symbol(obj, fn)
            return _asm.objdump(obj, name, 0), f"delinked {unit}.c.obj ({name})"
    selected = retail(selector)
    obj = _asm.TARGET / f"{selected.unit}.c.obj"
    if obj.is_file():
        return (_asm.objdump(obj, selected.name, selected.ordinal),
                f"delinked {selected.unit}.c.obj ({selected.name})")
    if not selected.size:
        _common.die(f"{selected.name} has no recorded size")
    return (_asm.image_text(get_context(), selected.rva, selected.size,
                            selected.name),
            f"retail image @ 0x{selected.rva:x} ({selected.name}, "
            f"{selected.size} B; capstone producer)")
