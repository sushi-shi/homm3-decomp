"""Named-constant index: integer value -> every name that spells it.

``homm3 constants`` answers "which named quantity is this literal?" by
joining three sources into one ``value -> [names]`` map:

* ``tree``    this checkout's src/ and include/ (vendor/ excluded):
              enumerators (plain, scoped, nested/member and H3_ENUM_*),
              integral ``const``/``static const`` objects, object-like
              numeric ``#define``s, and every declared array dimension,
              recorded as ``array length of NAME``;
* ``dc``      Dreamcast NB11 CodeView LF_ENUMERATE records with their
              original spellings (the authoritative source names);
* ``nh3api``  NH3API's enumerators and constants, when its checkout is
              found (``--nh3api PATH``, ``HOMM3_NH3API`` or a sibling
              ``homm3-symbols/NH3API``).

The tree scan is lexical: comments and string literals are removed,
scopes are tracked through braces, and values are evaluated from integer
literals, previously indexed names and C integer operators. Anything that
needs a cast, ``sizeof`` or a template is recorded as unresolved rather
than guessed. Nothing here compiles or rewrites source.

Queries
-------
  homm3 constants 83 0x9c         every name whose value is 83 or 156
  homm3 constants --name COUNT    names containing a substring
  homm3 constants --literals [--file F ...] [--min-score N] [--json]
        scan bare integer literals and rank the coupled ones first: a
        literal used as a bound, range check, size or template width that
        equals the length of an array referenced nearby (score 3); a
        literal array dimension repeated by another declaration of that
        array, or a bound equal to a count-like constant (score 2); any
        other literal with a candidate name scores 1. Annotation macros (VA, DATA, SIZE, ...)
        and the definitions themselves are not reported.

The index is cached under build/gen/cache, keyed on the content of every
scanned file, the Dreamcast executable and this implementation.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import asdict, dataclass, field
import hashlib
import json
import os
from pathlib import Path
import re
import sys

from homm3.core.cpp_tokens import tokens as _lex

CACHE_NAME = "constants-index.pickle"
ORIGINS = ("tree", "dc", "nh3api")
CACHE_VERSION = 1

#: Macros whose numeric arguments are addresses, sizes or offsets of the
#: retail image rather than program quantities.
ANNOTATIONS = frozenset({
    "VA", "VA_COMPGEN", "DATA", "DATA_COMPGEN", "DATA_COMPGEN_GUARD",
    "DC_ADDRESS", "MAC_ADDRESS", "MAC_COMPGEN_ADDRESS", "SIZE", "OFFSET",
    "static_assert", "HOMM3_STATIC_ASSERT",
})
COUNT_NAME = re.compile(
    r"(COUNT|NUM|MAX|CAPACITY|SIZE|LIMIT|TOTAL|LAST|LENGTH|kNum|kMax|Count|Num|Max|Total)")
BOUND_OPERATORS = frozenset({"<", "<=", ">", ">=", "==", "!="})
SIZE_CALLS = frozenset({"memset", "memcpy", "memmove", "memcmp", "MEMSET",
                        "MEMSET_LOCAL", "MEMCPY", "strncpy", "reserve", "resize"})
KEYWORDS = frozenset({
    "return", "case", "goto", "throw", "new", "delete", "sizeof", "if", "while",
    "for", "switch", "do", "else", "operator", "typedef", "using", "namespace",
})
INTEGRAL_WORDS = frozenset({
    "char", "short", "int", "long", "unsigned", "signed", "bool", "size_t",
    "__int8", "__int16", "__int32", "__int64", "BYTE", "WORD", "DWORD",
    "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t",
    "uint32_t", "uint64_t",
})
DECL_SPECIFIERS = frozenset({"static", "const", "extern", "constexpr", "volatile",
                             "inline", "mutable", "register"})


@dataclass(frozen=True)
class Entry:
    value: int | None
    name: str
    kind: str          # enumerator | const | define | array-length
    origin: str        # tree | dc | nh3api
    where: str         # file:line, or the DC enum record
    scope: str = ""    # enum or class qualification
    expression: str = ""

    @property
    def qualified(self) -> str:
        return f"{self.scope}::{self.name}" if self.scope else self.name

    def label(self) -> str:
        if self.kind == "array-length":
            return f"array length of {self.qualified}"
        return self.qualified


# --------------------------------------------------------------------------
# Lexing and evaluation


@dataclass
class _Tok:
    kind: str
    text: str
    line: int


def _code_tokens(source: str) -> list[_Tok]:
    """Code tokens with 1-based lines; comments, spaces and strings dropped."""
    result, line = [], 1
    for token in _lex(source):
        if token.kind in ("comment", "space"):
            line += token.text.count("\n")
            continue
        result.append(_Tok("string" if token.kind == "literal" else token.kind,
                           token.text, line))
        line += token.text.count("\n")
    return result


_INT = re.compile(r"^(0[xX][0-9a-fA-F]+|0[0-7]*|[1-9][0-9]*)([uUlL]*|i64|ui64)$")


def parse_int(text: str) -> int | None:
    match = _INT.match(text)
    if not match:
        return None
    digits = match.group(1)
    if digits.lower().startswith("0x"):
        return int(digits, 16)
    if len(digits) > 1 and digits.startswith("0"):
        return int(digits, 8)
    return int(digits)


_EVAL_OPS = {"+", "-", "*", "/", "%", "<<", ">>", "|", "&", "^", "~", "(", ")"}


def evaluate(expression: list[str], lookup) -> int | None:
    """Integer value of a C constant expression, or None when unsupported."""
    out = []
    for text in expression:
        number = parse_int(text)
        if number is not None:
            out.append(str(number))
        elif text in _EVAL_OPS:
            out.append("//" if text == "/" else text)
        elif re.match(r"^[A-Za-z_]\w*$", text):
            value = lookup(text)
            if value is None:
                return None
            out.append(f"({value})")
        elif text == "::":
            # A qualified enumerator: keep only its last component.
            if out and out[-1].startswith("("):
                out.pop()
            continue
        else:
            return None
    if not out:
        return None
    try:
        value = eval(" ".join(out), {"__builtins__": {}}, {})  # digits/operators only
    except (SyntaxError, ZeroDivisionError, TypeError, ValueError):
        return None
    return value if isinstance(value, int) else None


# --------------------------------------------------------------------------
# Tree scanning


@dataclass
class _Raw:
    name: str
    kind: str
    where: str
    scope: str
    expression: list[str]
    previous: "_Raw | None" = None   # implicit enumerator chain
    value: int | None = None
    resolved: bool = False


def _skip_balanced(toks: list[_Tok], at: int, open_: str, close: str) -> int:
    """Index after the bracket that closes toks[at] (which is `open_`)."""
    depth = 0
    while at < len(toks):
        if toks[at].text == open_:
            depth += 1
        elif toks[at].text == close:
            depth -= 1
            if depth == 0:
                return at + 1
        at += 1
    return at


def _enum_body(toks, at, scope, path, raws, end_texts=("}",)):
    """Parse enumerators from toks[at] (just after `{`). Returns end index."""
    previous = None
    while at < len(toks) and toks[at].text not in end_texts:
        tok = toks[at]
        if tok.kind != "word":
            at += 1
            continue
        if tok.text == "H3_ENUM_END" or tok.text == "H3_ENUM_END_SPLIT":
            break
        name, line = tok.text, tok.line
        at += 1
        expression: list[str] = []
        if at < len(toks) and toks[at].text == "=":
            at += 1
            depth = 0
            while at < len(toks):
                text = toks[at].text
                if depth == 0 and (text == "," or text in end_texts
                                   or text in ("H3_ENUM_END", "H3_ENUM_END_SPLIT")):
                    break
                depth += text in "([{"
                depth -= text in ")]}" and depth > 0
                expression.append(text)
                at += 1
        raw = _Raw(name, "enumerator", f"{path}:{line}", scope, expression,
                   None if expression else previous)
        if not expression and previous is None:
            raw.expression = ["0"]
        raws.append(raw)
        previous = raw
        if at < len(toks) and toks[at].text == ",":
            at += 1
    return at


def _declaration_start(toks, at) -> tuple[int, bool]:
    """Walk back from the declarator name at `at` over a type. Returns
    (index of the first type token, whether a declaration context holds)."""
    i = at - 1
    saw_type = False
    depth = 0
    while i >= 0:
        text = toks[i].text
        if text == ">":
            depth += 1
        elif text == "<" and depth:
            depth -= 1
        elif depth:
            pass
        elif toks[i].kind == "word":
            if text in KEYWORDS:
                return i, False
            saw_type = True
        elif text in ("*", "&", "::"):
            pass
        elif text in (";", "{", "}", "(", ",", ")", ":"):
            # `)`/`:` precede a declaration after an annotation macro or an
            # access specifier; `(`/`,` begin a parameter.
            return i + 1, saw_type
        else:
            return i, False
        i -= 1
    return 0, saw_type


def scan_source(source: str, path: str) -> list[_Raw]:
    """Lexically collect constant definitions from one source file."""
    toks = _code_tokens(source)
    raws: list[_Raw] = []
    scope_stack: list[str] = []      # names ("" for anonymous braces)
    pending_scope: str | None = None
    i = 0
    n = len(toks)
    line_start = True
    prev_line = 0
    while i < n:
        tok = toks[i]
        text = tok.text
        if tok.line != prev_line:
            line_start = True
            prev_line = tok.line
        # Preprocessor: object-like numeric #define.
        if text == "#" and line_start:
            j = i + 1
            if j < n and toks[j].text == "define" and j + 1 < n:
                name_tok = toks[j + 1]
                k = j + 2
                body = []
                # The directive ends with its physical line (continuations
                # are rare for numeric constants and simply end the body).
                function_like = (k < n and toks[k].text == "("
                                 and toks[k].line == name_tok.line
                                 and source_adjacent(source, name_tok, toks[k]))
                while k < n and toks[k].line == name_tok.line:
                    body.append(toks[k].text)
                    k += 1
                if not function_like and body and name_tok.kind == "word":
                    raws.append(_Raw(name_tok.text, "define", f"{path}:{name_tok.line}",
                                     "", body))
                i = k
                continue
            while j < n and toks[j].line == tok.line:
                j += 1
            i = j
            continue
        line_start = False
        if text in ("H3_ENUM_BEGIN", "H3_ENUM_BEGIN_SPLIT") and i + 2 < n and toks[i + 1].text == "(":
            name = toks[i + 2].text
            close = _skip_balanced(toks, i + 1, "(", ")")
            scope = "::".join([s for s in scope_stack if s] + [name])
            i = _enum_body(toks, close, scope, path, raws, end_texts=())
            continue
        if text == "enum":
            j = i + 1
            if j < n and toks[j].text in ("class", "struct"):
                j += 1
            name = ""
            if j < n and toks[j].kind == "word":
                name = toks[j].text
                j += 1
            if j < n and toks[j].text == ":":
                while j < n and toks[j].text not in ("{", ";"):
                    j += 1
            if j < n and toks[j].text == "{":
                parts = [s for s in scope_stack if s] + ([name] if name else [])
                end = _enum_body(toks, j + 1, "::".join(parts), path, raws)
                i = end + 1
                continue
            i = j
            continue
        if text in ("class", "struct", "union", "namespace") and i + 1 < n:
            # The scope name is the last word before the body's `{` or a
            # base clause (`class DLLEXPORT Name : public Base {`).
            k, name = i + 1, ""
            while k < n and toks[k].text not in ("{", ";", "(", ")", "=", ">", ",", ":"):
                if toks[k].kind == "word":
                    name = toks[k].text
                k += 1
            if k < n and toks[k].text == ":" and text != "namespace":
                while k < n and toks[k].text not in ("{", ";"):
                    k += 1
            if k < n and toks[k].text == "{":
                pending_scope = name
                i = k
                continue
            i += 1
            continue
        if text == "{":
            scope_stack.append(pending_scope or "")
            pending_scope = None
            i += 1
            continue
        if text == "}":
            if scope_stack:
                scope_stack.pop()
            i += 1
            continue
        pending_scope = None if text == ";" else pending_scope
        scope = "::".join(s for s in scope_stack if s)
        # Integral const object: [static] const <integral...> NAME = expr ;
        if text == "const" or text == "constexpr":
            j = i + 1
            saw_integral = False
            while j < n and (toks[j].text in INTEGRAL_WORDS or toks[j].text in DECL_SPECIFIERS):
                saw_integral |= toks[j].text in INTEGRAL_WORDS
                j += 1
            if saw_integral and j + 1 < n and toks[j].kind == "word" and toks[j + 1].text == "=":
                k = j + 2
                expression = []
                depth = 0
                while k < n and not (depth == 0 and toks[k].text in (";", ",")):
                    depth += toks[k].text in "([{"
                    depth -= toks[k].text in ")]}"
                    expression.append(toks[k].text)
                    k += 1
                raws.append(_Raw(toks[j].text, "const", f"{path}:{toks[j].line}",
                                 scope, expression))
                i = k
                continue
        # Array declarator: NAME [ expr ] following a type.
        if tok.kind == "word" and i + 1 < n and toks[i + 1].text == "[" and text not in KEYWORDS:
            start, ok = _declaration_start(toks, i)
            if ok and start < i:
                k = i + 1
                dims = []
                while k < n and toks[k].text == "[":
                    close = _skip_balanced(toks, k, "[", "]")
                    dims.append([t.text for t in toks[k + 1:close - 1]])
                    k = close
                follower = toks[k].text if k < n else ";"
                if follower in (";", "=", ",", ")", "{", ":"):
                    for index, dim in enumerate(dims):
                        if not dim:
                            continue
                        label = text if len(dims) == 1 else f"{text}[dim {index + 1}]"
                        raws.append(_Raw(label, "array-length", f"{path}:{tok.line}",
                                         scope, dim))
                    i = k
                    continue
        i += 1
    return raws


def source_adjacent(source: str, a: _Tok, b: _Tok) -> bool:
    """True when token b directly follows a on its line (function-like macro)."""
    for line in source.splitlines()[a.line - 1:a.line]:
        return re.search(re.escape(a.text) + r"\(", line) is not None
    return False


def resolve(raws: list[_Raw]) -> None:
    """Evaluate every expression to a fixpoint over the collected names."""
    by_name: dict[str, set[int]] = defaultdict(set)

    def lookup(name):
        values = by_name.get(name)
        return next(iter(values)) if values and len(values) == 1 else None

    for _ in range(12):
        progress = False
        for raw in raws:
            if raw.resolved:
                continue
            if raw.previous is not None:
                if raw.previous.resolved:
                    raw.value, raw.resolved = raw.previous.value + 1, True
            else:
                value = evaluate(raw.expression, lookup)
                if value is not None:
                    raw.value, raw.resolved = value, True
            if raw.resolved:
                progress = True
                if raw.kind != "array-length":
                    by_name[raw.name].add(raw.value)
        if not progress:
            break


def scan_tree(root: Path, directories=("include", "src")) -> list[Entry]:
    raws = []
    for path in tree_files(root, directories):
        relative = path.relative_to(root).as_posix()
        raws.extend(scan_source(path.read_text(encoding="latin-1"), relative))
    resolve(raws)
    return [Entry(r.value if r.resolved else None, r.name, r.kind, "tree", r.where,
                  r.scope, " ".join(r.expression)) for r in raws]


def tree_files(root: Path, directories=("include", "src")) -> list[Path]:
    files = []
    for directory in directories:
        base = root / directory
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix.lower() in (".h", ".hpp", ".c", ".cpp", ".inl") and path.is_file():
                if "vendor" in path.relative_to(root).parts or "zlib" in path.name:
                    continue
                files.append(path)
    return files


# --------------------------------------------------------------------------
# Dreamcast and NH3API


def scan_dreamcast(symbols) -> list[Entry]:
    from homm3.core.nb11_types import Types
    types = Types.from_symbols(symbols)
    entries, seen = [], set()
    for index in sorted(symbols.type_records):
        try:
            record = types.get(index)
            if record.get("kind") != "enum" or record.get("forward"):
                continue
            fields = types.get(record["fields"])
        except Exception:  # an unparsable record is simply not indexed
            continue
        name = record.get("name", "")
        for item in fields.get("entries", []):
            if item.get("kind") != "enumerator":
                continue
            key = (name, item["name"], item["value"])
            if key in seen:
                continue
            seen.add(key)
            entries.append(Entry(int(item["value"]), item["name"], "enumerator", "dc",
                                 f"NB11 LF_ENUM 0x{index:04x}", name))
    return entries


def nh3api_root(explicit: str | None = None) -> Path | None:
    candidates = [explicit, os.environ.get("HOMM3_NH3API")]
    from homm3.core import common
    # A sibling homm3-symbols checkout, from this tree or any linked worktree.
    candidates += [str(parent / "homm3-symbols/NH3API") for parent in common.HOMM3_DIR.parents]
    for candidate in candidates:
        if candidate and Path(candidate).is_dir():
            return Path(candidate)
    return None


def scan_nh3api(root: Path) -> list[Entry]:
    raws = []
    for path in sorted(root.rglob("*.hpp")):
        relative = path.relative_to(root).as_posix()
        raws.extend(scan_source(path.read_text(encoding="utf-8", errors="replace"),
                                relative))
    resolve(raws)
    return [Entry(r.value if r.resolved else None, r.name, r.kind, "nh3api",
                  f"NH3API/{r.where}", r.scope, " ".join(r.expression))
            for r in raws if r.kind != "array-length"]


# --------------------------------------------------------------------------
# Index


@dataclass
class Index:
    entries: list[Entry] = field(default_factory=list)

    def __post_init__(self):
        self.by_value: dict[int, list[Entry]] = defaultdict(list)
        for entry in self.entries:
            if entry.value is not None:
                self.by_value[entry.value].append(entry)

    def values(self, value: int, origins=None) -> list[Entry]:
        return [e for e in self.by_value.get(value, []) if not origins or e.origin in origins]

    def names(self, needle: str, origins=None) -> list[Entry]:
        needle = needle.lower()
        return [e for e in self.entries
                if needle in e.qualified.lower() and (not origins or e.origin in origins)]

    def array_lengths(self) -> dict[str, set[int]]:
        result: dict[str, set[int]] = defaultdict(set)
        for entry in self.entries:
            if entry.kind == "array-length" and entry.value is not None:
                result[entry.name.split("[", 1)[0]].add(entry.value)
        return result


def _fingerprint(root: Path, nh3api: Path | None, dreamcast: bool) -> str:
    digest = hashlib.sha256()
    digest.update(Path(__file__).read_bytes())
    digest.update(str(CACHE_VERSION).encode())
    for path in tree_files(root):
        digest.update(path.relative_to(root).as_posix().encode())
        digest.update(hashlib.sha256(path.read_bytes()).digest())
    if nh3api is not None:
        for path in sorted(nh3api.rglob("*.hpp")):
            digest.update(hashlib.sha256(path.read_bytes()).digest())
    if dreamcast:
        # The staged Dreamcast executable is verified against this pin.
        from homm3.core import inputs
        digest.update(f"dc:{inputs.DREAMCAST.sha256}:{inputs.is_staged(inputs.DREAMCAST)}".encode())
    return digest.hexdigest()


def build_index(root: Path, *, nh3api: Path | None = None, dreamcast: bool = True,
                use_cache: bool = True) -> Index:
    from homm3.core import content_cache
    key = _fingerprint(root, nh3api, dreamcast)
    if use_cache:
        cached = content_cache.load(CACHE_NAME, root)
        if cached.get("key") == key:
            return Index([Entry(**row) for row in cached["entries"]])
    entries = scan_tree(root)
    if dreamcast:
        try:
            from homm3.core import inputs
            entries += scan_dreamcast(inputs.dreamcast_symbols())
        except Exception as exc:  # no staged Dreamcast executable
            print(f"[constants] Dreamcast enumerators unavailable: {exc}", file=sys.stderr)
    if nh3api is not None:
        entries += scan_nh3api(nh3api)
    if use_cache:
        content_cache.store(CACHE_NAME, {"key": key,
                                         "entries": [asdict(e) for e in entries]}, root)
    return Index(entries)


# --------------------------------------------------------------------------
# Literal scan


@dataclass
class Literal:
    path: str
    line: int
    text: str
    value: int
    context: str          # bound | size | dimension | template | index | value
    score: int
    arrays: list[str]     # same-length arrays referenced nearby
    candidates: list[Entry]
    source: str


def _context(toks: list[_Tok], at: int) -> str:
    before = toks[at - 1].text if at else ""
    after = toks[at + 1].text if at + 1 < len(toks) else ""
    if before == "[" and after == "]":
        # A declarator's length, versus an element index such as
        # `g_heroScreen[7]` or `m_armies[7] = 0`.
        i = at - 1
        while i > 0 and toks[i].text == "[" and toks[i - 1].text == "]":
            i = _matching_open(toks, i - 1)
        name = i - 1
        if name >= 0 and toks[name].kind == "word":
            start, declared = _declaration_start(toks, name)
            close = at + 1
            while close + 1 < len(toks) and toks[close + 1].text == "[":
                close = _skip_balanced(toks, close + 1, "[", "]") - 1
            follower = toks[close + 1].text if close + 1 < len(toks) else ";"
            if declared and start < name and follower in (";", "=", ",", ")", "{", ":"):
                return "dimension"
        return "index"
    if before == "<" and after in (">", ","):
        # `bitset<144>` versus `i < 144`: a template width is followed by
        # `>`, a comparison by `;`/`)`/an operator.
        if at >= 2 and toks[at - 2].kind == "word" and after == ">":
            return "template"
    if before in BOUND_OPERATORS or after in BOUND_OPERATORS - {"<", ">"}:
        return "bound"
    if before == "<" or (after in ("<=", ">=") ):
        return "bound"
    # A size argument: memset(p, 0, 144) / reserve(144).
    depth, i = 0, at - 1
    while i >= 0 and i >= at - 12:
        text = toks[i].text
        if text == ")":
            depth += 1
        elif text == "(":
            if depth == 0:
                if i and toks[i - 1].text in SIZE_CALLS:
                    return "size"
                break
            depth -= 1
        elif text in (";", "{", "}"):
            break
        i -= 1
    return "value"


def _declared_name(toks: list[_Tok], at: int) -> str:
    """The array name whose `[N]` holds the literal at `at`."""
    i = at - 1
    while i > 0 and toks[i].text == "[" and toks[i - 1].text == "]":
        i = _matching_open(toks, i - 1) - 1
    i -= 1
    return toks[i].text if i >= 0 and toks[i].kind == "word" else ""


def _matching_open(toks: list[_Tok], close: int) -> int:
    depth = 0
    for i in range(close, -1, -1):
        if toks[i].text == "]":
            depth += 1
        elif toks[i].text == "[":
            depth -= 1
            if depth == 0:
                return i
    return 0


def _skipped_ranges(toks: list[_Tok]) -> set[int]:
    """Token indices inside annotation macros, #directives other than
    #define bodies, enum bodies and const definitions."""
    skip = set()
    i, n = 0, len(toks)
    while i < n:
        text = toks[i].text
        if text in ANNOTATIONS and i + 1 < n and toks[i + 1].text == "(":
            end = _skip_balanced(toks, i + 1, "(", ")")
            skip.update(range(i, end))
            i = end
            continue
        if text == "#":
            j = i
            while j < n and toks[j].line == toks[i].line:
                j += 1
            skip.update(range(i, j))   # #define bodies are definitions
            i = j
            continue
        if text == "enum":
            j = i + 1
            while j < n and toks[j].text not in ("{", ";"):
                j += 1
            if j < n and toks[j].text == "{":
                end = _skip_balanced(toks, j, "{", "}")
                skip.update(range(i, end))
                i = end
                continue
        if text in ("H3_ENUM_BEGIN", "H3_ENUM_BEGIN_SPLIT"):
            j = i
            while j < n and toks[j].text not in ("H3_ENUM_END", "H3_ENUM_END_SPLIT"):
                j += 1
            skip.update(range(i, j))
            i = j
            continue
        if text in ("const", "constexpr"):
            j = i + 1
            while j < n and (toks[j].text in INTEGRAL_WORDS or toks[j].text in DECL_SPECIFIERS):
                j += 1
            if j + 1 < n and toks[j].kind == "word" and toks[j + 1].text == "=" and j > i + 1:
                k = j + 2
                while k < n and toks[k].text != ";":
                    k += 1
                skip.update(range(i, k))
                i = k
                continue
        i += 1
    return skip


