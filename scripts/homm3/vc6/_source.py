"""homm3.vc6._source - locate a function's DEFINITION BODY in a C++ TU.

Shared support module for reg_model (`why-reg`) and flow_model
(`why-branch`): both solvers mutate the target function's body text and let
the pinned compiler judge, so both need the same answer to one question -
"where, in this .cpp, is the body of the function the .obj calls F?".

v1 answered it with `re.finditer(rf"\\b{fn}\\s*\\(")` over the raw text plus a
"skip an optional `const`, then demand `{`" tail. Two defects, both measured
2026-08-14 on src/bottomviewsubwindow.cpp:

  1. NAME.  flow_model searched the MANGLED symbol verbatim (never present in
     source); reg_model's demangler handled only `?name@Class@@...`, so every
     `??0`/`??1`/`??_G`/operator special name decoded to garbage
     (`??0TBottomViewKingdom@@QAE@PAVheroWindow@@@Z` -> `QAE::?0TBottom...`).
     Result: the tree's ENTIRE constructor + destructor + operator population
     was outside both solvers - exactly where the campaign's plateaus sit.
  2. CARCASS.  The search ran over raw text, so it hit whichever definition
     came FIRST - and this tree fences unreconstructed functions in
     `#if 0  // @carcass` blocks near the top of the file, where each is a
     `{ /* @stub */ }`. `sum_mobility` located to line 155 (the stub), not
     line 1269 (the real body); `animate` to line 100, not 1299. Mutating a
     stub is a NO-OP, so the solver reported "nothing helped" - a silently
     WRONG capped verdict, not a missing one. 102 of the tree's source files
     carry a carcass block.

The locator therefore works over a MASKED copy of the TU (same length, so
every index is an index into the original): comments, string/char literals
and inactive `#if 0` regions become spaces, and every preprocessor directive
line is blanked so a brace-carrying `#define` cannot unbalance the scan.
Then, per candidate source name, it walks the definition grammar:

    NAME ( params )  [const|volatile|throw(..)|__decl..]  [: a(x), b(y)]  {

which accepts qualified member definitions, constructors WITH a
member-initialiser list, destructors, operators and cv/exception/calling-
convention suffixes.  Anything that is not followed by a body (a
declaration, a call site like `new Foo(...)`) is skipped, so the locator
still refuses what it should refuse.

Deliberate v1 limits (documented, not silent): template-id scopes
(`?$vector@...`) are not demangled - such rows are header-defined and
correctly report "not located"; `#if FOO` / `#ifdef FOO` are treated as
ACTIVE (only the literal `#if 0` / `#if 1`-`#else` idiom is killed), so a
definition fenced behind a real feature macro can still be found.
"""
from __future__ import annotations

import re
from typing import NamedTuple

# --- MSVC special names (the `??X` operator codes) -----------------------------------
#
# Codes that name a real, source-writable entity.  `?0`/`?1` are handled
# separately: their source spelling is derived from the class name.
_OPERATORS = {
    "2": "operator new",   "3": "operator delete", "4": "operator=",
    "5": "operator>>",     "6": "operator<<",      "7": "operator!",
    "8": "operator==",     "9": "operator!=",      "A": "operator[]",
    "C": "operator->",     "D": "operator*",       "E": "operator++",
    "F": "operator--",     "G": "operator-",       "H": "operator+",
    "I": "operator&",      "J": "operator->*",     "K": "operator/",
    "L": "operator%",      "M": "operator<",       "N": "operator<=",
    "O": "operator>",      "P": "operator>=",      "Q": "operator,",
    "R": "operator()",     "S": "operator~",       "T": "operator^",
    "U": "operator|",      "V": "operator&&",      "W": "operator||",
    "X": "operator*=",     "Y": "operator+=",      "Z": "operator-=",
    "_0": "operator/=",    "_1": "operator%=",     "_2": "operator>>=",
    "_3": "operator<<=",   "_4": "operator&=",     "_5": "operator|=",
    "_6": "operator^=",    "_U": "operator new[]", "_V": "operator delete[]",
}

# Codes for entities the COMPILER synthesises: there is no source body to
# locate, and saying so is a measurement, not a failure.  (In this tree they
# are emitted by the VA_COMPGEN macro, which has no braces at all.)
_COMPILER_GENERATED = {
    "_7": "vftable",
    "_8": "vbtable",
    "_9": "vcall thunk",
    "_A": "typeof",
    "_B": "local static guard",
    "_C": "string literal",
    "_D": "vbase destructor",
    "_E": "vector deleting destructor",
    "_F": "default constructor closure",
    "_G": "scalar deleting destructor",
    "_H": "vector constructor iterator",
    "_I": "vector destructor iterator",
    "_J": "vector vbase constructor iterator",
    "_K": "virtual displacement map",
    "_L": "eh vector constructor iterator",
    "_M": "eh vector destructor iterator",
    "_N": "eh vector vbase constructor iterator",
    "_O": "copy constructor closure",
    "_S": "local vftable",
    "_T": "local vftable constructor closure",
}

