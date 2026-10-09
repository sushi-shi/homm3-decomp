"""Normalised callee identities shared by the Dreamcast inventory diffs.

Dreamcast roster/CodeView names, MSVC-decorated publics, Clang call names and
CodeWarrior-mangled Mac symbols spell one helper differently. `callee_key`
reduces each to its last source component (case and underscores ignored);
constructors reduce to their class and destructors to `~class`. The standard
library, C runtime, operator new/delete and compiler/runtime helpers return
None: they are platform plumbing, not game helper boundaries.
"""
from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Any, Iterable


@dataclass(frozen=True)
class Callee:
    key: str          # normalised last component; `~key` for a destructor
    kind: str         # "call", "constructor" or "destructor"
    display: str      # the readable last component, as first seen


# C runtime and libc-style helpers present in every port's runtime.
RUNTIME = frozenset("""
abs atan atan2 atof atoi atol bsearch calloc ceil clock cos exit exp fabs fclose
feof fflush fgetc fgets floor fmod fopen fprintf fputc fputs fread free fscanf
fseek ftell fwrite getc isalnum isalpha isdigit islower isspace isupper itoa labs
log log10 ltoa malloc memchr memcmp memcpy memmove memset pow printf putc qsort
rand realloc rewind sin sprintf sqrt srand sscanf strcat strchr strcmp strcmpi
strcpy strcspn stricmp strlen strlwr strncat strncmp strncpy strnicmp strrchr
strspn strstr strtok strtol strtoul strupr tan time tolower toupper vsprintf
min max swap
""".split())

_OPERATOR_NEW_DELETE = re.compile(r"operator\s*(?:new|delete)\b")
# CodeWarrior operator codes; nw/dl/nwa/dla are allocation plumbing.
_CW_ALLOCATION = {"nw": "operator new", "nwa": "operator new[]",
                  "dl": "operator delete", "dla": "operator delete[]"}
_CW_OPERATORS = {
    "as": "operator=", "eq": "operator==", "ne": "operator!=", "lt": "operator<",
    "gt": "operator>", "le": "operator<=", "ge": "operator>=", "vc": "operator[]",
    "cl": "operator()", "pl": "operator+", "mi": "operator-", "ml": "operator*",
    "dv": "operator/", "md": "operator%", "apl": "operator+=", "ami": "operator-=",
    "amu": "operator*=", "adv": "operator/=", "rf": "operator->", "pp": "operator++",
    "mm": "operator--", "nt": "operator!", "aa": "operator&&", "oo": "operator||",
    "ad": "operator&", "or": "operator|", "er": "operator^", "co": "operator~",
    "ls": "operator<<", "rs": "operator>>", "op": "operator",
}
_MSVC_OPERATORS = {
    "4": "operator=", "5": "operator>>", "6": "operator<<", "7": "operator!",
    "8": "operator==", "9": "operator!=", "A": "operator[]", "C": "operator->",
    "D": "operator*", "E": "operator++", "F": "operator--", "G": "operator-",
    "H": "operator+", "I": "operator&", "K": "operator/", "M": "operator<",
    "N": "operator<=", "O": "operator>", "P": "operator>=", "R": "operator()",
    "Y": "operator+=", "Z": "operator-=",
}
_HEX_SUFFIX = re.compile(r"_[0-9a-f]{5,8}$")


def _key(text: str) -> str:
    return text.replace("_", "").replace(" ", "").casefold()


def type_key(name: str) -> str:
    """The normalised unqualified class name of a type spelling."""
    return _key(_strip_template(split_scopes(name)[-1]))


def split_scopes(name: str) -> list[str]:
    """Top-level `::` components; template arguments stay intact."""
    parts: list[str] = []
    depth = start = 0
    index = 0
    while index < len(name):
        char = name[index]
        if char == "<":
            depth += 1
        elif char == ">" and depth:
            depth -= 1
        elif char == "(" and depth == 0:
            break  # a demangled signature tail
        elif name.startswith("::", index) and depth == 0:
            parts.append(name[start:index])
            start = index + 2
            index += 1
        index += 1
    parts.append(name[start:index])
    return [part.strip() for part in parts]


def _strip_template(text: str) -> str:
    return text.split("<", 1)[0].strip()


def _cw_class(text: str) -> tuple[str, str] | None:
    """Read one CodeWarrior class qualifier: `10heroWindow` or `Q23std6vector`."""
    match = re.match(r"Q(\d)", text)
    if match:
        rest, names = text[2:], []
        for _ in range(int(match[1])):
            item = re.match(r"(\d+)", rest)
            if not item:
                return None
            size = int(item[1])
            names.append(rest[len(item[1]):len(item[1]) + size])
            rest = rest[len(item[1]) + size:]
        return "::".join(names), rest
    item = re.match(r"(\d+)", text)
    if not item:
        return None
    size = int(item[1])
    return text[len(item[1]):len(item[1]) + size], text[len(item[1]) + size:]