def scan_literals(index: Index, root: Path, files: list[Path], *, minimum: int = 3,
                  window: int = 25) -> list[Literal]:
    arrays = index.array_lengths()
    by_length: dict[int, set[str]] = defaultdict(set)
    for name, lengths in arrays.items():
        for length in lengths:
            by_length[length].add(name)
    literal_dims: dict[tuple[str, int], list[tuple[str, int]]] = defaultdict(list)
    for entry in index.entries:
        if entry.kind == "array-length" and entry.origin == "tree" \
                and parse_int(entry.expression) is not None and entry.value is not None:
            path, _, line = entry.where.rpartition(":")
            literal_dims[(entry.name, entry.value)].append((path, int(line)))
    results = []
    for path in files:
        source = path.read_text(encoding="latin-1")
        lines = source.split("\n")
        toks = _code_tokens(source)
        skip = _skipped_ranges(toks)
        words_by_line: dict[int, set[str]] = defaultdict(set)
        for tok in toks:
            if tok.kind == "word":
                words_by_line[tok.line].add(tok.text)
        relative = path.relative_to(root).as_posix()
        for at, tok in enumerate(toks):
            if tok.kind != "number" or at in skip:
                continue
            value = parse_int(tok.text)
            if value is None or value < minimum:
                continue
            context = _context(toks, at)
            candidates = [e for e in index.values(value) if e.kind != "array-length"]
            near: set[str] = set()
            for line in range(tok.line - window, tok.line + window + 1):
                near |= words_by_line.get(line, set())
            same_length = sorted(name for name in by_length.get(value, ()) if name in near)
            counts = [e for e in candidates if COUNT_NAME.search(e.name)]
            if context == "dimension":
                # A table length spelled as a literal: coupled when another
                # declaration of the same array repeats it (an extern and
                # its definition, or copies across TUs).
                own = _declared_name(toks, at)
                repeats = [w for w in literal_dims.get((own, value), ()) if w != (relative, tok.line)]
                same_length = [own] if repeats else []
            if context in ("index", "value"):
                score = 1 if candidates else 0
                if not score:
                    continue
            elif context in ("bound", "size", "template") and same_length:
                score = 3
            elif context != "value" and counts:
                score = 2
            elif candidates or same_length:
                score = 1
            else:
                continue
            candidates.sort(key=lambda e: (not COUNT_NAME.search(e.name),
                                           ORIGINS.index(e.origin), e.qualified))
            results.append(Literal(relative, tok.line, tok.text, value, context, score,
                                   same_length, candidates,
                                   lines[tok.line - 1].strip() if tok.line <= len(lines) else ""))
    results.sort(key=lambda r: (-r.score, r.path, r.line))
    return results