# `B` is `operator <type>()`: source-writable but its spelling is the TYPE,
# which needs a full type demangler.  Treated as unsupported, not generated.
_UNSUPPORTED_OPS = {"B"}


class Mangled(NamedTuple):
    """What a decoded MSVC symbol tells us about its SOURCE spelling."""
    base: str | None            # source basename (`~Foo`, `operator=`, `bar`)
    scope: list[str]            # class/namespace chain, innermost FIRST
    compgen: str | None         # set => compiler-generated, no source body
    note: str | None            # set => could not decode (reason)


def _scope_chain(s: str, i: int) -> tuple[list[str], str | None]:
    """Parse the `@`-separated qualification chain (innermost first) that
    ends at the `@@` terminator.  Returns (tokens, note-if-undecodable)."""
    toks: list[str] = []
    while i < len(s):
        if s[i] == "@":                       # the `@@` terminator
            return toks, None
        j = s.find("@", i)
        if j < 0:
            return toks, "unterminated qualification chain"
        tok = s[i:j]
        if tok.startswith("?$"):
            return toks, "template-id scope (no type demangler in v1)"
        if tok.isdigit():
            return toks, "back-referenced name (no name table in v1)"
        toks.append(tok)
        i = j + 1
    return toks, "unterminated qualification chain"


def demangle(fn: str) -> Mangled:
    """Decode as much of an MSVC decorated name as the locator needs.

    Not a general demangler: it recovers the SOURCE-VISIBLE spelling (base
    name + qualification chain) and stops - parameter and return types are
    irrelevant to finding a definition, and decoding them is where a
    hand-rolled demangler goes wrong.  A plain (undecorated) `fn` is
    returned as its own basename, which is the probe / free-function case.
    """
    if not fn.startswith("?"):
        return Mangled(fn, [], None, None)
    if fn.startswith("??"):
        rest = fn[2:]
        if not rest:
            return Mangled(None, [], None, "truncated special name")
        code = rest[:2] if rest[0] == "_" else rest[:1]
        rest = rest[len(code):]
        scope, note = _scope_chain(rest, 0)
        if code in _COMPILER_GENERATED:
            return Mangled(None, scope, _COMPILER_GENERATED[code], None)
        if note:
            return Mangled(None, scope, None, note)
        if code in ("0", "1"):                # constructor / destructor
            if not scope:
                return Mangled(None, [], None, "ctor/dtor without a class")
            base = scope[0] if code == "0" else "~" + scope[0]
            return Mangled(base, scope, None, None)
        if code in _OPERATORS:
            return Mangled(_OPERATORS[code], scope, None, None)
        if code in _UNSUPPORTED_OPS:
            return Mangled(None, scope, None,
                           "conversion operator (needs a type demangler)")
        return Mangled(None, scope, None, f"unknown special-name code ?{code}")
    at = fn.find("@", 1)
    if at < 0:
        return Mangled(fn[1:], [], None, None)
    name = fn[1:at]
    if name.startswith("?$"):
        return Mangled(None, [], None,
                       "template-id function (no type demangler in v1)")
    scope, note = _scope_chain(fn, at + 1)
    if note:
        return Mangled(name, scope, None, note)
    return Mangled(name, scope, None, None)


def source_names(fn: str) -> list[str]:
    """Source-level identifiers to look for, MOST QUALIFIED FIRST.

    `??0TBottomViewKingdom@@QAE@PAVheroWindow@@@Z`
        -> ["TBottomViewKingdom::TBottomViewKingdom", "TBottomViewKingdom"]
    `?animate@TBottomViewNewTurn@@UAEXXZ`
        -> ["TBottomViewNewTurn::animate", "animate"]
    `??1type_bottom_view_window@@UAE@XZ`
        -> ["type_bottom_view_window::~type_bottom_view_window",
            "~type_bottom_view_window"]
    """
    m = demangle(fn)
    if m.base is None:
        return []
    out = []
    if m.scope:
        full = "::".join(reversed(m.scope)) + "::" + m.base
        out.append(full)
        nearest = m.scope[0] + "::" + m.base
        if nearest != full:
            out.append(nearest)
    out.append(m.base)
    return out


