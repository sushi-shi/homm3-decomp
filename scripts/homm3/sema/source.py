"""Candidate source statements from an on-demand VC6 ``/Z7`` build.

The debug object is a disposable side artifact.  Its line records annotate the
normal matching object's assembly only after the selected function's code bytes
have been proven identical across the two compilations.
"""
from __future__ import annotations

import bisect
from dataclasses import dataclass
import json
import os
from pathlib import Path
import subprocess
import sys

from homm3 import manifest
from homm3.core import codeview, common
from homm3.core import cc_wrap
from homm3.core.cc_wrap import scan_header_deps


DEBUG_DIR = common.HOMM3_DIR / "build/debug"
STAMP_SCHEMA = 1


class SourceError(RuntimeError):
    pass


class NoLineRecords(SourceError):
    """/Z7 emitted no statement rows for the function: a compiler-generated
    body (destructor, scalar deleting destructor, operator=, thunk) has no
    source statements to label. Callers fall back to the unlabelled view."""


@dataclass(frozen=True)
class Statement:
    offset: int
    line: int
    text: str


@dataclass(frozen=True)
class SourceMap:
    source: str
    statements: tuple[Statement, ...]

    def __post_init__(self):
        grouped: dict[int, list[Statement]] = {}
        for statement in self.statements:
            grouped.setdefault(statement.offset, []).append(statement)
        object.__setattr__(self, "_grouped", grouped)
        object.__setattr__(self, "_offsets", tuple(sorted(grouped)))

    def heads_at(self, offset: int) -> tuple[Statement, ...]:
        return tuple(self._grouped.get(offset, ()))

    def active_at(self, offset: int) -> tuple[Statement, ...]:
        index = bisect.bisect_right(self._offsets, offset) - 1
        if index < 0:
            return ()
        return tuple(self._grouped[self._offsets[index]])


def format_heading(statement: Statement, source: str = "",
                   changed: bool = False) -> str:
    marker = "!! " if changed else ""
    location = f"{source}:{statement.line}" if source else f"line {statement.line}"
    return (f"; {marker}{location} | {statement.text}"
            if statement.text else f"; {marker}{location}")


def _strip_comments(line: str, in_block: bool) -> tuple[str, bool]:
    """Enough lexical filtering to find the first executable body line."""
    out = []
    i = 0
    while i < len(line):
        if in_block:
            end = line.find("*/", i)
            if end < 0:
                return "".join(out), True
            in_block = False
            i = end + 2
            continue
        if line.startswith("//", i):
            break
        if line.startswith("/*", i):
            in_block = True
            i += 2
            continue
        out.append(line[i])
        i += 1
    return "".join(out), in_block


def _first_body_line(lines: list[str], begin_line: int) -> int:
    """Resolve VC6's implicit offset-zero statement from the ``.bf`` line.

    ``.bf`` normally names the opening brace, while the first executable line
    has no IMAGE_LINENUMBER row.  Scan forward over comments/directives/braces
    so a one-line wrapper is labelled by its ``return``, not by ``{``.
    """
    start = max(begin_line, 1)
    in_block = False
    for line_number in range(start, len(lines) + 1):
        code, in_block = _strip_comments(lines[line_number - 1], in_block)
        stripped = code.strip()
        if line_number == begin_line and "{" in stripped:
            stripped = stripped.split("{", 1)[1].strip()
        if (not stripped or stripped.startswith("#")
                or stripped in ("{", "}", "};")):
            continue
        return line_number
    return start


def _cache_inputs(source: Path) -> list[Path]:
    inputs = [source.resolve()]
    inputs.extend(Path(path) for path in scan_header_deps(
        source, common.HOMM3_DIR / "include",
        common.HOMM3_DIR / cc_wrap.ZLIB_INC))
    return sorted(set(inputs), key=str)


def _cache_payload(unit: str, source: Path, flags: list[str]) -> dict:
    return {
        "schema": STAMP_SCHEMA,
        "unit": unit,
        "flags": list(flags),
        "inputs": {
            os.path.relpath(path, common.HOMM3_DIR): common.sha256_of(path)
            for path in _cache_inputs(source)
        },
    }