def _codewarrior(name: str) -> str | None:
    """Demangle the qualified name of a CodeWarrior symbol (no signature)."""
    special = re.match(r"__([a-z]+)__(.*)$", name)
    if special:
        code, rest = special[1], special[2]
        owner = _cw_class(rest) if rest[:1].isdigit() or rest.startswith("Q") else None
        if code in ("ct", "dt"):
            if not owner:
                return None
            last = _strip_template(split_scopes(owner[0])[-1])
            return f"{owner[0]}::{'~' if code == 'dt' else ''}{last}"
        operator = _CW_ALLOCATION.get(code) or _CW_OPERATORS.get(code)
        if operator is None:
            return None
        return f"{owner[0]}::{operator}" if owner else operator
    match = re.match(r"([A-Za-z_]\w*?)__(?=\d|Q\d|C?F)(.*)$", name)
    if not match:
        return None
    method, rest = match[1], match[2]
    owner = _cw_class(rest) if rest[:1].isdigit() or rest.startswith("Q") else None
    return f"{owner[0]}::{method}" if owner else method


def _msvc(name: str) -> str | None:
    """The qualified source name of an MSVC-decorated public, or None."""
    if name.startswith(("??_", "??2", "??3")):
        return None  # new/delete, deleting destructors and other generated code
    match = re.match(r"\?\?([0-9A-Z])([^@]*)@([^@]*)", name)
    if match:
        code, first, second = match[1], match[2], match[3]
        if code == "0":
            return f"{first}::{first}"
        if code == "1":
            return f"{first}::~{first}"
        operator = _MSVC_OPERATORS.get(code)
        if operator is None:
            return None
        return f"{first}::{operator}" if first else operator
    match = re.match(r"\?([^@?]+)@((?:[^@]+@)*)@", name)
    if not match:
        return None
    scopes = [part for part in match[2].split("@") if part]
    return "::".join([*reversed(scopes), match[1]])


def qualified_name(name: str | None) -> str | None:
    """Best-effort qualified source spelling of any supported symbol form."""
    if not name:
        return None
    text = name.strip().lstrip(".")
    if text.startswith("?"):
        return _msvc(text)
    if "::" not in text and "__" in text:
        demangled = _codewarrior(text)
        if demangled is not None:
            return demangled
    return re.sub(r"@\d+$", "", text)  # __stdcall decoration `gzseek@12`


def callee(name: str | None) -> Callee | None:
    """A normalised game-helper identity, or None for plumbing/unknown."""
    qualified = qualified_name(name)
    if not qualified or qualified.startswith(("<", "mac:", "symbol:", "object:")):
        return None
    if _OPERATOR_NEW_DELETE.search(qualified):
        return None
    scopes = split_scopes(qualified)
    if scopes[0] in ("std", "_STL", "__std") or scopes[0].startswith(("std<", "_")):
        return None
    last = scopes[-1]
    bare = _strip_template(last)
    if not bare or (len(scopes) == 1 and (bare.startswith("_") or bare.casefold() in RUNTIME
                                          or (bare.startswith("mac_") and _HEX_SUFFIX.search(bare)))):
        return None
    if bare.startswith("~"):
        return Callee("~" + _key(bare[1:]), "destructor", bare)
    if len(scopes) >= 2 and _strip_template(scopes[-2]) == bare:
        return Callee(_key(bare), "constructor", bare)
    return Callee(_key(bare), "call", bare)


def inline_traces(groups: Iterable[dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    """Index Dreamcast inline-clue groups by the callee key of each candidate
    helper definition, so a call one side lacks can show its inline residue."""
    out: dict[str, list[dict[str, Any]]] = {}
    for group in groups:
        trace = {"source": group["source"], "definition_line": group["definition_line"],
                 "lines": list(group["source_lines"]), "dc_start": group["address"],
                 "dc_end": group["end_address"], "confidence": group["confidence"],
                 "definitions": list(group["definitions"])}
        for definition in group["definitions"]:
            identity = callee(definition)
            if identity is not None:
                out.setdefault(identity.key, []).append(trace)
    return out


def render_traces(traces: list[dict[str, Any]], limit: int = 3) -> list[str]:
    """One line per expanded definition: sites, lines and first DC extent."""
    grouped: dict[tuple, list[dict[str, Any]]] = {}
    for trace in traces:
        source = trace["source"].replace("\\", "/").rsplit("/", 1)[-1]
        grouped.setdefault((trace["confidence"], source, trace["definition_line"]), []).append(trace)
    out = []
    for (confidence, source, line), items in list(grouped.items())[:limit]:
        lines = sorted({value for item in items for value in item["lines"]})
        shown = ",".join(str(value) for value in lines[:8]) + (",..." if len(lines) > 8 else "")
        out.append(f"DC inline {confidence}: {source}:{line} at {len(items)} site(s), "
                   f"lines {shown}; first dc 0x{items[0]['dc_start']:x}..0x{items[0]['dc_end']:x}")
    if len(grouped) > limit:
        out.append(f"(+{len(grouped) - limit} more inline definition(s))")
    return out