# --- masking: comments, literals and inactive preprocessor regions -------------------

def _mask_lex(text: str) -> str:
    """Blank comments and string/char literals to spaces, length-preserving
    (newlines survive so the directive pass stays line-addressable)."""
    out = list(text)
    n = len(text)

    def blank(a: int, b: int) -> None:
        for k in range(a, b):
            if out[k] != "\n":
                out[k] = " "

    i = 0
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "/" and nxt == "/":
            j = text.find("\n", i)
            j = n if j < 0 else j
            blank(i, j)
            i = j
        elif c == "/" and nxt == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            blank(i, j)
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == c:
                    j += 1
                    break
                if text[j] == "\n":       # unterminated: not a literal
                    break
                j += 1
            blank(i, min(j, n))
            i = min(j, n)
        else:
            i += 1
    return "".join(out)


_DIRECTIVE = re.compile(r"^[ \t]*#[ \t]*(\w+)[ \t]*(.*)$")


def _mask_preprocessor(masked: str) -> str:
    """Blank every directive line, and every line inside a dead `#if 0`
    (or the `#else` of an `#if 1`) branch.

    Deliberately conservative: only the LITERAL constant conditions are
    evaluated.  `#ifdef FOO` and `#if FOO` stay ACTIVE, because guessing a
    macro's value would hide a real definition - the failure mode we are
    fixing is the opposite one (finding a fenced stub), and this tree fences
    with the literal `#if 0  // @carcass` idiom.
    """
    lines = masked.split("\n")
    out: list[str] = []
    stack: list[dict] = []      # {"kill": bool, "literal": 0|1|None}
    cont = False                # previous line was a directive ending in `\`
    for ln in lines:
        dead = any(lv["kill"] for lv in stack)
        m = None if cont else _DIRECTIVE.match(ln)
        if m:
            name, rest = m.group(1), m.group(2).strip()
            if name in ("if", "ifdef", "ifndef"):
                lit = None
                if name == "if" and rest in ("0", "1"):
                    lit = int(rest)
                stack.append({"kill": dead or lit == 0, "literal": lit})
            elif name == "else" and stack:
                lv = stack[-1]
                outer = any(x["kill"] for x in stack[:-1])
                if lv["literal"] == 0:
                    lv["kill"] = outer or False
                elif lv["literal"] == 1:
                    lv["kill"] = True
                else:
                    lv["kill"] = outer
            elif name == "elif" and stack:
                lv = stack[-1]
                lv["literal"] = None
                lv["kill"] = any(x["kill"] for x in stack[:-1])
            elif name == "endif" and stack:
                stack.pop()
        blanked = m is not None or cont or dead
        out.append(" " * len(ln) if blanked else ln)
        cont = (m is not None or cont) and ln.rstrip().endswith("\\")
    return "\n".join(out)


def mask(text: str) -> str:
    """The searchable view of a TU: same length as `text`, with comments,
    literals, directive lines and dead `#if 0` regions replaced by spaces."""
    return _mask_preprocessor(_mask_lex(text))


# --- the definition grammar walk ----------------------------------------------------

class Definition(NamedTuple):
    name: str                   # the source identifier that matched
    head: int                   # index of the identifier's first char
    par_open: int               # index of the parameter list's `(`
    par_close: int              # index of its `)`
    init: int | None            # index of the member-initialiser `:`, if any
    body_open: int              # index of the body's `{`
    body_close: int             # index of the matching `}`


_SUFFIX_WORD = re.compile(r"(const|volatile|throw|__cdecl|__stdcall|"
                          r"__fastcall|__thiscall|__declspec)\b")


def _skip_ws(s: str, i: int) -> int:
    while i < len(s) and s[i].isspace():
        i += 1
    return i


def _match_paren(s: str, i: int, opener: str = "(", closer: str = ")"):
    """Index of the delimiter matching s[i], or None."""
    depth = 0
    while i < len(s):
        if s[i] == opener:
            depth += 1
        elif s[i] == closer:
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return None


def _skip_init_list(s: str, i: int):
    """From the `:` that opens a member-initialiser list, return the index of
    the body's `{`, or None if this is not an init list after all.

    Balanced `(...)`/`[...]` are stepped over so an initialiser argument
    cannot be mistaken for the body; a `;` or `)` at depth 0 means we were
    looking at something that is not a definition (a bitfield, a label, the
    `?:` of a default argument) and the candidate is rejected.
    """
    i += 1
    while i < len(s):
        c = s[i]
        if c == "{":
            return i
        if c == "(" or c == "[":
            j = _match_paren(s, i, c, ")" if c == "(" else "]")
            if j is None:
                return None
            i = j + 1
            continue
        if c in ";)}":
            return None
        i += 1
    return None


