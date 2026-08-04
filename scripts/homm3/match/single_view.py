#!/usr/bin/env python3
"""homm3.match.single_view - every global must have ONE view: its real extern.

gruntz audit/single_view ported into match/ (our fatal-gate area). A
global declared with two different (type, linkage) signatures across the
tree is a SPLIT VIEW - two names/types for one datum at one address.
Only one can match retail's actual symbol; the other is a fake alias (a
`void*` placeholder, a cross-class re-view, a stale linkage) that
mis-models the datum and would emit a symbol a candidate link cannot
resolve. Recover the ONE real type and delete the view.

The tree currently declares ZERO externs (the cleanliness board's
`cpp extern decls` row keeps .cpp files that way; owner headers are
where externs will appear), so this gate is pure arrival-prevention:
the backlog (config/single-view-baseline.tsv, gruntz shape - frozen
splits reported as standing debt, drained in supervised review) starts
empty and any new split is fatal in the `homm3 build` tail. Never runs
in `--fast`.

Known blind spot (same as gruntz): a template type with pointers or
multiple arguments (`std::vector<char*>`) does not fit DECL_RE and its
declarations are not compared. Acceptable - the split-view hazard lives
overwhelmingly in scalar/pointer globals.

Runs its embedded selftest on every invocation - the gate proves it can
fail before it judges the tree.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

from homm3.core import common
# One comment/string/char stripper for every source gate - the board owns
# it (incl. the backtick-apostrophe lesson); do not fork the semantics.
from homm3.cleanliness.board import _strip

BASELINE = common.HOMM3_DIR / "config/single-view-baseline.tsv"
ROOTS = ("src", "include")
EXTS = {".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}

# extern TYPE [*&...] NAME [array] ;   (TYPE may be qualified / templated)
# `extern "C"` is rewritten to a sentinel BEFORE stripping - the shared
# string-stripper would otherwise eat the "C" and erase the linkage
# distinction (caught by this gate's own selftest on landing day).
_EXTERN_C = re.compile(r'extern\s+"C"')
DECL_RE = re.compile(
    r"^[ \t]*(extern__C\s+|extern\s+)"
    r"([A-Za-z_][\w:<>]*(?:\s*\*+)?)\s+"
    r"\**([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*;",
    re.MULTILINE)


def collect(files) -> dict:
    """name -> {(type, linkage): set(display paths)} over stripped text.
    `files` is an iterable of (display_path, raw_text)."""
    views: dict[str, dict] = {}
    for path, raw in files:
        text = _strip(_EXTERN_C.sub("extern__C", raw))
        for m in DECL_RE.finditer(text):
            linkage = "C" if "__C" in m.group(1) else "C++"
            declared_type = re.sub(r"\s+", " ", m.group(2).strip())
            views.setdefault(m.group(3), {}).setdefault(
                (declared_type, linkage), set()).add(path)
    return views


def splits(views: dict) -> list:
    """[(name, {(type, linkage): paths})] for every split-view global."""
    return sorted((name, forms) for name, forms in views.items()
                  if len(forms) > 1)


def render(name: str, forms: dict) -> list[str]:
    lines = [f"SPLIT VIEW: {name} has {len(forms)} declared views:"]
    for (declared_type, linkage), paths in sorted(forms.items()):
        lines.append(f"  extern {declared_type} [{linkage}] in "
                     + ", ".join(sorted(paths)))
    return lines


def load_backlog() -> set[str]:
    if not BASELINE.is_file():
        return set()
    return {line.split("\t")[0] for line in BASELINE.read_text().splitlines()
            if line and not line.startswith("#")}


def write_backlog(found: list) -> None:
    head = ("# KNOWN split-view backlog - standing debt reported by\n"
            "# homm3.match.single_view, drained in supervised review;\n"
            "# NEW splits are fatal.\n")
    BASELINE.write_text(head + "".join(
        f"{name}\t{len(forms)} views\n" for name, forms in found))


# --- the embedded negative control ------------------------------------------------

def selftest() -> list[str]:
    failures = []

    def names(files):
        return [name for name, _ in splits(collect(files))]

    if names([("a.h", "extern int g_x;"), ("b.cpp", "extern short g_x;")]) \
            != ["g_x"]:
        failures.append("type split not detected")
    if names([("a.h", 'extern "C" int g_y;'),
              ("b.h", "extern int g_y;")]) != ["g_y"]:
        failures.append("linkage split not detected")
    if names([("a.h", "extern int g_z;"), ("b.h", "extern int g_z;")]):
        failures.append("consistent extern wrongly split")
    if names([("a.h", "extern TTown* g_p;"),
              ("b.h", "extern void* g_p;")]) != ["g_p"]:
        failures.append("pointer-view split not detected")
    if names([("a.cpp", "int g_w;"), ("b.cpp", "short g_w;")]):
        failures.append("non-extern definitions wrongly compared")
    if names([("a.h", "// extern int g_c;"),
              ("b.h", "extern short g_c;")]):
        failures.append("commented declaration not stripped")
    return failures


# --- entry points -----------------------------------------------------------------

def _scan() -> list:
    files = []
    for root in ROOTS:
        base = common.HOMM3_DIR / root
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix in EXTS and path.is_file():
                files.append((str(path.relative_to(common.HOMM3_DIR)),
                              path.read_text(errors="ignore")))
    return splits(collect(files))


def run_gate() -> list[str]:
    """Selftest + the real tree. Returns FATAL lines (empty = pass);
    backlogged splits print as standing debt instead."""
    broken = selftest()
    if broken:
        return [f"single-view SELFTEST BROKEN: {b}" for b in broken]
    found = _scan()
    backlog = load_backlog()
    fatal = []
    for name, forms in found:
        if name not in backlog:
            fatal.extend(render(name, forms))
    known = sum(1 for name, _ in found if name in backlog)
    stale = backlog - {name for name, _ in found}
    if not fatal:
        summary = f"[build] single-view: {len(found)} split(s)"
        if known:
            summary += f" ({known} known-backlog)"
        if stale:
            summary += (f"; {len(stale)} baseline row(s) no longer fire - "
                        "remove them")
        print(summary + " - no new splits")
    return fatal


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--selftest" in argv:
        broken = selftest()
        for b in broken:
            print(f"SELFTEST BROKEN: {b}", file=sys.stderr)
        print("selftest OK" if not broken else "selftest FAILED")
        return 2 if broken else 0
    if "--write-baseline" in argv:
        found = _scan()
        write_backlog(found)
        print(f"single-view backlog frozen: {len(found)} split(s) -> "
              f"{BASELINE.relative_to(common.HOMM3_DIR)}")
        return 0
    fatal = run_gate()
    for line in fatal:
        print(line, file=sys.stderr)
    return 1 if fatal else 0


if __name__ == "__main__":
    sys.exit(main())