def _debug_obj(unit: str) -> tuple[Path, Path, str]:
    units = manifest.by_unit()
    definition = units.get(unit)
    if definition is None:
        raise SourceError(
            f"{unit or 'no unit'} is not a manifest unit; candidate source "
            "labels require a compiled TU")
    profiles = manifest.flag_profiles()
    flags = list(profiles.get(definition.get("flags", ""), ()))
    if not flags:
        raise SourceError(f"{unit} has no usable compiler flag profile")
    source_rel = definition.get("source", "")
    source_path = common.HOMM3_DIR / source_rel
    if not source_path.is_file():
        raise SourceError(f"manifest source is missing: {source_rel}")

    DEBUG_DIR.mkdir(parents=True, exist_ok=True)
    obj = DEBUG_DIR / f"{unit}.obj"
    stamp = obj.with_name(obj.name + ".stamp.json")
    expected = _cache_payload(unit, source_path, flags)
    fresh = False
    if obj.is_file() and stamp.is_file():
        try:
            fresh = json.loads(stamp.read_text()) == expected
        except (OSError, json.JSONDecodeError):
            fresh = False
    if not fresh:
        env = os.environ.copy()
        scripts = str(common.HOMM3_DIR / "scripts")
        env["PYTHONPATH"] = (scripts + os.pathsep + env["PYTHONPATH"]
                             if env.get("PYTHONPATH") else scripts)
        env["PYTHONDONTWRITEBYTECODE"] = "1"
        cmd = [
            sys.executable, "-m", "homm3.core.cc_wrap",
            "--out", str(obj), "--src", str(source_path), "--",
            *flags, "/Z7",
        ]
        result = subprocess.run(
            cmd, cwd=common.HOMM3_DIR, env=env,
            capture_output=True, text=True)
        if result.returncode != 0 or not obj.is_file():
            detail = "\n".join(
                (result.stdout + result.stderr).strip().splitlines()[-12:])
            raise SourceError(
                f"/Z7 compile failed for {unit}; run inside "
                f"`nix develop .#build`{(':\n' + detail) if detail else ''}")
        stamp.write_text(json.dumps(expected, indent=2, sort_keys=True) + "\n")
    return obj, source_path, source_rel


def _same_logical_code(base: bytes, debug: bytes) -> bool:
    if base == debug:
        return True
    if len(base) > len(debug) and len(base) - len(debug) <= 15:
        return base[:len(debug)] == debug and set(base[len(debug):]) <= {0x90}
    if len(debug) > len(base) and len(debug) - len(base) <= 15:
        return debug[:len(base)] == base and set(debug[len(base):]) <= {0x90}
    return False


def load(unit: str, name: str, ordinal: int, base_obj: Path) -> SourceMap:
    """Compile/load one TU's debug object and return a verified source map."""
    debug_obj, source_path, source_rel = _debug_obj(unit)
    try:
        info = codeview.parse_lines(debug_obj).get((name, ordinal))
        if info is None:
            raise NoLineRecords(
                f"/Z7 emitted no classic line records for {name} "
                f"occurrence {ordinal} in {unit} - a compiler-generated "
                "body has no source statements to label")
        base_code = codeview.function_bytes(base_obj, name, ordinal)
    except codeview.CodeViewError as exc:
        raise SourceError(str(exc)) from exc
    if not _same_logical_code(base_code, info.code):
        raise SourceError(
            f"/Z7 code for {name} is not byte-identical to "
            f"{base_obj.relative_to(common.HOMM3_DIR)}; refusing unsafe "
            "source offsets (run `homm3 build` and retry)")

    source_lines = source_path.read_text(errors="replace").splitlines()
    raw = [(record.offset, record.line) for record in info.lines]
    if not any(offset == 0 for offset, _line in raw):
        raw.insert(0, (0, _first_body_line(source_lines, info.begin_line)))
    seen = set()
    statements = []
    for offset, line in sorted(raw, key=lambda item: item[0]):
        if (offset, line) in seen:
            continue
        seen.add((offset, line))
        if not 1 <= line <= len(source_lines):
            raise SourceError(
                f"/Z7 line {line} for {name} is outside manifest source "
                f"{source_rel} ({len(source_lines)} lines); refusing to show "
                "a possibly wrong source label")
        text = source_lines[line - 1].strip()
        statements.append(Statement(offset, line, text))
    if not statements:
        raise SourceError(f"no source statements recovered for {name}")
    return SourceMap(source_rel, tuple(statements))


def render_disassembly(text: str, source_map: SourceMap,
                       verbose: bool) -> str:
    """Interleave statement headings with an already-selected base listing.

    The default rows are the same folded listing plain `disasm` prints
    (addresses, call/data symbols in the operands); --verbose keeps the
    raw objdump rows with byte columns and reloc lines."""
    from homm3.sema import _asm

    if verbose:
        rows = [(parsed[0] if parsed else None, line)
                for line in text.splitlines()
                if (parsed := _asm.parse_ins(line)) is not None
                or line.strip()]
    else:
        rows = _asm.lite_rows(text)
    first = next((offset for offset, _line in rows if offset is not None), 0)
    out = []
    for offset, line in rows:
        if offset is not None:
            for statement in source_map.heads_at(offset - first):
                out.append(format_heading(statement, source_map.source))
        out.append(line)
    return "\n".join(out) + "\n"