def _definition_at(s: str, start: int, name: str) -> Definition | None:
    """Try to read a full definition whose identifier begins at `start`."""
    i = _skip_ws(s, start + len(name))
    if i >= len(s) or s[i] != "(":
        return None
    par_open = i
    par_close = _match_paren(s, i)
    if par_close is None:
        return None
    i = _skip_ws(s, par_close + 1)
    # trailing cv-qualifier / exception-spec / calling-convention noise
    while True:
        m = _SUFFIX_WORD.match(s, i)
        if not m:
            break
        i = _skip_ws(s, m.end())
        if m.group(1) in ("throw", "__declspec") and i < len(s) and s[i] == "(":
            j = _match_paren(s, i)
            if j is None:
                return None
            i = _skip_ws(s, j + 1)
    init = None
    if i < len(s) and s[i] == ":" and not s.startswith("::", i):
        init = i
        b = _skip_init_list(s, i)
        if b is None:
            return None
        i = b
    if i >= len(s) or s[i] != "{":
        return None
    close = _match_paren(s, i, "{", "}")
    if close is None:
        return None
    return Definition(name, start, par_open, par_close, init, i, close)


def find_definitions(text: str, fn: str) -> list[Definition]:
    """Every definition of `fn` in `text`, in source order.

    Candidate names are tried MOST QUALIFIED FIRST and the first name that
    yields any definition wins, so `Foo::bar` is never confused with a
    same-named free `bar`.
    """
    s = mask(text)
    for name in source_names(fn):
        pat = re.compile(r"(?<![\w:~])" + re.escape(name))
        found = [d for m in pat.finditer(s)
                 if (d := _definition_at(s, m.start(), name)) is not None]
        if found:
            return found
    return []


def body_span(text: str, fn: str):
    """(open_brace_idx, close_brace_idx) of fn's definition body, or None.

    Indices are into `text` itself - the mask is length-preserving - so a
    caller splices mutations straight back into the original source.
    """
    defs = find_definitions(text, fn)
    return (defs[0].body_open, defs[0].body_close) if defs else None


def params(text: str, fn: str) -> list[str]:
    """Parameter NAMES of fn's located definition (last identifier of each
    comma-separated declarator; best-effort).

    Reads the recorded parameter-list span rather than scanning backwards
    from the body, which a member-initialiser list would otherwise poison
    (`: base(parent)` looks exactly like a parameter list in reverse).
    """
    defs = find_definitions(text, fn)
    if not defs:
        return []
    d = defs[0]
    out = []
    depth = 0
    part: list[str] = []
    chunks = []
    for ch in text[d.par_open + 1:d.par_close]:
        if ch in "(<[":
            depth += 1
        elif ch in ")>]":
            depth -= 1
        if ch == "," and depth == 0:
            chunks.append("".join(part))
            part = []
        else:
            part.append(ch)
    chunks.append("".join(part))
    for chunk in chunks:
        toks = re.findall(r"[A-Za-z_]\w*", chunk.split("=")[0])
        if toks and toks[-1] not in ("void", "const", "unsigned", "signed"):
            out.append(toks[-1])
    return out


def explain_miss(text: str, fn: str) -> str:
    """Why `fn` has no locatable body - the message the solvers print.

    Distinguishes the three cases that used to look identical: a
    compiler-generated entity (nothing to find, and that IS the answer), a
    name the v1 decoder cannot spell, and a genuine absence.
    """
    m = demangle(fn)
    if m.compgen:
        return (f"{fn} is a {m.compgen} - compiler-generated, there is no "
                "source body to search (its bytes come from the class "
                "layout, not from a statement)")
    if m.note:
        return f"cannot derive a source name for {fn}: {m.note}"
    names = source_names(fn)
    s = mask(text)
    for name in names:
        pat = re.compile(r"(?<![\w:~])" + re.escape(name))
        if pat.search(s):
            return (f"found `{name}` in the active source but never as a "
                    "definition (declaration or call site only) - is the "
                    "body in another TU, a header, or fenced by a macro?")
    raw = _mask_lex(text)
    for name in names:
        if re.search(r"(?<![\w:~])" + re.escape(name), raw):
            return (f"`{name}` appears only inside an INACTIVE preprocessor "
                    "region (`#if 0  // @carcass`) - the row is still a "
                    "fenced stub, so there is nothing for a solver to say")
    return (f"no definition of {' / '.join(names)} in this TU")
