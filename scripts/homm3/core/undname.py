"""homm3.core.undname - MSVC name demangling for the navigation tools.

`llvm-undname` (LLVM, in the dev shell) does the decoding; this module
batches names through it and reduces each result to the QUALIFIED NAME
(`game::GetTeam`, `CHeroWindowEx::~CHeroWindowEx`, `AI_value_of_event`),
which is the spelling the Dreamcast CodeView corpus and humans use. The
retail inventory is mangled-only, so this is the bridge that lets a
`Class::method` reach `homm3 sema` and a `?method@Class@@...` reach
`homm3 dreamcast`. Without the binary every function degrades to "no
match" - never a wrong match.
"""
from __future__ import annotations

import re
import shutil
import subprocess
from typing import Iterable

_ACCESS = re.compile(r"^(?:public|protected|private): ")
_SIGNATURE = re.compile(r"\((?:[^()]|\([^()]*\))*\)(?:\s*const)?(?:\s*volatile)?$")


def available() -> bool:
    return shutil.which("llvm-undname") is not None


def demangle(names: Iterable[str]) -> dict[str, str]:
    """mangled -> demangled for every name llvm-undname accepts."""
    wanted = [n for n in dict.fromkeys(names) if n.startswith("?")]
    if not wanted or not available():
        return {}
    res = subprocess.run(["llvm-undname"], input="\n".join(wanted) + "\n",
                         capture_output=True, text=True)
    out: dict[str, str] = {}
    lines = [ln for ln in res.stdout.splitlines() if ln.strip()]
    # The tool echoes each input line to stdout, then its result; a name
    # it rejects gets only an "error:" line on STDERR, so the echo is then
    # followed directly by the next echo. A result never starts with "?".
    i = 0
    for name in wanted:
        while i < len(lines) and lines[i] != name:
            i += 1
        if (i + 1 < len(lines) and not lines[i + 1].startswith("?")
                and not lines[i + 1].startswith("error:")):
            out[name] = lines[i + 1]
        i += 1
    return out


def qualified(demangled: str) -> str | None:
    """The qualified name inside a demangled declaration: drop access,
    return type, calling convention and the parameter list.

    'public: int __thiscall game::GetTeam(int) const' -> 'game::GetTeam'
    """
    text = _ACCESS.sub("", demangled.strip())
    text = re.sub(r"^(?:virtual |static )+", "", text)
    # The parameter list is the LAST balanced (...) group; `operator()`
    # keeps its own empty pair because that pair is not last.
    match = _SIGNATURE.search(text)
    head = text[:match.start()] if match else text
    head = head.rstrip()
    # Walk back to the last whitespace that is not inside <...>, (...)
    # or a `quoted' special name.
    depth = 0
    quoted = False
    start = 0
    for i in range(len(head) - 1, -1, -1):
        ch = head[i]
        if ch == "'":
            quoted = True
        elif ch == "`":
            quoted = False
        elif quoted:
            continue
        elif ch in ">)":
            depth += 1
        elif ch in "<(":
            depth -= 1
        elif ch == " " and depth <= 0:
            start = i + 1
            break
    name = head[start:].lstrip("*&")
    if head[:start].rstrip().endswith("operator"):
        name = "operator " + name
    if not name or name.startswith("__"):
        return None
    return name


def qualified_names(names: Iterable[str]) -> dict[str, str]:
    """mangled -> qualified name for every mangled name that decodes."""
    return {mangled: q for mangled, dem in demangle(names).items()
            if (q := qualified(dem))}


def strip_signature(name: str) -> str:
    """'army::GetName() const' -> 'army::GetName' (a pasted declaration)."""
    return _SIGNATURE.sub("", name.strip()).rstrip()


def bare(qualified_name: str) -> str:
    """The last scope component: 'game::GetTeam' -> 'GetTeam'."""
    depth = 0
    for i in range(len(qualified_name) - 1, 0, -1):
        ch = qualified_name[i]
        if ch == ">":
            depth += 1
        elif ch == "<":
            depth -= 1
        elif depth == 0 and qualified_name[i - 1:i + 1] == "::":
            return qualified_name[i + 1:]
    return qualified_name