# --------------------------------------------------------------------------
# CLI


def _format_entry(entry: Entry) -> str:
    value = "?" if entry.value is None else str(entry.value)
    expression = f"  = {entry.expression}" if entry.expression and entry.origin == "tree" \
        and entry.expression != value else ""
    return f"  {value:>8}  {entry.origin:<6} {entry.kind:<12} {entry.label():<48} {entry.where}{expression}"


def _parse_value(text: str) -> int:
    value = parse_int(text.lstrip("-"))
    if value is None:
        raise argparse.ArgumentTypeError(f"not an integer literal: {text}")
    return -value if text.startswith("-") else value


def main(argv=None) -> int:
    from homm3.core import common
    parser = argparse.ArgumentParser(
        prog="homm3 constants", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("values", nargs="*", type=_parse_value,
                        help="integer values (decimal, hex or octal)")
    parser.add_argument("--name", action="append", default=[],
                        help="case-insensitive name substring (repeatable)")
    parser.add_argument("--literals", action="store_true",
                        help="scan bare integer literals for candidate names")
    parser.add_argument("--file", action="append", default=[],
                        help="limit --literals to these files or directories")
    parser.add_argument("--min-score", type=int, default=2,
                        help="--literals: lowest score shown (default 2)")
    parser.add_argument("--min-value", type=int, default=3,
                        help="--literals: ignore smaller literals (default 3)")
    parser.add_argument("--origin", action="append", choices=("tree", "dc", "nh3api"),
                        help="restrict candidate origins (repeatable)")
    parser.add_argument("--nh3api", metavar="PATH", help="NH3API checkout")
    parser.add_argument("--no-dreamcast", action="store_true",
                        help="skip Dreamcast enumerators")
    parser.add_argument("--no-cache", action="store_true", help="rebuild the index")
    parser.add_argument("--limit", type=int, default=0,
                        help="maximum rows per query (0 = all)")
    parser.add_argument("--all-names", action="store_true",
                        help="--literals: print every candidate, not the first six")
    parser.add_argument("--json", action="store_true", help="machine-readable output")
    args = parser.parse_args(argv)

    root = common.HOMM3_DIR
    index = build_index(root, nh3api=nh3api_root(args.nh3api),
                        dreamcast=not args.no_dreamcast, use_cache=not args.no_cache)
    origins = set(args.origin) if args.origin else None
    payload: dict = {}

    def clip(rows):
        return rows[:args.limit] if args.limit else rows

    for value in args.values:
        rows = sorted(index.values(value, origins),
                      key=lambda e: (ORIGINS.index(e.origin), e.kind, e.qualified))
        payload[str(value)] = clip(rows)
    for needle in args.name:
        rows = sorted(index.names(needle, origins), key=lambda e: (e.origin, e.qualified))
        payload[f"name:{needle}"] = clip(rows)
    literals = None
    if args.literals:
        files = tree_files(root)
        if args.file:
            wanted = [(root / f).resolve() for f in args.file]
            files = [p for p in files
                     if any(p.resolve() == w or w in p.resolve().parents for w in wanted)]
        literals = [r for r in scan_literals(index, root, files, minimum=args.min_value)
                    if r.score >= args.min_score]
        if origins:
            for row in literals:
                row.candidates = [e for e in row.candidates if e.origin in origins]
        literals = clip(literals)
    if not args.values and not args.name and literals is None:
        parser.print_help()
        return 0

    if args.json:
        out = {key: [asdict(e) | {"label": e.label()} for e in rows]
               for key, rows in payload.items()}
        if literals is not None:
            out["literals"] = [{**{k: v for k, v in asdict(r).items() if k != "candidates"},
                                "candidates": [e.label() + f" ({e.origin})" for e in r.candidates]}
                               for r in literals]
        json.dump(out, sys.stdout, indent=1)
        print()
        return 0
    for key, rows in payload.items():
        title = key if key.startswith("name:") else f"{key} (0x{int(key) & 0xffffffff:x})"
        print(f"{title}: {len(rows)} name(s)")
        for entry in rows:
            print(_format_entry(entry))
    if literals is not None:
        print(f"literals: {len(literals)} (score >= {args.min_score})")
        for row in literals:
            labels = list(dict.fromkeys(f"{e.label()}" + ("" if e.origin == "tree" else f" ({e.origin})")
                                        for e in row.candidates))
            shown = labels if args.all_names else labels[:6]
            names = ", ".join(shown) + (f", +{len(labels) - len(shown)} more"
                                        if len(labels) > len(shown) else "")
            arrays = f" arrays: {', '.join(row.arrays)};" if row.arrays else ""
            print(f"  [{row.score}] {row.path}:{row.line} {row.text} {row.context};"
                  f"{arrays} names: {names or '-'}")
            print(f"        {row.source[:140]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
