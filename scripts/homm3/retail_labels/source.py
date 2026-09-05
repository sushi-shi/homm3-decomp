#!/usr/bin/env python3
"""homm3.retail_labels.source - per-TU source-claim extraction.

    homm3 labels [--unit U ...] [--all]

The extraction universe is the sorted regular C/C++ files directly under
src/, NOT the manifest (see the package docstring).

EVERY MACRO IS SCANNED BY BALANCED PARENS, never per line (the mechanism
ported from gruntz). DATA_COMPGEN sits in EXPRESSION position, so
clang-format wraps its argument list across lines and may put two on one
line; VA_COMPGEN's four arguments wrap for the same reason. The per-line
regex scan this replaced required a macro's whole argument list on the
head's own line and dropped every wrapped site SILENTLY - 18 addresses,
present, valid and intended, contributing nothing with no error and no
warning. `check_completeness` is the oracle that makes that
unreintroducible, and it carries its own negative control (`selftest`).

Per TU, the mechanisms are otherwise unchanged from the pre-port
homm3.build.labels:

  VA(0xva, size)      lexical scan (comments/strings blanked); the claim's
                      working name derives from the DECLARATOR that follows
                      (source-as-authority). Fatal: an address below the
                      image base or off a carved function entry, and an
                      orphan annotation with no declaration below it (the
                      gruntz orphan-annotation incident).
  VA_COMPGEN          compiler-generated bodies (static-init dispatch,
                      atexit, scalar deleting dtors, ...) - the claim is
                      named __h3cg$<unit>$<kind>$<owner>; unknown kinds die.
  DATA(0xva)          data claim, dense working name data_<rva>. On a
                      HEADER extern it claims retail's own datum, which
                      this build never defines - a different channel; the
                      model gates those, this module does not.
  DATA_COMPGEN(_GUARD) compiler-generated data pins, named
                      __h3cg$<unit>$...$<name>. Repeated expansions of
                      one pooled datum COALESCE - the VALUE is the claim,
                      so only a disagreeing value is a defect - both
                      within a TU (here) and across TUs (the model, on
                      the agreement this module proves).

THE NAME BINDING (extraction's second, join-bearing concern - as in
gruntz). Two mechanisms, in strict precedence:

  src-VA+ir    clang IR (homm3.core.clang). `@llvm.global.annotations`
               pairs the VA() string DIRECTLY with the function's
               MSVC-mangled symbol, so there is no positional join - an
               inline header definition cannot steal a nearby address -
               and no lossy demangle key, so two functions that normalize
               to one spelling cannot swap names. The proposed name is
               then CONFIRMED against cl's own base obj: the exact string
               must be a defined symbol there. cl's object is therefore a
               CONFIRMER, never an answerer - a stale artifact can no
               longer supply a name the current source has stopped
               producing; it can only fail to confirm, which is reported.

  src-VA+base  the lexical declarator + base-obj key join, kept for the
               claims IR structurally cannot reach: the `#if 0 //
               @carcass` claim-only stubs whose real definition lives in
               a header or in the STL (no compiler sees those bodies) and
               any TU clang cannot parse. This is the mechanism the IR
               channel exists to retire, so what it still binds is
               counted and reported rather than assumed sound.

The fragment keeps the RAW declarator name alongside the bound spelling
because the model's scan-order dedup replays over raw names, exactly as
the monolith did.

Fragments (build/gen/claims/<unit>.tsv) are written atomically and
content-idempotently; a stale fragment whose src file vanished is pruned
on a full run. Fragment freshness follows the base objs: run after a
build (the `homm3 delink` chain does).
"""

from __future__ import annotations

import bisect
import os
import re
import struct
import sys

from homm3.core import clang, common
from homm3.core.tsv import write as write_tsv
from homm3.retail_labels.fragments import FRAGMENTS, HEADER, fragment_path

SRC_DIR = common.HOMM3_DIR / "src"
SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx"})

#: Macro HEADS - the name and its opening paren, nothing more. The
#: argument list is then taken by balanced-paren matching
#: (`macro_invocations`), never by a per-line regex: the argument lists
#: WRAP. Spelled `\s*` between name and paren so the scanner is at least
#: as permissive as `MACRO_SITE_RE`, the completeness sweep's own probe -
#: a site the sweep can see but the scanner cannot reach would make the
#: gate unsatisfiable.
#:
#: VA and VA_COMPGEN stay anchored to the start of a line: they are
#: statement-position attributes that precede a declarator, and the
#: anchor is what keeps the declarator scan meaningful. Anchoring the
#: HEAD costs nothing now that the arguments may wrap past it.
VA_HEAD_RE = re.compile(r"(?m)^[ \t]*VA\s*\(")
VA_COMPGEN_HEAD_RE = re.compile(r"(?m)^[ \t]*VA_COMPGEN\s*\(")
DATA_HEAD_RE = re.compile(r"\bDATA\s*\(")
DATA_COMPGEN_HEAD_RE = re.compile(r"\bDATA_COMPGEN\s*\(")
DATA_COMPGEN_GUARD_HEAD_RE = re.compile(r"\bDATA_COMPGEN_GUARD\s*\(")
#: Claims nothing about the retail image, so it emits no row - indexed
#: only so its own wrapped continuation lines are never mistaken for the
#: declarator a VA() above it is looking for.
DC_ONLY_HEAD_RE = re.compile(r"(?m)^[ \t]*DC_ONLY\s*\(")

#: macro -> (head, arity, prototype). Iteration order is irrelevant; rows
#: come out in TEXT order (see scan_file).
MACRO_HEADS = {
    "VA": (VA_HEAD_RE, 2, "VA(addr, size)"),
    "VA_COMPGEN": (VA_COMPGEN_HEAD_RE, 4,
                   "VA_COMPGEN(addr, size, kind, owner)"),
    "DATA_COMPGEN_GUARD": (DATA_COMPGEN_GUARD_HEAD_RE, 3,
                           "DATA_COMPGEN_GUARD(addr, name, owner)"),
    "DATA_COMPGEN": (DATA_COMPGEN_HEAD_RE, 3,
                     "DATA_COMPGEN(addr, name, value)"),
    "DATA": (DATA_HEAD_RE, 1, "DATA(addr)"),
    "DC_ONLY": (DC_ONLY_HEAD_RE, 2, "DC_ONLY(off, cb)"),
}

ADDR_ARG_RE = re.compile(r"0x[0-9a-fA-F]+$")
SIZE_ARG_RE = re.compile(r"0x[0-9a-fA-F]+$|\d+$")
IDENT_ARG_RE = re.compile(r"[A-Za-z_]\w*$")
ANNOTATION_RE = re.compile(r"^\s*(?:VA|VA_COMPGEN|DATA|DC_ONLY)\s*\(")
DECLARATOR_RE = re.compile(r"([~\w:]+(?:<[^<>()]*>)?)\s*\(")
# Deliberately bounded comparison-operator spellings. Generic C++ declarator
# parsing is still outside this scanner's contract, but operator==/operator!=
# are real source identities carried by VC6's ??8/??9 publics and must not
# collapse to a flat return-type-prefixed working label.
OPERATOR_EQUAL_RE = re.compile(
    r"([~\w:]+(?:<[^<>()]*>)?)::operator\s*==\s*\(")
OPERATOR_NOT_EQUAL_RE = re.compile(
    r"([~\w:]+(?:<[^<>()]*>)?)::operator\s*!=\s*\(")
# MSVC special members render with backticks: Cls::`scalar deleting
# destructor'(...), `default constructor closure'(...)
SPECIAL_RE = re.compile(r"([\w:]+)::`([^'`]+)'\s*\(")
# Template argument list in a source declarator: `vector<int>::begin` ->
# `vector::begin`. Applied to a fixed point so nested lists collapse.
# Template arguments never take part in the join key (the mangled side
# cannot be parsed back to them without a full demangler), so both sides
# normalize them away - see TEMPLATE_MEMBER_RE.
TEMPLATE_ARGS_RE = re.compile(r"<[^<>]*>")
# One MSVC-mangled member of a class template: `?begin@?$vector@HV?$allo
# cator@H@std@@@std@@QAEPAHXZ` -> member `begin`, template `vector`,
# enclosing namespace `std`. The lazy middle skips the argument list
# without parsing it; a shape this does not match simply fails to join
# (a missed rename, never a wrong one).
TEMPLATE_MEMBER_RE = re.compile(r"^\?(\w+)@\?\$(\w+)@.*?@(\w+)@@")
GLOBAL_TEMPLATE_MEMBER_RE = re.compile(r"^\?(\w+)@\?\$(\w+)@.*@@@@")
IDENT_RE = re.compile(r"[^0-9A-Za-z_]+")
#: VC6's builtin-type letters, for the containers whose element is a
#: primitive and therefore carries no class name to key on.
DEQUE_PRIMITIVE_ELEMENT = {"C": "signed_char", "D": "char",
                           "E": "unsigned_char", "F": "short",
                           "G": "unsigned_short", "H": "int",
                           "I": "unsigned_int", "J": "long",
                           "K": "unsigned_long"}
#: The char-instantiated stream and string members VC6 emits as COMDATs.
#: Every one of these templates exists only as `<char, char_traits<char>,
#: allocator<char>>` in this image, so the instantiation carries no useful
#: owner and they all key "char" - the spelling `stringbuf_overflow` and
#: `basic_string_assign_ptr_size` already established. Entries are
#: (mangled prefix, required mangled suffix or None, key suffix); the
#: suffix is what separates two overloads of one member.
CHAR_STREAM_MEMBERS = (
    ("?_Iput@?$num_put@D", None, "num_put_iput"),
    ("?_Fput@?$num_put@D", None, "num_put_fput"),
    ("?_Rep@?$num_put@D", None, "num_put_rep"),
    # num_put::do_put is SIX overloads separated only by the final
    # parameter letter (_N bool, J long, K unsigned long, N double,
    # O long double, PBX const void*). They share one key deliberately:
    # claimed together they form a six-member overload group, which the
    # join zips by RVA order against COFF order - and the two arms whose
    # sizes are unique (bool 658 B, const void* 448 B) sit at the ends of
    # both orders, which is what confirms the zip.
    ("?do_put@?$num_put@D", None, "num_put_do_put"),
    ("?str@?$basic_stringbuf@D", None, "stringbuf_str"),
    ("?seekoff@?$basic_stringbuf@D", None, "stringbuf_seekoff"),
    ("?seekpos@?$basic_stringbuf@D", None, "stringbuf_seekpos"),
    ("?pbackfail@?$basic_stringbuf@D", None, "stringbuf_pbackfail"),
    ("?underflow@?$basic_stringbuf@D", None, "stringbuf_underflow"),
    ("?substr@?$basic_string@D", None, "basic_string_substr"),
    ("?_Grow@?$basic_string@D", None, "basic_string_grow"),
    ("?append@?$basic_string@D", "@ABV12@II@Z", "basic_string_append_str"),
    ("?append@?$basic_string@D", "@PBDI@Z", "basic_string_append_ptr"),
    ("?find@?$basic_string@D", None, "basic_string_find"),
    ("?_Freeze@?$basic_string@D", None, "basic_string_freeze"),
    ("?xsgetn@?$basic_streambuf@D", None, "streambuf_xsgetn"),
    ("?sputc@?$basic_streambuf@D", None, "streambuf_sputc"),
    ("?opfx@?$basic_ostream@D", None, "ostream_opfx"),
    ("??Hstd@@YA?AV?$basic_string@D", None, "basic_string_concat"),
    ("?_Decref@facet@locale@std@@", None, "locale_facet_decref"),
    ("?getloc@ios_base@std@@", None, "ios_base_getloc"),
)


COMPGEN_KINDS = {"STATIC_INIT_DISPATCH", "STATIC_ATEXIT", "STATIC_DTOR",
                 "STATIC_CTOR", "SCALAR_DELETING_DTOR",
                 "VECTOR_DELETING_DTOR", "DEFAULT_CTOR_CLOSURE",
                 "VECTOR_DTOR", "VECTOR_SIZE",
                 "VECTOR_CAPACITY",
                 "VECTOR_CONSTRUCTOR_ITERATOR",
                 "VECTOR_RESIZE", "VECTOR_INSERT", "VECTOR_ERASE",
                 "VECTOR_DESTROY", "VECTOR_UCOPY", "VECTOR_UFILL",
                 "VECTOR_COPY_ASSIGN",
                 "BITSET_TIDY", "BITSET_CTOR",
                 "BITSET_SUBSCRIPT", "BITSET_REFERENCE_ASSIGN",
                 "BITSET_ITERATOR_DEREF",
                 "BITSET_FLIP",
                 "BITSET_COUNT", "BITSET_ANY", "BITSET_SET",
                 "BITSET_TEST", "BITSET_XRAN", "TREE_MIN",
                 "TREE_INSERT", "TREE_NODE_INSERT",
                 "TREE_CONST_ITERATOR_DEC", "TREE_CONST_ITERATOR_INC",
                 "TREE_COPY", "TREE_COPY_NODE", "TREE_ERASE",
                 "STRINGBUF_OVERFLOW", "STRINGBUF_INIT",
                 "DEQUE_FREEFRONT", "DEQUE_FREEBACK", "DEQUE_BUYBACK",
                 "BASIC_STRING_ASSIGN_PTR_SIZE",
                 "OSTREAM_PUT", "OSTREAM_INSERT_CSTR",
                 "INSERTION_SORT_1",
                 "STD_SORT", "STD_SORT_0", "STD_MEDIAN",
                 "STD_UNGUARDED_PARTITION", "STD_UNGUARDED_INSERT",
                 "STD_COPY_BACKWARD", "STD_FILL",
                 "TREE_ERASE_ITERATOR", "TREE_ERASE_RANGE",
                 "DEQUE_ERASE", "VECTOR_RESERVE", "VECTOR_CLEAR",
                 "EXCEPTION_DORAISE", "FUNCTOR_CALL",
                 "DEQUE_ITERATOR_ADD_ASSIGN",
                 "STREAMBUF_XSPUTN",
                 "PAIR_CONST_INT_DTOR",
                 "STD_CONSTRUCT", "STD_COPY",
                 "CLASS_CTOR",
                 "IMPLICIT_COPY_CTOR", "IMPLICIT_COPY_ASSIGN",
                 "IMPLICIT_DTOR"}
COMPGEN_KINDS |= {member.upper() for _p, _s, member in CHAR_STREAM_MEMBERS}


def mask_lexical_noise(blob: str) -> str:
    """Blank comments and string/char literals byte-for-byte (newlines
    kept) so the macro regexes can never fire inside them. Ported from
    homm2's annotated_data._mask_lexical_noise."""
    out = list(blob)
    i, n = 0, len(blob)
    state = None  # None | "line" | "block" | '"' | "'"
    while i < n:
        c = blob[i]
        if state is None:
            if c == "/" and i + 1 < n and blob[i + 1] == "/":
                state = "line"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if c == "/" and i + 1 < n and blob[i + 1] == "*":
                state = "block"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if c == '"':
                state = c
                i += 1
                continue
            if c == "'":
                # enter char-literal state only when it closes nearby: the
                # carcass carries MSVC `scalar deleting destructor' names
                # whose lone apostrophe would otherwise swallow the file
                closer = blob.find("'", i + 1, i + 5)
                if closer != -1 and "\n" not in blob[i:closer]:
                    state = c
                i += 1
                continue
            i += 1
            continue
        if state == "line":
            if c == "\n":
                state = None
            else:
                out[i] = " "
            i += 1
            continue
        if state == "block":
            if c == "*" and i + 1 < n and blob[i + 1] == "/":
                out[i] = out[i + 1] = " "
                state = None
                i += 2
                continue
            if c != "\n":
                out[i] = " "
            i += 1
            continue
        # string/char literal
        if c == "\\" and i + 1 < n:
            out[i] = out[i + 1] = " "
            i += 2
            continue
        if c == state:
            state = None
        elif c != "\n":
            out[i] = " "
        i += 1
    return "".join(out)


def _skip_quote(text: str, i: int) -> int:
    """Index just past the literal opening at `text[i]` (escapes
    honoured). `mask_lexical_noise` already blanks literal BODIES, so on
    masked text this only steps over an empty pair - it is kept so the
    scanner is correct on raw text too, and so a future change to the
    masker cannot silently turn a `,` or `)` inside a string literal into
    an argument separator."""
    quote, n = text[i], len(text)
    i += 1
    while i < n and text[i] != quote:
        i += 2 if text[i] == "\\" else 1
    return i + 1


def _split_top_level(body: str) -> list[str]:
    """`body` split on its TOP-LEVEL commas - a nested call keeps its
    own (`DATA_COMPGEN(0x.., n, f(a, b))` is three arguments, not four),
    exactly as the preprocessor counts them."""
    parts, depth, start, i = [], 0, 0, 0
    while i < len(body):
        c = body[i]
        if c in "\"'":
            i = _skip_quote(body, i)
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            parts.append(body[start:i])
            start = i + 1
        i += 1
    parts.append(body[start:])
    return [p.strip() for p in parts]


def macro_invocations(text: str, head: re.Pattern,
                      raw: str | None = None) -> list[tuple]:
    """[(start, end, args, raw_args)] for every invocation of one macro
    over ALREADY-MASKED text; `end` is the offset of the closing paren,
    or None when the parens never close.

    `args` come from the MASKED text, so a comment written inside an
    argument list is blanked and an identifier argument stays clean.
    `raw_args` come from `raw` at the SAME offsets - masking is
    length-preserving, it only overwrites bytes with spaces - and are the
    only place a string literal's BODY survives. The pooled-datum
    agreement check needs that body: masked, `"%s %s"` and `"%d %d"` are
    both six blanks between quotes, so comparing masked values would call
    two different literals equal.

    Balanced-paren and quote-aware, NEVER line-based - the mechanism
    ported from gruntz's retail_labels.source. DATA_COMPGEN sits in
    EXPRESSION position, so clang-format wraps it across lines and may
    put two of them on one line; VA_COMPGEN's four arguments wrap for the
    same reason. The per-line regexes this replaces required a macro's
    whole argument list on the head's own line, so every wrapped site was
    dropped SILENTLY - a macro that is present, valid and intended
    contributing nothing, with no error and no warning. That is the worst
    failure class a label pipeline has, and `check_completeness` is the
    oracle that now makes it impossible to reintroduce.
    """
    raw = text if raw is None else raw
    out = []
    for m in head.finditer(text):
        depth, j, n = 1, m.end(), len(text)
        while j < n and depth:
            c = text[j]
            if c in "\"'":
                j = _skip_quote(text, j)
                continue
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            j += 1
        if depth:
            out.append((m.start(), None, [], []))
            continue
        out.append((m.start(), j - 1,
                    _split_top_level(text[m.end():j - 1]),
                    _split_top_level(raw[m.end():j - 1])))
    return out


def _line_starts(text: str) -> list[int]:
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)
    return starts


def _macro_args(args, arity: int, prototype: str, where: str) -> list[str]:
    """The invocation's arguments, or death. An arity the preprocessor
    itself would reject can only mean a malformed source line, and a
    malformed line that merely produced no row is exactly the silent loss
    this module exists to prevent."""
    if len(args) != arity:
        common.die(f"{where}: {prototype} takes {arity} argument(s); this "
                   f"invocation has {len(args)} ({args!r})")
    return args


def _arg(value: str, pattern: re.Pattern, what: str, where: str) -> str:
    if not pattern.match(value):
        common.die(f"{where}: {what} {value!r} is malformed")
    return value


def rva_of(addr_text: str, where: str) -> int:
    value = int(addr_text, 16)
    if value < common.IMAGE_BASE:
        common.die(f"{where}: address {addr_text} below image base - the "
                   "v2 contract uses ABSOLUTE VAs")
    return value - common.IMAGE_BASE


def scan_file(path, functions: set[int],
              problems: list[str] | None = None) -> list[dict]:
    """All annotation rows of one src file, in scan (TEXT) order. Names
    are the RAW pre-join spellings; `channel` is the pre-join
    provenance.

    Every macro is taken by `macro_invocations`, so a wrapped argument
    list is read exactly like a single-line one. Row order is the order
    the invocations START in the file, which for unwrapped macros is the
    line order the fragment contract and the model's scan-order dedup
    require.

    REPEATED EXPANSIONS COALESCE (the gruntz rule). DATA_COMPGEN and
    DATA_COMPGEN_GUARD sit in expression position: one pooled literal is
    referenced from as many sites as the source needs it, and each site
    writes the macro again. They are one datum, so they are one row - the
    VALUE is the claim, and only a disagreeing value is a defect. (The
    per-line scanner this replaces never had to decide, because it was
    dropping the wrapped repeats; the moment they became visible the
    model's duplicate-rva gate fired on all 14 of them.)"""
    unit = path.stem
    if problems is None:
        problems = []
    raw = path.read_text(errors="replace")
    text = mask_lexical_noise(raw)
    lines = text.splitlines()
    line_starts = _line_starts(text)

    def line_of(offset: int) -> int:
        return bisect.bisect_right(line_starts, offset)

    found = []
    for macro, (head, arity, prototype) in MACRO_HEADS.items():
        for start, end, args, raw_args in macro_invocations(text, head, raw):
            if end is None:
                common.die(f"{path.name}:{line_of(start)}: {macro}( is "
                           "never closed - unbalanced parentheses")
            found.append((start, macro, end, arity, prototype, args,
                          raw_args))
    found.sort(key=lambda f: f[0])

    # A wrapped invocation's CONTINUATION lines carry no declarator, so a
    # VA() above one must look past them. (No VA site wraps today; the
    # skip is what keeps that from silently becoming an orphan-annotation
    # death the day clang-format decides one should.)
    continuation = set()
    for start, _macro, end, _arity, _proto, _args, _raw in found:
        continuation.update(range(line_of(start) + 1, line_of(end) + 1))

    rows = []
    coalesced: dict[tuple, tuple] = {}     # (macro, rva) -> (payload, where)
    for start, macro, end, arity, prototype, args, raw_args in found:
        where = f"{path.name}:{line_of(start)}"
        args = _macro_args(args, arity, prototype, where)
        if macro == "DC_ONLY":
            continue          # indexed for its line span only; claims nothing
        rva = rva_of(_arg(args[0], ADDR_ARG_RE, "address", where), where)

        if macro in ("DATA_COMPGEN", "DATA_COMPGEN_GUARD"):
            # args[1] names the datum; raw_args[2] is what allocates it -
            # the pooled literal, or the guard's owner. The value is the
            # claim, so it is what decides whether two sites are one datum.
            payload = (args[1], raw_args[2])
            first = coalesced.get((macro, rva))
            if first is not None:
                seen, first_where = first
                if seen[1] != payload[1]:
                    problems.append(
                        f"{unit}: {macro}(0x{rva + common.IMAGE_BASE:08x}) at "
                        f"{where} pins {payload[1]!r} but {first_where} pins "
                        f"{seen[1]!r} - one address, two values")
                elif seen[0] != payload[0]:
                    problems.append(
                        f"{unit}: {macro}(0x{rva + common.IMAGE_BASE:08x}) at "
                        f"{where} names the datum {payload[0]!r} but "
                        f"{first_where} names it {seen[0]!r}; the value "
                        f"agrees, so cl pooled one literal for both roles - "
                        f"{seen[0]!r} wins (first in scan order)")
                continue
            coalesced[(macro, rva)] = (payload, where)

        if macro == "VA":
            if rva not in functions:
                common.die(f"{where}: VA {args[0]} is not a carved "
                           "function entry")
            declared = int(_arg(args[1], SIZE_ARG_RE, "size", where), 0)
            follower = None
            for lineno in range(line_of(end) + 1, line_of(end) + 4):
                if lineno > len(lines):
                    break
                candidate = lines[lineno - 1]
                if (lineno in continuation or not candidate.strip()
                        or ANNOTATION_RE.match(candidate)):
                    continue
                follower = candidate
                break
            if follower is None:
                common.die(f"{where}: orphan VA annotation - no "
                           "declaration follows")
            sm = SPECIAL_RE.search(follower)
            om = OPERATOR_EQUAL_RE.search(follower)
            nom = OPERATOR_NOT_EQUAL_RE.search(follower)
            if sm:
                raw = f"{sm.group(1)}__{sm.group(2)}"
            elif om:
                raw = f"{om.group(1)}::operator_equal"
            elif nom:
                raw = f"{nom.group(1)}::operator_not_equal"
            else:
                # full C++ declarator parsing is a tar pit (templates,
                # most operators, MSVC spellings); everything before the
                # first paren is a stable working label until a
                # clang-IR channel binds real mangled names
                raw = follower.split("(", 1)[0]
                # `std::vector<int>::begin` -> `std::vector::begin`:
                # without this the `<int>` breaks the qualified-name
                # run and only `::begin` survives, which no mangled
                # key can join.
                while TEMPLATE_ARGS_RE.search(raw):
                    raw = TEMPLATE_ARGS_RE.sub("", raw)
                last = DECLARATOR_RE.findall(raw + "(")
                raw = last[-1] if last else raw
                # A NESTED class's constructor or destructor is spelled
                # `Outer::Inner::Inner`, but MSVC mangles it `??0Inner@Outer@@`
                # and _demangle_key keys that as `inner_inner` - so the
                # three-component declarator never joins and the claim keeps a
                # flat carve name forever. Drop the enclosing qualifiers for
                # exactly this shape (last two components equal modulo the
                # destructor tilde); every other declarator is untouched.
                parts = raw.split("::")
                if len(parts) > 2 and parts[-1].lstrip("~") == parts[-2]:
                    raw = "::".join(parts[-2:])
            name = IDENT_RE.sub("_", raw).strip("_")[:64]
            if not name:
                name = f"fn_{rva:x}"
            rows.append({"rva": rva, "unit": unit, "size": declared,
                         "kind": "func", "name": name,
                         "channel": "src-VA",
                         # ctors and dtors collapse to the same
                         # class_class label; the tilde in the
                         # declarator is the only discriminator left
                         "dtor": "~" in raw})
        elif macro == "VA_COMPGEN":
            kind = _arg(args[2], IDENT_ARG_RE, "kind", where)
            owner = _arg(args[3], IDENT_ARG_RE, "owner", where)
            if kind not in COMPGEN_KINDS:
                common.die(f"{where}: unknown VA_COMPGEN kind {kind}")
            rows.append({"rva": rva, "unit": unit,
                         "size": int(_arg(args[1], SIZE_ARG_RE, "size",
                                          where), 0),
                         "kind": "func",
                         "name": f"__h3cg${unit}${kind.lower()}${owner}",
                         "channel": "src-VA_COMPGEN",
                         "ckind": kind, "owner": owner})
        elif macro == "DATA_COMPGEN_GUARD":
            name = _arg(args[1], IDENT_ARG_RE, "name", where)
            rows.append({"rva": rva, "unit": unit, "size": 4,
                         "kind": "data",
                         "name": f"__h3cg${unit}$static_init_guard${name}",
                         "channel": "src-DATA_COMPGEN_GUARD"})
        elif macro == "DATA_COMPGEN":
            name = _arg(args[1], IDENT_ARG_RE, "name", where)
            rows.append({"rva": rva, "unit": unit, "size": "",
                         "kind": "data",
                         "name": f"__h3cg${unit}$data${name}",
                         "channel": "src-DATA_COMPGEN"})
        else:                                              # DATA
            rows.append({"rva": rva, "unit": unit, "size": "",
                         "kind": "data", "name": f"data_{rva:x}",
                         "channel": "src-DATA"})
    return rows


# --- the IR channel ---------------------------------------------------
# `@llvm.global.annotations` is an appending array of tuples whose first
# two members are the annotated symbol and the annotation string; the
# strings themselves are `@.str* = ... c"..."` constants above it.
IR_STR_DEF_RE = re.compile(r'^(@[\w.$"]+)\s*=.*?\bc"((?:[^"\\]|\\.)*)"', re.M)
IR_ANN_TUPLE_RE = re.compile(
    r'\{\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,')
IR_VA_ANN_RE = re.compile(r"^va:(0x[0-9a-fA-F]+) size:(?:0x[0-9a-fA-F]+|\d+)$")


def _unescape_ir_cstr(text: str) -> str:
    out = bytearray()
    i = 0
    while i < len(text):
        if (text[i] == "\\" and len(text) - i >= 3
                and all(c in "0123456789abcdefABCDEF"
                        for c in text[i + 1:i + 3])):
            out.append(int(text[i + 1:i + 3], 16))
            i += 3
        else:
            out.append(ord(text[i]))
            i += 1
    if out and out[-1] == 0:
        out.pop()
    return out.decode("utf-8", "replace")


def _ir_symbol_name(ref: str) -> str:
    """`@"?Value@Widget@@QBEHH@Z"` / `@_foo` -> the bare symbol. A `\\01`
    prefix means clang already wrote the final object name."""
    ref = ref[1:]
    if ref.startswith('"') and ref.endswith('"'):
        ref = ref[1:-1]
    return ref[3:] if ref.startswith("\\01") else ref


def ir_va_names(ir: str) -> dict:
    """{rva: mangled name} from the TU's IR - each pair produced by the
    compiler itself, never by a scan of the text around the macro."""
    strings = {m.group(1): _unescape_ir_cstr(m.group(2))
               for m in IR_STR_DEF_RE.finditer(ir)}
    out = {}
    for line in ir.splitlines():
        if "@llvm.global.annotations" not in line:
            continue
        for sym_ref, str_ref in IR_ANN_TUPLE_RE.findall(line):
            annotation = strings.get(str_ref)
            if annotation is None:
                continue
            m = IR_VA_ANN_RE.match(annotation)
            if m:
                rva = int(m.group(1), 16) - common.IMAGE_BASE
                out[rva] = _ir_symbol_name(sym_ref)
    return out


def unit_ir_names(path) -> dict | None:
    """The unit's IR name map, or None when clang could not read the TU
    (no toolchain, or a source construct cl accepts and clang does not).
    None is always reported by the caller - a silent empty map would look
    exactly like a TU with no claims."""
    ir = clang.emit_ir(path)
    return None if ir is None else ir_va_names(ir)


def _template_width(mangled: str, template_name: str) -> int | None:
    """Decode VC6's non-type argument in a one-width class template."""
    match = re.search(
        rf"\?\${re.escape(template_name)}@\$0([0-9A-P]+)@", mangled)
    if not match:
        return None
    encoded = match.group(1)
    if len(encoded) == 1 and encoded.isdigit():
        return int(encoded) + 1
    value = 0
    for digit in encoded:
        if digit < "A" or digit > "P":
            return None
        value = value * 16 + ord(digit) - ord("A")
    return value


def _bitset_width(mangled: str) -> int | None:
    return _template_width(mangled, "bitset")


#: The `std::` sequence algorithms VC6 emits as ordinary COMDATs. Each is
#: keyed on the CONTAINER ELEMENT plus the comparison predicate, because a
#: single unit routinely emits several instantiations of one of them (in
#: ai_player: `_Sort` over type_creature_value with `greater`, over
#: type_creature_value with the default, and over `long`) and the member
#: name alone cannot tell them apart.
STD_ALGORITHMS = {"_Sort": "std_sort", "_Sort_0": "std_sort_0",
                  "_Median": "std_median",
                  "_Unguarded_partition": "std_unguarded_partition",
                  "_Unguarded_insert": "std_unguarded_insert",
                  "_Insertion_sort_1": "insertion_sort_1",
                  "copy_backward": "std_copy_backward",
                  "fill": "std_fill"}
STD_ALGORITHM_RE = re.compile(
    r"^\?(" + "|".join(sorted(STD_ALGORITHMS, key=len, reverse=True))
    + r")@std@@YI(.*)$")
#: A trailing `U<Name>@@` that is not the element is the predicate; a
#: `greater<T>` arrives as a template and is named for its template.
STD_PREDICATE_RE = re.compile(r"U([A-Za-z_]\w*)@@")


def _std_algorithm_element(rest: str):
    """(element, pointer_depth) for the first real type in a mangled
    argument list, or None. `rest` is everything after `YI`: a void return
    (`X`), a by-value return (`?A<T>`) and any number of `PA`/`PB`
    pointer prefixes are stripped first, so `_Median`'s by-value element
    and `_Sort`'s `T*` first parameter decode the same way."""
    if rest.startswith("X"):
        rest = rest[1:]
    elif rest.startswith("?A"):
        rest = rest[2:]
    depth = 0
    while rest[:2] in ("PA", "PB"):
        rest = rest[2:]
        depth += 1
    if rest.startswith("V?$basic_string@D"):
        return "string", max(0, depth - 1)
    match = re.match(r"[VU]([A-Za-z_]\w*)@", rest)
    if match:
        return match.group(1), max(0, depth - 1)
    if rest[:1] in DEQUE_PRIMITIVE_ELEMENT:
        return DEQUE_PRIMITIVE_ELEMENT[rest[0]], max(0, depth - 1)
    return None


def _std_algorithm_key(mangled: str):
    """`<element>[_ptr][_<predicate>]@<member>` for one of the sequence
    algorithms above, or None. Returns None rather than a partial key when
    the element cannot be decoded, so an unmodelled instantiation stays
    unclaimed instead of colliding with a modelled one."""
    match = STD_ALGORITHM_RE.match(mangled)
    if not match:
        return None
    member, rest = match.group(1), match.group(2)
    decoded = _std_algorithm_element(rest)
    if not decoded:
        return None
    element, depth = decoded
    owner = element.lower() + "_ptr" * depth
    if "U?$greater@" in mangled:
        owner += "_greater"
    else:
        predicate = next(
            (name for name in STD_PREDICATE_RE.findall(rest)
             if name.lower() != element.lower()), None)
        if predicate:
            owner += "_" + predicate.lower()
    return f"{owner}@{STD_ALGORITHMS[member]}"


def _demangle_key(mangled: str):
    """Normalized join key for one MSVC public name: ?Method@Class@@... ->
    class_method, matching scan_file's declarator spelling (:: -> _).
    Ctors (??0) key as class_class - the same collapse the declarator
    scan produces for `armyGroup::armyGroup`; dtors (??1) key as
    class_class@dtor so an overloaded-ctor group never absorbs its
    dtor. Assignment (??4) keys to the declarator scanner's stable
    `Class_Class_operator` spelling. Equality (??8) keys to the bounded
    `Class_operator_equal` / `Class_operator_not_equal` spellings; other
    special operators return None."""
    tree_value = re.search(
        r"\?\$_Tree@H(?:V|U)\?\$pair@\$\$CBH(?:V|U)([A-Za-z_]\w*)@",
        mangled)
    # Maps whose key is a class encode that key immediately after `_Tree@`.
    # Keep the mapped-value key above for the established map<int, T> claims,
    # and use the named key as the stable owner for maps such as the resource
    # cache (`_Tree@UTCacheMapKey@ResourceManager@@...`).
    tree_named_owner = re.search(
        r"\?\$_Tree@(?:V|U)([A-Za-z_]\w*)@", mangled)
    tree_pointer_owner = re.search(
        r"\?\$_Tree@P(?:A|B)?(?:V|U)([A-Za-z_]\w*)@", mangled)
    tree_string_owner = re.search(
        r"\?\$_Tree@V\?\$basic_string@D", mangled)
    tree_set_primitive = re.match(
        r"^\?\w+@\?\$_Tree@([CDEFGHIJK])\1U_Kfn@\?\$set@\1", mangled)
    tree_owner = ((DEQUE_PRIMITIVE_ELEMENT[tree_set_primitive.group(1)]
                   + "_set") if tree_set_primitive else
                  tree_value.group(1) if tree_value else
                  tree_named_owner.group(1) if tree_named_owner else
                  tree_pointer_owner.group(1) if tree_pointer_owner else
                  "string" if tree_string_owner else None)
    if mangled.startswith("?_Min@?$_Tree@") and tree_value:
        return f"{tree_value.group(1).lower()}@tree_min"
    if mangled.startswith("?insert@?$_Tree@") and tree_owner:
        return f"{tree_owner.lower()}@tree_insert"
    if mangled.startswith("?_Insert@?$_Tree@") and tree_value:
        return f"{tree_value.group(1).lower()}@tree_node_insert"
    if mangled.startswith("?_Dec@const_iterator@?$_Tree@") and tree_owner:
        return f"{tree_owner.lower()}@tree_const_iterator_dec"
    if mangled.startswith("?_Inc@const_iterator@?$_Tree@") and tree_owner:
        return f"{tree_owner.lower()}@tree_const_iterator_inc"
    # _Tree's two _Copy overloads and its node eraser. `_Copy` is
    # overloaded on the SAME class, so the two arms are separate kinds
    # rather than one two-member group: the node form is the one whose
    # return type is `_Node*` (`IAEPAU_Node@`), the whole-tree form takes
    # `const _Tree&` and returns void.
    if mangled.startswith("?_Copy@?$_Tree@") and tree_owner:
        if "IAEPAU_Node@" in mangled:
            return f"{tree_owner.lower()}@tree_copy_node"
        return f"{tree_owner.lower()}@tree_copy"
    if mangled.startswith("?_Erase@?$_Tree@") and tree_owner:
        return f"{tree_owner.lower()}@tree_erase"
    # ...and the PUBLIC `erase`, which is overloaded on one class: the
    # range form takes two iterators (`V312@0@Z`), the single form one
    # (`V312@@Z`). Two kinds rather than a two-member overload group, for
    # the same reason `_Copy` needed the split - the group's members would
    # otherwise have to be told apart by size alone.
    if mangled.startswith("?erase@?$_Tree@") and tree_owner:
        if mangled.endswith("V312@0@Z"):
            return f"{tree_owner.lower()}@tree_erase_range"
        return f"{tree_owner.lower()}@tree_erase_iterator"
    # basic_stringbuf's two out-of-line members. Keyed on the TEMPLATE
    # name, not on `_Init` alone: basic_streambuf has a nullary `_Init` of
    # its own and both COMDATs can live in one object.
    if mangled.startswith("?overflow@?$basic_stringbuf@D"):
        return "char@stringbuf_overflow"
    if mangled.startswith("?_Init@?$basic_stringbuf@D"):
        return "char@stringbuf_init"
    # deque's two block-freeing helpers. Its element is a PRIMITIVE here
    # (`?$deque@H` is deque<int>), which the class-name regexes above
    # cannot reach - VC6 spells builtin types as a single letter with no
    # `@` terminator - so the code is decoded directly. The owner spelling
    # follows the one std::copy's int arms already use.
    deque_primitive = re.match(
        r"^\?(_Free(?:front|back))@\?\$deque@([CDEFGHIJK])V\?\$allocator@",
        mangled)
    if deque_primitive:
        element = DEQUE_PRIMITIVE_ELEMENT.get(deque_primitive.group(2))
        if element:
            member = deque_primitive.group(1).lstrip("_").lower()
            return f"{element}@deque_{member}"
    deque_iterator = re.match(
        r"^\?\?Yiterator@\?\$deque@([CDEFGHIJK])V\?\$allocator@", mangled)
    if deque_iterator:
        element = DEQUE_PRIMITIVE_ELEMENT.get(deque_iterator.group(1))
        if element:
            return f"{element}@deque_iterator_add_assign"
    deque_erase = re.match(
        r"^\?erase@\?\$deque@([CDEFGHIJK])V\?\$allocator@", mangled)
    if deque_erase:
        element = DEQUE_PRIMITIVE_ELEMENT.get(deque_erase.group(1))
        if element:
            return f"{element}@deque_erase"
    # ...and the block-BUYING half, whose deque element here is an
    # ordinary pointer-to-class, so the usual `P[AB](V|U)<Name>@` shape
    # names it.
    deque_class = re.match(
        r"^\?(_Buyback)@\?\$deque@(?:P[AB])?(?:V|U)([A-Za-z_]\w*)@",
        mangled)
    if deque_class:
        member = deque_class.group(1).lstrip("_").lower()
        return f"{deque_class.group(2).lower()}@deque_{member}"
    vector_string_element = "?$vector@V?$basic_string@D" in mangled
    #: A vector over a BUILTIN element carries no class name at all - VC6
    #: spells the type as a single letter with no `@` terminator - so the
    #: class regex below cannot reach it. Decoded like deque's, above.
    vector_primitive = re.match(
        r"^\?\??\w*@?\?\$vector@([CDEFGHIJK])V\?\$allocator@", mangled)
    vector_element = re.search(
        r"\?\$vector@(?:(?:P[AB][VU])|(?:V|U|W4))?"
        r"([A-Za-z_]\w*)@", mangled)
    nested_vector_element = re.search(
        r"\?\$vector@V\?\$vector@(?:V|U)([A-Za-z_]\w*)@", mangled)
    vector_owner = (
        f"{nested_vector_element.group(1).lower()}_vector"
        if nested_vector_element else
        "string" if vector_string_element else
        DEQUE_PRIMITIVE_ELEMENT[vector_primitive.group(1)]
        if vector_primitive else
        vector_element.group(1).lower() if vector_element else None)
    if mangled.startswith("??1?$vector@") and vector_owner:
        return f"{vector_owner}@vector_dtor"
    if mangled.startswith("??_H@"):
        # MSVC's array/vector constructor iterator is type-erased in its
        # public name: the element constructor arrives as a function-pointer
        # argument.  There can be only one ??_H COMDAT spelling in a TU, so
        # its claim key is intentionally independent of the descriptive
        # source owner.
        return "vector_constructor_iterator"
    if mangled.startswith("?size@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_size"
    if mangled.startswith("?capacity@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_capacity"
    if mangled.startswith("?clear@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_clear"
    if mangled.startswith("?reserve@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_reserve"
    if mangled.startswith("?resize@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_resize"
    if mangled.startswith("?insert@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_insert"
    if mangled.startswith("?erase@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_erase"
    if mangled.startswith("?_Destroy@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_destroy"
    if mangled.startswith("?_Ucopy@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_ucopy"
    if mangled.startswith("?_Ufill@?$vector@") and vector_owner:
        return f"{vector_owner}@vector_ufill"
    if mangled.startswith("??4?$vector@") and vector_owner:
        return f"{vector_owner}@vector_copy_assign"
    bitset_width = _bitset_width(mangled)
    if bitset_width is not None:
        if mangled.startswith("??0?$bitset@"):
            return f"bitset{bitset_width}@bitset_ctor"
        if mangled.startswith("??A?$bitset@"):
            return f"bitset{bitset_width}@bitset_subscript"
        if mangled.startswith("??4reference@?$bitset@"):
            return f"bitset{bitset_width}@bitset_reference_assign"
        for member in (
                "_Tidy", "_Xran", "flip", "count", "any", "set", "test"):
            if mangled.startswith(f"?{member}@?$bitset@"):
                return (f"bitset{bitset_width}@bitset_"
                        f"{member.lstrip('_').lower()}")
    iterator_width = _template_width(mangled, "bitset_iterator")
    if iterator_width is not None and mangled.startswith(
            "??D?$bitset_iterator@"):
        return f"bitset{iterator_width}@bitset_iterator_deref"
    if mangled.startswith("?_Construct@std@@YIXPAV?$basic_string@D"):
        return "string@std_construct"
    # ...and over a map's value_type, whose element is `pair<const int, T>`.
    # Keyed on T with a `_pair` suffix, the spelling `pair_const_int_dtor`
    # already uses for the same shape.
    construct_pair = re.match(
        r"^\?_Construct@std@@YIXPAU\?\$pair@\$\$CBH(?:V|U)([A-Za-z_]\w*)@",
        mangled)
    if construct_pair:
        return f"{construct_pair.group(1).lower()}_pair@std_construct"
    construct_owner = re.match(
        r"^\?_Construct@std@@YIXPA(?:V|U)([A-Za-z_]\w*)@", mangled)
    if construct_owner:
        return f"{construct_owner.group(1).lower()}@std_construct"
    copy_owner = re.match(
        r"^\?copy@std@@YI(?:PA|PB)(?:V|U)([A-Za-z_]\w*)@", mangled)
    if copy_owner:
        return f"{copy_owner.group(1).lower()}@std_copy"
    if mangled.startswith("?copy@std@@YIPAHPAH"):
        return "int@std_copy"
    if mangled.startswith("?copy@std@@YIPAHPBH"):
        return "const_int@std_copy"
    if mangled.startswith(
            "?put@?$basic_ostream@DU?$char_traits@D@std@@@std@@"):
        return "char@ostream_put"
    if mangled.startswith(
            "??6std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@") \
            and mangled.endswith("@PBD@Z"):
        return "char@ostream_insert_cstr"
    if (mangled.startswith("?_Insertion_sort_1@std@@")
            and "CampaignHeaderPointerLess" in mangled):
        return "campaignheaderpointerless@insertion_sort_1"
    # The spellbook's own list sort. Named explicitly beside the campaign
    # one rather than generalized: the two instantiations put DIFFERENT
    # things in the mangling (that one names its predicate, this one its
    # element), so no single extraction covers both.
    if (mangled.startswith("?_Insertion_sort_1@std@@")
            and "TSpellbookEntry" in mangled):
        return "tspellbookentry@insertion_sort_1"
    algorithm_key = _std_algorithm_key(mangled)
    if algorithm_key:
        return algorithm_key
    for prefix, suffix, member in CHAR_STREAM_MEMBERS:
        if mangled.startswith(prefix) and (suffix is None
                                           or mangled.endswith(suffix)):
            return f"char@{member}"
    if mangled.startswith(
            "?xsputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@"):
        return "char@streambuf_xsputn"
    if (mangled.startswith("?assign@?$basic_string@D")
            and mangled.endswith("@PBDI@Z")):
        return "char@basic_string_assign_ptr_size"
    doraise = re.match(r"^\?_Doraise@([A-Za-z_]\w*)@std@@", mangled)
    if doraise:
        return f"{doraise.group(1).lower()}@exception_doraise"
    functor_call = re.match(r"^\?\?R([A-Za-z_]\w*)@@", mangled)
    if functor_call:
        return f"{functor_call.group(1).lower()}@functor_call"
    if mangled.startswith("??_G"):
        # scalar deleting destructor - joined by the VA_COMPGEN
        # SCALAR_DELETING_DTOR claims (owner = the class)
        owner = mangled[4:]
        if owner.startswith("?$"):
            # A class-template owner starts `?$Class@...`; only the stable
            # template name joins, exactly as the ??_D arm below already does
            # (pairs CAutoArray<T> scalar-deleting-dtors in dxplay/mpw).
            cls = owner[2:].split("@", 1)[0]
        else:
            cls = owner.split("@@", 1)[0].split("@")[0]
        return f"{cls}_{cls}".lower() + "@gdtor"
    if mangled.startswith("??_E"):
        # vector deleting destructor - another real named COFF public,
        # distinct from both ??_G and std::vector<T>::~vector.
        cls = mangled[4:].split("@@", 1)[0].split("@")[0]
        return f"{cls}_{cls}".lower() + "@vdtor"
    if mangled.startswith("??_F"):
        # MSVC's `default constructor closure'. The first admitted case is
        # vector<CObjectType>; VA_COMPGEN's identifier-only owner names the
        # element class, which is stable even though the full template type
        # cannot be a macro argument without exposing its comma list.
        if vector_element:
            return f"{vector_element.group(1).lower()}@fctor"
        cls = mangled[4:].split("@@", 1)[0].split("@")[0]
        return f"{cls.lower()}@fctor" if cls else None
    pair_const_int = re.match(
        r"^\?\?1\?\$pair@\$\$CBH(?:V|U)([A-Za-z_]\w*)@@@std@@", mangled)
    if pair_const_int:
        return f"{pair_const_int.group(1).lower()}@pair_const_int_dtor"
    if mangled.startswith("??_D"):
        # MSVC's `vbase destructor' closure. Claim-only carcass rows use
        # the compiler's own backtick spelling, which scan_file normalizes
        # to Class__vbase_destructor.
        owner = mangled[4:]
        if owner.startswith("?$"):
            # A class-template owner starts `?$Class@...`; only the stable
            # template name participates in the source join, just as it does
            # for ordinary template members below.
            cls = owner[2:].split("@", 1)[0]
        else:
            cls = owner.split("@@", 1)[0].split("@")[0]
        return f"{cls}__vbase_destructor".lower()
    if mangled.startswith("??0") or mangled.startswith("??1"):
        owner = mangled[3:]
        if owner.startswith("?$"):
            # Ordinary ctors/dtors of a global class template use
            # `??[01]?$Class@...`; retain only the stable template name,
            # exactly as the member and deleting-dtor arms above do.
            cls = owner[2:].split("@", 1)[0]
        else:
            cls = owner.split("@@", 1)[0].split("@")[0]
        key = f"{cls}_{cls}".lower()
        return f"{key}@dtor" if mangled.startswith("??1") else key
    if mangled.startswith("??4"):
        cls = mangled[3:].split("@@", 1)[0].split("@")[0]
        return f"{cls}_{cls}_operator".lower()
    if mangled.startswith("??8"):
        cls = mangled[3:].split("@@", 1)[0].split("@")[0]
        return f"{cls}_operator_equal".lower() if cls else None
    if mangled.startswith("??9"):
        cls = mangled[3:].split("@@", 1)[0].split("@")[0]
        return f"{cls}_operator_not_equal".lower() if cls else None
    m = GLOBAL_TEMPLATE_MEMBER_RE.match(mangled)
    if m:
        # member of a global class template: template_member. Global owners
        # end in four @ characters; there is no namespace component between
        # the template argument list and the member-function type.
        return f"{m.group(2)}_{m.group(1)}".lower()
    m = TEMPLATE_MEMBER_RE.match(mangled)
    if m:
        # member of a class template: namespace_template_member, the same
        # spelling scan_file derives from `std::vector<int>::begin`
        # once TEMPLATE_ARGS_RE has dropped the argument list
        return f"{m.group(3)}_{m.group(2)}_{m.group(1)}".lower()
    if not mangled.startswith("?") or mangled.startswith("??"):
        # C-mangled stdcall/fastcall publics: _name@N / @name@N -> name
        # (extern "C" __stdcall in a game TU; first hit _WinMain@16).
        # Plain cdecl _name is left alone - no claim needs it yet.
        m = re.match(r"^[_@]([A-Za-z_$][\w$]*)@\d+$", mangled)
        if m:
            return m.group(1).lower()
        return None
    components = mangled[1:].split("@@", 1)[0].split("@")
    qualified = list(reversed(components[1:])) + [components[0]]
    return "_".join(qualified).lower()


def _base_authority_names(unit: str) -> dict:
    """key -> [(mangled, content_size)...] defined text symbols (external
    or file-static function) of the
    unit's compiled base obj, each key's list in DEFINITION order (COFF
    section order - VC6 emits one COMDAT per function in source order,
    so an overload group's order matches the claims' rva order).
    content_size is the symbol's section raw size minus the trailing
    0x90 COMDAT-alignment fill (same rule the comparison normalization
    applies) - the discriminator for overload groups that carry
    unclaimed members retail dropped."""
    obj = common.HOMM3_DIR / f"build/objdiff/base/{unit}.obj"
    if not obj.is_file():
        return {}
    data = obj.read_bytes()
    nsec, = struct.unpack_from("<H", data, 2)
    section_sizes = {}
    for index in range(nsec):
        header = 20 + index * 40
        raw_size, raw_offset = struct.unpack_from("<II", data, header + 16)
        content = raw_size
        if raw_offset:
            raw = data[raw_offset:raw_offset + raw_size]
            run = 0
            while run < len(raw) and run < 15 and raw[len(raw) - 1 - run] == 0x90:
                run += 1
            content = raw_size - run
        section_sizes[index + 1] = content
    symoff, nsyms = struct.unpack_from("<II", data, 8)
    strtab = symoff + nsyms * 18
    def symname(o):
        if struct.unpack_from("<I", data, o)[0] == 0:
            so = struct.unpack_from("<I", data, o + 4)[0]
            return data[strtab + so:data.index(b"\0", strtab + so)].decode(
                errors="replace")
        return data[o:o + 8].rstrip(b"\0").decode(errors="replace")
    ordered = []
    o, i = symoff, 0
    while i < nsyms:
        section = struct.unpack_from("<h", data, o + 12)[0]
        storage = data[o + 16]
        # Defined externals, plus file-static FUNCTIONS (storage 3 with
        # a C++-mangled name; first: monframeinfo's static
        # InitializeCreatureAnimationTraits, which retail keeps static -
        # the mangled spelling is still the true pairing name). Static
        # DATA never mangles (`_name`), so _demangle_key drops it.
        if storage in (2, 3) and section > 0:
            name = symname(o)
            key = _demangle_key(name)
            if key:
                ordered.append((section, key, name,
                                section_sizes.get(section, 0)))
            elif name.startswith("??"):
                # The IR join already has the exact Clang-mangled spelling;
                # keep other C++ operators in the authority set even when
                # the weaker lexical-key join has no safe operator key.
                ordered.append((section, "@mangled:" + name, name,
                                section_sizes.get(section, 0)))
        aux = data[o + 17]
        o += 18 * (1 + aux)
        i += 1 + aux
    groups = {}
    for _section, key, name, content in sorted(ordered):
        groups.setdefault(key, []).append((name, content))
    return groups


def ir_bind(unit: str, rows: list[dict], ir_names: dict,
            problems: list[str]) -> set:
    """Bind VA() claims to the mangled names clang paired them with, in
    place; returns the mangled names taken (which the lexical join must
    then leave alone).

    cl's own obj CONFIRMS: the exact string clang proposed must be a
    defined symbol there. Confirmation is what makes the mirror and the
    two compilers' manglers self-checking - and it is one-directional, so
    a stale obj can only fail to confirm, never answer with a name the
    source has stopped producing."""
    content = {name: size
               for group in _base_authority_names(unit).values()
               for name, size in group}
    taken = set()
    for row in rows:
        if row["channel"] != "src-VA":
            continue
        mangled = ir_names.get(row["rva"])
        if mangled is None:
            continue          # not compiled here (a `#if 0` carcass stub)
        if content and mangled not in content:
            # The obj contradicts the compiler's own pairing, so it is the
            # LAST thing that may name this claim: handing the row to the
            # lexical key join would let that same disputed object answer
            # by a weaker match. The claim keeps its raw declarator label
            # and the disagreement is stated.
            row["ir_unconfirmed"] = True
            problems.append(
                f"{unit}: VA(0x{row['rva'] + common.IMAGE_BASE:08x}) - clang "
                f"names it {mangled!r}, which {unit}.obj does not define; "
                f"the object may be stale, or the two manglers disagree. "
                f"Claim left UNJOINED on its raw declarator label.")
            continue
        row["joined"] = mangled
        row["channel"] = "src-VA+ir"
        taken.add(mangled)
        _report_size_mismatch(unit, row, mangled, content, problems)
    return taken


#: A claim whose retail extent dwarfs the compiled body of the symbol the
#: annotation actually landed on. Measured spread over the 1396 bound
#: claims: 1240 are size-EQUAL (the byte-exact ones) and the widest
#: legitimate gap is 2.2x (`game::Save`, a reconstruction still short of
#: retail). 4x is therefore comfortably outside the real distribution and
#: far below a mis-binding: the mapcell incident of 2026-08-20 put a
#: 0x1e3-byte claim on a small local helper, ~10x.
SIZE_MISMATCH_RATIO = 4


def _report_size_mismatch(unit, row, mangled, content, problems) -> None:
    """The cross-check the IR channel makes possible at all.

    Knowing WHICH symbol the annotation landed on lets us ask cl how big
    that symbol's body is. It catches the one mis-binding IR cannot
    prevent: a definition written BETWEEN the VA() line and the declarator
    it was meant to annotate. A leading `__attribute__` binds to the next
    declaration, so clang attaches the annotation to the intervening
    definition exactly as the lexical follower scan does - the source
    genuinely says the helper is annotated, and no reader can know
    otherwise. The retail extent can: a claim sized for a real function
    does not fit a helper."""
    compiled = content.get(mangled)
    if not compiled or not isinstance(row["size"], int):
        return
    if row["size"] < compiled * SIZE_MISMATCH_RATIO:
        return
    problems.append(
        f"{unit}: VA(0x{row['rva'] + common.IMAGE_BASE:08x}) claims "
        f"0x{row['size']:x} retail bytes but the symbol it annotates, "
        f"{mangled!r}, compiles to only 0x{compiled:x} - "
        f"{row['size'] / compiled:.0f}x. Check for a definition written "
        f"between the VA() line and its intended declarator (the "
        f"annotation binds to whatever declaration FOLLOWS it).")


def join_unit(unit: str, rows: list[dict], taken: set | None = None) -> None:
    """The base-obj name-authority join, in place: a compiled unit's public
    symbols carry the TRUE MSVC spellings; uniquely-joined claims adopt
    them (channel src-VA+base). Keys are built from RAW names - equivalent
    to the monolith's post-dedup key stripping, since a `_<rva>` suffix
    always stripped back to the raw spelling before keying.

    Runs only over what the IR channel did not bind. `taken` names are
    removed from the authority groups: a symbol clang already paired with
    its own claim must not also be offered to a second, weaker key match."""
    taken = taken or set()
    unit_rows = [r for r in rows
                 if not r.get("ir_unconfirmed")
                 and (r["channel"] == "src-VA"
                      or (r["channel"] == "src-VA_COMPGEN"
                          and ("$scalar_deleting_dtor$" in r["name"]
                               or "$vector_deleting_dtor$" in r["name"]
                               or "$default_ctor_closure$" in r["name"]
                               or "$vector_dtor$" in r["name"]
                               or "$vector_size$" in r["name"]
                               or "$vector_capacity$" in r["name"]
                               or "$vector_constructor_iterator$" in r["name"]
                               or "$vector_reserve$" in r["name"]
                               or "$vector_clear$" in r["name"]
                               or "$exception_doraise$" in r["name"]
                               or "$functor_call$" in r["name"]
                               or "$deque_iterator_add_assign$" in r["name"]
                               or "$vector_resize$" in r["name"]
                               or "$vector_insert$" in r["name"]
                               or "$vector_erase$" in r["name"]
                               or "$vector_destroy$" in r["name"]
                               or "$vector_ucopy$" in r["name"]
                               or "$vector_ufill$" in r["name"]
                               or "$vector_copy_assign$" in r["name"]
                               or "$bitset_tidy$" in r["name"]
                               or "$bitset_ctor$" in r["name"]
                               or "$bitset_subscript$" in r["name"]
                               or "$bitset_reference_assign$" in r["name"]
                               or "$bitset_iterator_deref$" in r["name"]
                               or "$bitset_flip$" in r["name"]
                               or "$bitset_count$" in r["name"]
                               or "$bitset_any$" in r["name"]
                               or "$bitset_set$" in r["name"]
                               or "$bitset_test$" in r["name"]
                               or "$bitset_xran$" in r["name"]
                               or "$tree_min$" in r["name"]
                               or "$tree_insert$" in r["name"]
                               or "$tree_node_insert$" in r["name"]
                               or "$tree_const_iterator_dec$" in r["name"]
                               or "$tree_const_iterator_inc$" in r["name"]
                               or "$tree_copy$" in r["name"]
                               or "$tree_copy_node$" in r["name"]
                               or "$tree_erase$" in r["name"]
                               or "$tree_erase_iterator$" in r["name"]
                               or "$tree_erase_range$" in r["name"]
                               or "$deque_erase$" in r["name"]
                               or "$stringbuf_overflow$" in r["name"]
                               or "$stringbuf_init$" in r["name"]
                               or "$deque_freefront$" in r["name"]
                               or "$deque_freeback$" in r["name"]
                               or "$deque_buyback$" in r["name"]
                               or "$basic_string_assign_ptr_size$" in r["name"]
                               or "$ostream_put$" in r["name"]
                               or "$ostream_insert_cstr$" in r["name"]
                               or "$insertion_sort_1$" in r["name"]
                               or "$std_sort$" in r["name"]
                               or "$std_sort_0$" in r["name"]
                               or "$std_median$" in r["name"]
                               or "$std_unguarded_partition$" in r["name"]
                               or "$std_unguarded_insert$" in r["name"]
                               or "$std_copy_backward$" in r["name"]
                               or "$std_fill$" in r["name"]
                               or "$streambuf_xsputn$" in r["name"]
                               or any(f"${member}$" in r["name"]
                                      for _p, _s, member
                                      in CHAR_STREAM_MEMBERS)
                               or "$pair_const_int_dtor$" in r["name"]
                               or "$std_construct$" in r["name"]
                               or "$std_copy$" in r["name"]
                               or "$class_ctor$" in r["name"]
                               or "$implicit_copy_ctor$" in r["name"]
                               or "$implicit_copy_assign$" in r["name"]
                               or "$implicit_dtor$" in r["name"])))]
    if not unit_rows:
        return
    authority = {key: [(n, c) for n, c in group if n not in taken]
                 for key, group in _base_authority_names(unit).items()}
    authority = {key: group for key, group in authority.items() if group}
    if not authority:
        return
    dtor_rvas = {r["rva"] for r in rows if r.get("dtor")}
    claim_keys = {}
    for row in unit_rows:
        if "$scalar_deleting_dtor$" in row["name"]:
            # ??_G claims join the base publics like source functions do
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}_{owner}@gdtor",
                                  []).append(row)
            continue
        if "$vector_deleting_dtor$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}_{owner}@vdtor",
                                  []).append(row)
            continue
        if "$default_ctor_closure$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@fctor", []).append(row)
            continue
        if "$vector_dtor$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_dtor", []).append(row)
            continue
        if "$vector_size$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_size", []).append(row)
            continue
        if "$vector_capacity$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_capacity", []).append(row)
            continue
        if "$vector_constructor_iterator$" in row["name"]:
            claim_keys.setdefault("vector_constructor_iterator", []).append(row)
            continue
        simple = next(
            (kind for kind in ("vector_clear", "exception_doraise",
                               "functor_call", "deque_iterator_add_assign")
             if f"${kind}$" in row["name"]), None)
        if simple is not None:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@{simple}", []).append(row)
            continue
        if "$vector_reserve$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_reserve", []).append(row)
            continue
        if "$vector_resize$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_resize", []).append(row)
            continue
        if "$vector_insert$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_insert", []).append(row)
            continue
        if "$vector_erase$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_erase", []).append(row)
            continue
        if "$vector_destroy$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_destroy", []).append(row)
            continue
        if "$vector_ucopy$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_ucopy", []).append(row)
            continue
        if "$vector_ufill$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_ufill", []).append(row)
            continue
        if "$vector_copy_assign$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@vector_copy_assign", []).append(row)
            continue
        if "$bitset_tidy$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@bitset_tidy", []).append(row)
            continue
        bitset_member = next((member for member in (
            "ctor", "subscript", "reference_assign", "flip", "count",
            "any", "set", "test", "xran")
            if f"$bitset_{member}$" in row["name"]), None)
        if bitset_member is not None:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@bitset_{bitset_member}", []).append(row)
            continue
        if "$bitset_iterator_deref$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@bitset_iterator_deref", []).append(row)
            continue
        if "$tree_min$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@tree_min", []).append(row)
            continue
        if "$tree_insert$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@tree_insert", []).append(row)
            continue
        if "$tree_node_insert$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@tree_node_insert", []).append(row)
            continue
        if "$tree_const_iterator_dec$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@tree_const_iterator_dec", []).append(row)
            continue
        if "$tree_const_iterator_inc$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@tree_const_iterator_inc", []).append(row)
            continue
        tree_extra = next((member for member in (
            "copy_node", "copy", "erase")
            if f"$tree_{member}$" in row["name"]), None)
        if tree_extra is not None:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@tree_{tree_extra}",
                                  []).append(row)
            continue
        deque_member = next((member for member in ("freefront", "freeback",
                                                   "buyback")
                             if f"$deque_{member}$" in row["name"]), None)
        if deque_member is not None:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@deque_{deque_member}",
                                  []).append(row)
            continue
        stringbuf_member = next((member for member in ("overflow", "init")
                                 if f"$stringbuf_{member}$" in row["name"]),
                                None)
        if stringbuf_member is not None:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@stringbuf_{stringbuf_member}",
                                  []).append(row)
            continue
        if "$basic_string_assign_ptr_size$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@basic_string_assign_ptr_size", []).append(row)
            continue
        if "$ostream_put$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@ostream_put", []).append(row)
            continue
        if "$ostream_insert_cstr$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@ostream_insert_cstr", []).append(row)
            continue
        if "$insertion_sort_1$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@insertion_sort_1", []).append(row)
            continue
        algorithm = next(
            (kind for kind in ("std_sort_0", "std_sort", "std_median",
                               "std_unguarded_partition",
                               "std_unguarded_insert",
                               "std_copy_backward", "std_fill")
             if f"${kind}$" in row["name"]), None)
        if algorithm is not None:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@{algorithm}", []).append(row)
            continue
        tree_or_deque = next(
            (kind for kind in ("tree_erase_iterator", "tree_erase_range",
                               "deque_erase")
             if f"${kind}$" in row["name"]), None)
        if tree_or_deque is not None:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@{tree_or_deque}", []).append(row)
            continue
        char_member = next(
            (member for _p, _s, member in CHAR_STREAM_MEMBERS
             if f"${member}$" in row["name"]), None)
        if char_member is not None:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@{char_member}", []).append(row)
            continue
        if "$streambuf_xsputn$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@streambuf_xsputn", []).append(row)
            continue
        if "$pair_const_int_dtor$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(
                f"{owner}@pair_const_int_dtor", []).append(row)
            continue
        if "$std_construct$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@std_construct", []).append(row)
            continue
        if "$std_copy$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}@std_copy", []).append(row)
            continue
        if "$class_ctor$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}_{owner}", []).append(row)
            continue
        if "$implicit_copy_ctor$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}_{owner}", []).append(row)
            continue
        if "$implicit_copy_assign$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}_{owner}_operator", []).append(row)
            continue
        if "$implicit_dtor$" in row["name"]:
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}_{owner}@dtor", []).append(row)
            continue
        key = row["name"].lower()
        if row["rva"] in dtor_rvas:
            key = f"{key}@dtor"
        claim_keys.setdefault(key, []).append(row)
    for key, mangled_group in authority.items():
        candidates = claim_keys.get(key)
        if not candidates:
            continue  # unimplemented group: leave labeled
        if len(candidates) == len(mangled_group):
            # overload groups zip in order: claims by rva (link
            # order), mangled names by COFF section (definition
            # order) - the same order for a VC6 TU
            for row, (mangled, _content) in zip(
                    sorted(candidates, key=lambda r: r["rva"]),
                    mangled_group):
                row["joined"] = mangled
                row["channel"] = "src-VA+base"
            continue
        # count mismatch: the base emits overloads retail dropped
        # (/Ob2 keeps every definition, OPT:REF discarded the
        # unreferenced ones). Pair by EXACT content size, and only
        # when the assignment is unambiguous both ways.
        for row in candidates:
            fits = [name for name, content in mangled_group
                    if content == row["size"]]
            if len(fits) != 1:
                continue
            mangled = fits[0]
            claim_fits = [r for r in candidates
                          if any(c == r["size"]
                                 for n, c in mangled_group
                                 if n == mangled)]
            if len(claim_fits) != 1:
                continue
            row["joined"] = mangled
            row["channel"] = "src-VA+base"


def _fragment_rows(rows: list[dict]) -> list[list[str]]:
    out = []
    for r in rows:
        size = r["size"]
        out.append([f"0x{r['rva']:x}",
                    f"0x{size:x}" if isinstance(size, int) else "",
                    r.get("joined", r["name"]), r["kind"], r["channel"],
                    r["name"], "1" if r.get("dtor") else "",
                    r.get("ckind", ""), r.get("owner", "")])
    return out


def src_files() -> list:
    """The extraction universe: regular C/C++ files, one unit per stem.
    A stem collision would silently merge two files' claims into one
    fragment - fatal, has never existed. Directories such as a transient
    ``src/.codex`` workspace must never enter the source universe."""
    paths = sorted(p for p in SRC_DIR.iterdir()
                   if p.is_file() and p.suffix.lower() in SOURCE_SUFFIXES)
    stems = [p.stem for p in paths]
    for stem in stems:
        if stems.count(stem) > 1:
            common.die(f"src/ stem collision: two files share unit {stem!r}")
    return paths


def _extract_one(path, functions: set, ir_names: dict | None,
                 problems: list[str]) -> list[dict]:
    """One unit's rows, IR-bound where clang reached the TU and lexically
    joined for the rest."""
    unit = path.stem
    rows = scan_file(path, functions, problems)
    taken = set()
    if ir_names is not None:
        taken = ir_bind(unit, rows, ir_names, problems)
    join_unit(unit, rows, taken)
    return rows


def run(only_units: list[str] | None = None,
        jobs: int | None = None) -> tuple[list[str], list[str], list[str]]:
    """Extract fragments; returns (changed units, pruned fragments,
    problems). Fragment writes are content-idempotent so an unchanged TU
    never dirties downstream freshness probes.

    The clang probes are independent per TU and each costs about 0.4 s, so
    they run in a thread pool; everything after them is per-unit local."""
    from concurrent.futures import ThreadPoolExecutor

    paths = src_files()
    known = {p.stem for p in paths}
    if only_units is not None:
        for name in only_units:
            if name not in known:
                raise SystemExit(f"[labels] unknown unit {name!r} - units "
                                 f"are src/ file stems, e.g. 'advmgr'")
    functions = {r["rva"] for r in _census_functions()}
    todo = [p for p in paths
            if only_units is None or p.stem in only_units]
    changed, pruned, problems = [], [], []

    if clang.clang_bin() is None:
        problems.append("clang is not on PATH and $HOMM3_CLANG is unset - "
                        "every claim falls back to the lexical declarator "
                        "join; run inside `nix develop`")
        ir_maps = [None] * len(todo)
    else:
        clang.mirror()          # once, before the pool shares it
        workers = jobs or min(16, (os.cpu_count() or 4))
        with ThreadPoolExecutor(max_workers=workers) as pool:
            ir_maps = list(pool.map(unit_ir_names, todo))

    no_ir = []
    for path, ir_names in zip(todo, ir_maps):
        if ir_names is None:
            no_ir.append(path.stem)
        rows = _extract_one(path, functions, ir_names, problems)
        banner = [f"# GENERATED claim fragment for unit {path.stem} - the "
                  f"macros in src/{path.name} are the storage; do not edit."]
        if write_tsv(fragment_path(path.stem), banner, HEADER,
                     _fragment_rows(rows)):
            changed.append(path.stem)
    if no_ir:
        problems.append(
            f"{len(no_ir)} TU(s) clang could not read, so their claims keep "
            f"the lexical declarator join: {', '.join(sorted(no_ir))}")
    if only_units is None and FRAGMENTS.is_dir():
        for stale in sorted(FRAGMENTS.glob("*.tsv")):
            if stale.stem not in known:
                stale.unlink()
                pruned.append(stale.stem)
    return changed, pruned, problems


MACRO_SITE_RE = re.compile(
    r"\b(VA_COMPGEN|DATA_COMPGEN_GUARD|DATA_COMPGEN|VA|DATA)"
    r"\s*\(\s*(0x[0-9a-fA-F]+)")


def sweep_sites() -> dict:
    """Tree-wide macro-site census over src/ + include/ (comments blanked,
    va.h's own #defines excluded): {macro: {rva: ['file:line', ...]}}."""
    out: dict = {}
    for base in ("src", "include"):
        root = common.HOMM3_DIR / base
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".c", ".cpp", ".cxx", ".h") \
                    or path.name == "va.h":
                continue
            text = mask_lexical_noise(path.read_text(errors="replace"))
            for m in MACRO_SITE_RE.finditer(text):
                lineno = text.count("\n", 0, m.start()) + 1
                rva = int(m.group(2), 16) - common.IMAGE_BASE
                out.setdefault(m.group(1), {}).setdefault(rva, []).append(
                    f"{path.relative_to(common.HOMM3_DIR)}:{lineno}")
    return out


#: What each macro claims, and which fragment `kind` must carry it.
SITE_KINDS = (("VA", "func"), ("VA_COMPGEN", "func"), ("DATA", "data"),
              ("DATA_COMPGEN", "data"), ("DATA_COMPGEN_GUARD", "data"))

#: The enumerated floor: header VA() sites this sweep reports as standing
#: DEBT instead of a fatal loss. One entry, keyed on rva, each with the
#: reason it is still open. Any OTHER header VA() is fatal; a floor entry
#: whose site has gone is reported as stale, so the floor cannot quietly
#: outlive its cause. It is code, not a config table, because a single
#: entry's justification is prose that belongs beside the rule.
HEADER_VA_FLOOR = {}


def pooled_sites() -> dict:
    """{(macro, rva): {unit: (name, value, 'file:line')}} - the FIRST
    site per unit of every pooled compgen macro in src/*.c*.

    Within-unit repeats are already adjudicated by `scan_file`; this is
    the CROSS-unit view, which no single fragment can see."""
    out: dict = {}
    for path in src_files():
        raw = path.read_text(errors="replace")
        text = mask_lexical_noise(raw)
        line_starts = _line_starts(text)
        for macro in ("DATA_COMPGEN", "DATA_COMPGEN_GUARD"):
            head, arity, _proto = MACRO_HEADS[macro]
            for start, end, args, raw_args in macro_invocations(
                    text, head, raw):
                if end is None or len(args) != arity \
                        or not ADDR_ARG_RE.match(args[0]):
                    continue          # scan_file owns the fatal for these
                rva = int(args[0], 16) - common.IMAGE_BASE
                where = (f"{path.name}:"
                         f"{bisect.bisect_right(line_starts, start)}")
                out.setdefault((macro, rva), {}).setdefault(
                    path.stem, (args[1], raw_args[2], where))
    return out


def pooled_agreement_problems(sites: dict) -> list[str]:
    """Cross-TU disagreements over the pooled compiler-generated data.

    The model COALESCES these claims - one pooled literal is a single
    image-wide allocation that every TU spelling it claims - and it does
    so blind, because a fragment carries no value. That coalesce is only
    honest if somebody proves the claims agree, and this is that
    somebody: extraction is the one stage that can still see the values.
    Two units pinning one address to two different VALUES is a defect the
    model would otherwise swallow silently."""
    problems = []
    for (macro, rva), by_unit in sorted(sites.items()):
        if len(by_unit) < 2:
            continue
        units = sorted(by_unit)      # all_claims() reads units stem-sorted,
        owner = units[0]             # so the first one is what the model keeps
        name, value, where = by_unit[owner]
        va = rva + common.IMAGE_BASE
        for unit in units[1:]:
            other_name, other_value, other_where = by_unit[unit]
            if other_value != value:
                problems.append(
                    f"{macro}(0x{va:08x}) pins {other_value!r} at "
                    f"{other_where} but {value!r} at {where} - one address, "
                    f"two values; the model coalesces these claims and "
                    f"cannot see the disagreement (FATAL)")
            elif other_name != name:
                problems.append(
                    f"{macro}(0x{va:08x}) is {other_name!r} in {unit} and "
                    f"{name!r} in {owner}; the value agrees, so one pooled "
                    f"literal serves both roles - {name!r} wins ({owner} is "
                    f"first in scan order)")
    return problems


def completeness_problems(sites: dict, have: dict,
                          floor: dict | None = None) -> list[str]:
    """The oracle OVER extraction, as a pure function of (macro-site
    census, fragment coverage) so the selftest can drive it.

    `sites` is `sweep_sites()`; `have` is {kind: {rva}} over every
    fragment claim. Returns the problems; a line tagged (FATAL) is a lost
    label.

    TWO CHANNELS, and the split is the whole point of this gate:

    src/*.c* SITES belong to EXTRACTION. Every one must reach a fragment.
    A macro that is present, valid and intended but reaches nothing is
    the failure this gate exists for - it was real (18 rvas, wrapped by
    clang-format across lines, dropped by a per-line regex scan) and it
    was silent.

    HEADER SITES belong to a different channel, and are enumerated here
    rather than counted as losses:

      DATA() on a header extern is a DECLARATION-SITE annotation. The
      datum is retail's, not this build's - none of the 100 such externs
      has a definition anywhere in src/, because our source only
      REFERENCES them - so there is no storage for extraction to own and
      no TU to own it. Nor would extraction improve them: the src-DATA
      channel names every claim `data_<rva>`, the same grade of dense
      working label these addresses already carry from the reloc-target
      channel, and a worse spelling for the .rdata ones (`const_<rva>` is
      section-correct). Their real channel is the INVENTORY, so the model
      owns their gate - `homm3.model` fails if a header DATA() address
      reaches no data row - and this sweep leaves them alone. Measured
      2026-08-20: 101 header DATA() addresses, 101 already carried
      (99 reloc-target, 1 reloc-alias, 1 src-DATA_COMPGEN).

      VA() in a header is NOT excused. A function address always gets a
      row (the working-label pass covers the whole universe), so the
      model cannot tell that the SOURCE NAME was dropped - only this
      sweep can. The claim belongs in the owning TU's `#if 0 //
      @carcass` block, where 160 claim-only stubs already put the retail
      functions this build does not define; there it reaches a fragment
      and keeps its declarator name. `floor` enumerates the sites this
      sweep still reports as standing debt rather than a fatal loss.
    """
    floor = HEADER_VA_FLOOR if floor is None else floor
    problems = []
    seen_floor = set()
    for macro, kind in SITE_KINDS:
        for rva, wheres in sorted(sites.get(macro, {}).items()):
            if rva in have.get(kind, set()):
                continue
            header_only = all(":" in w and w.split(":")[0].endswith(".h")
                              for w in wheres)
            if macro == "DATA" and header_only:
                continue                  # the model's gate, by doctrine
            va = rva + common.IMAGE_BASE
            if header_only and rva in floor:
                seen_floor.add(rva)
                problems.append(
                    f"{macro}(0x{va:08x}) at {wheres[0]} is in a HEADER and "
                    f"names nothing - KNOWN, enumerated: {floor[rva]}")
            elif header_only:
                problems.append(
                    f"{macro}(0x{va:08x}) at {wheres[0]} is in a HEADER, "
                    f"which extraction does not read - move the claim to the "
                    f"owning TU's @carcass block or it names nothing (FATAL)")
            else:
                problems.append(
                    f"{macro}(0x{va:08x}) at {wheres[0]} is in NO fragment - "
                    f"a present, valid, intended macro that contributed "
                    f"nothing (FATAL)")
    for rva in sorted(set(floor) - seen_floor):
        problems.append(
            f"0x{rva + common.IMAGE_BASE:08x} is enumerated in "
            f"HEADER_VA_FLOOR but no header site claims it any more - the "
            f"debt is paid, delete the entry")
    return problems


def check_completeness() -> list[str]:
    """`completeness_problems` + `pooled_agreement_problems` over the
    real tree."""
    from homm3.retail_labels.fragments import all_claims
    have: dict = {}
    for claim in all_claims():
        have.setdefault(claim.kind, set()).add(claim.rva)
    return (completeness_problems(sweep_sites(), have)
            + pooled_agreement_problems(pooled_sites()))


# --- the embedded negative control ------------------------------------

#: One synthetic TU carrying, in order: an unwrapped DATA_COMPGEN, the
#: SAME claim wrapped the way clang-format wraps it, a wrapped
#: DATA_COMPGEN_GUARD, a wrapped VA_COMPGEN, and two decoys - a macro
#: inside a block comment and one inside a string literal. Every wrapped
#: form here is copied from a real site the per-line scanner dropped
#: (advmgr.cpp:919, herodefs.cpp:78, cmbtmgr.cpp:483).
_SELFTEST_TU = '''
    sprintf(gText, DATA_COMPGEN(0x00660000, flat, "%s, %s"), a, b);
    sprintf(gText, DATA_COMPGEN(
        0x00660004, wrapped, "%s, %s"),
        a, b);
    DATA_COMPGEN_GUARD(0x00660008, guard,
                      owner)
    VA_COMPGEN(0x0040100c, 0x10,
               STATIC_INIT_DISPATCH,
               someOwner)
    /* DATA_COMPGEN(0x00660010, inComment, "x") */
    puts("DATA_COMPGEN(0x00660014, inString, \\"x\\")");
'''


def selftest() -> list[str]:
    """Synthetic defects that MUST be detected + clean samples that MUST
    pass. Runs on every gate invocation: the gate proves it can still
    fail before it judges the tree (this repo's rule - a gate shipping
    without a negative control proves nothing)."""
    failures = []
    masked = mask_lexical_noise(_SELFTEST_TU)

    def calls(macro):
        head, _arity, _proto = MACRO_HEADS[macro]
        return macro_invocations(masked, head, _SELFTEST_TU)

    # 1. the defect this gate exists for: a WRAPPED macro must be read
    #    exactly like the flat one. A per-line regex scan finds only the
    #    flat site, so a scanner that regressed to one scores 1 here.
    dc = calls("DATA_COMPGEN")
    if len(dc) != 2:
        failures.append(f"wrapped DATA_COMPGEN not scanned "
                        f"({len(dc)} of 2 sites found)")
    else:
        # masked args carry the address and the name; the VALUE survives
        # only in the raw ones (see macro_invocations)
        got = [(a[0], a[1], r[2]) for _s, _e, a, r in dc]
        if got != [("0x00660000", "flat", '"%s, %s"'),
                   ("0x00660004", "wrapped", '"%s, %s"')]:
            failures.append(f"wrapped DATA_COMPGEN differs from the flat "
                            f"spelling: {got}")
    if len(calls("DATA_COMPGEN_GUARD")) != 1:
        failures.append("wrapped DATA_COMPGEN_GUARD not scanned")
    if len(calls("VA_COMPGEN")) != 1:
        failures.append("wrapped VA_COMPGEN not scanned")

    # 2. the decoys stay invisible - blanking must survive the rewrite
    if any("inComment" in a or "inString" in a
           for _s, _e, args, _r in dc for a in args):
        failures.append("a macro in a comment or string literal was scanned")

    # 3. argument splitting is structural, not textual
    if _split_top_level('0x1, n, f(a, b)') != ["0x1", "n", "f(a, b)"]:
        failures.append("a comma inside a nested call split an argument")
    if _split_top_level('0x1, n, "a, b"') != ["0x1", "n", '"a, b"']:
        failures.append("a comma inside a string literal split an argument")

    # Special-name joins used by claim-only compiler-generated rows. In
    # particular, ??_D is MSVC's vbase destructor rather than an ordinary
    # destructor (??1), so collapsing the two loses the ostrstream closure.
    if _demangle_key("??_Dostrstream@std@@QAEXXZ") != \
            "ostrstream__vbase_destructor":
        failures.append("MSVC vbase destructor did not get its distinct key")
    if _demangle_key(
            "??_D?$basic_ostringstream@DU?$char_traits@D@std@@"
            "V?$allocator@D@2@@std@@QAEXXZ") != \
            "basic_ostringstream__vbase_destructor":
        failures.append("template vbase destructor owner was not normalized")
    if _demangle_key(
            "?GetCount@?$CAutoArray@VCDPlayAddressElement@@@@UAEKXZ") != \
            "cautoarray_getcount":
        failures.append("global class-template member key regressed")
    if _demangle_key(
            "??0?$CAutoArray@VCDPlayAddressElement@@@@QAE@XZ") != \
            "cautoarray_cautoarray":
        failures.append("global class-template constructor key regressed")
    if _demangle_key(
            "??1?$CAutoArray@VCDPlayAddressElement@@@@UAE@XZ") != \
            "cautoarray_cautoarray@dtor":
        failures.append("global class-template destructor key regressed")
    if _demangle_key("??_Gios_base@std@@UAEPAXI@Z") != \
            "ios_base_ios_base@gdtor":
        failures.append("MSVC scalar deleting destructor key regressed")
    if _demangle_key("??_ENewmapCell@@QAEPAXI@Z") != \
            "newmapcell_newmapcell@vdtor":
        failures.append("MSVC vector deleting destructor key regressed")
    if _demangle_key(
            "??_F?$vector@VCObjectType@@V?$allocator@VCObjectType@@@std@@"
            "@std@@QAEXXZ") != "cobjecttype@fctor":
        failures.append("MSVC default constructor closure key regressed")
    if _demangle_key(
            "??_F?$vector@PAVtype_artifact_effect@@"
            "V?$allocator@PAVtype_artifact_effect@@@std@@@std@@QAEXXZ") \
            != "type_artifact_effect@fctor":
        failures.append(
            "MSVC pointer-vector constructor closure key regressed")
    if _demangle_key(
            "??1?$vector@VBlackBoxData@@V?$allocator@VBlackBoxData@@@std@@"
            "@std@@QAE@XZ") != "blackboxdata@vector_dtor":
        failures.append("MSVC vector destructor key regressed")
    if _demangle_key(
            "??1?$pair@$$CBHUtype_map_hero_info@@@std@@QAE@XZ") != \
            "type_map_hero_info@pair_const_int_dtor":
        failures.append("MSVC pair<const int, T> destructor key regressed")
    if _demangle_key(
            "?size@?$vector@VCObjectType@@V?$allocator@VCObjectType@@@std@@"
            "@std@@QBEIXZ") != "cobjecttype@vector_size":
        failures.append("MSVC vector size key regressed")
    if _demangle_key(
            "?resize@?$vector@VTSeerHut@@V?$allocator@VTSeerHut@@@std@@"
            "@std@@QAEXIABVTSeerHut@@@Z") != "tseerhut@vector_resize":
        failures.append("MSVC vector resize key regressed")
    if _demangle_key(
            "?insert@?$vector@VBlackBoxData@@V?$allocator@VBlackBoxData@@"
            "@std@@@std@@QAEXPAVBlackBoxData@@IABV3@@Z") != \
            "blackboxdata@vector_insert":
        failures.append("MSVC vector insert key regressed")
    if _demangle_key(
            "?erase@?$vector@VTQuestGuard@@V?$allocator@VTQuestGuard@@@std"
            "@@@std@@QAEPAVTQuestGuard@@PAV3@0@Z") != \
            "tquestguard@vector_erase":
        failures.append("MSVC vector erase key regressed")
    if _demangle_key(
            "?erase@?$vector@V?$vector@Vhero@@V?$allocator@Vhero@@@std@@"
            "@std@@V?$allocator@V?$vector@Vhero@@V?$allocator@Vhero@@"
            "@std@@@std@@@2@@std@@") != "hero_vector@vector_erase":
        failures.append("MSVC nested-vector erase key regressed")
    if _demangle_key(
            "?size@?$vector@V?$vector@Vhero@@V?$allocator@Vhero@@@std@@"
            "@std@@V?$allocator@V?$vector@Vhero@@V?$allocator@Vhero@@"
            "@std@@@std@@@2@@std@@QBEIXZ") != \
            "hero_vector@vector_size":
        failures.append("MSVC nested-vector size key regressed")
    if _demangle_key(
            "?insert@?$vector@V?$vector@Utype_artifact@@"
            "V?$allocator@Utype_artifact@@@std@@@std@@"
            "V?$allocator@V?$vector@Utype_artifact@@"
            "V?$allocator@Utype_artifact@@@std@@@std@@@2@@std@@"
            "QAEXPAV?$vector@Utype_artifact@@"
            "V?$allocator@Utype_artifact@@@std@@@2@IABV32@@Z") != \
            "type_artifact_vector@vector_insert":
        failures.append("MSVC nested-vector insert key regressed")
    if _demangle_key(
            "?capacity@?$vector@USecondarySkillData@@V?$allocator@USecondarySkillData@@"
            "@std@@@std@@QBEIXZ") != "secondaryskilldata@vector_capacity":
        failures.append("MSVC vector capacity key regressed")
    if _demangle_key("??_H@YGXPAXIHP6EX0@Z@Z") != \
            "vector_constructor_iterator":
        failures.append("MSVC vector constructor iterator key regressed")
    if _demangle_key("?_Tidy@?$bitset@$09@std@@AAEXK@Z") != \
            "bitset10@bitset_tidy":
        failures.append("MSVC bitset<10> _Tidy key regressed")
    bitset_cases = {
        "??0?$bitset@$0BM@@std@@QAE@K@Z": "bitset28@bitset_ctor",
        "??A?$bitset@$0JB@@std@@QAE?AVreference@01@I@Z":
            "bitset145@bitset_subscript",
        "??D?$bitset_iterator@$0JB@@@QBE?AVreference@?$bitset@$0JB@@std@@XZ":
            "bitset145@bitset_iterator_deref",
        "??4reference@?$bitset@$04@std@@QAEAAV012@_N@Z":
            "bitset5@bitset_reference_assign",
        "?flip@?$bitset@$0BM@@std@@QAEAAV12@XZ":
            "bitset28@bitset_flip",
        "?count@?$bitset@$0BM@@std@@QBEIXZ": "bitset28@bitset_count",
        "?any@?$bitset@$0BM@@std@@QBE_NXZ": "bitset28@bitset_any",
        "?set@?$bitset@$0JB@@std@@QAEAAV12@I_N@Z":
            "bitset145@bitset_set",
        "?test@?$bitset@$07@std@@QBE_NI@Z": "bitset8@bitset_test",
        "?_Xran@?$bitset@$0BM@@std@@ABEXXZ": "bitset28@bitset_xran",
    }
    for mangled, expected in bitset_cases.items():
        if _demangle_key(mangled) != expected:
            failures.append(f"MSVC {expected} key regressed")
    if _demangle_key(
            "?_Min@?$_Tree@HU?$pair@$$CBHUtype_map_hero_info@@@std@@"
            "U_Kfn@?$map@HUtype_map_hero_info@@U?$less@H@std@@"
            "V?$allocator@Utype_map_hero_info@@@3@@2@U?$less@H@2@"
            "V?$allocator@Utype_map_hero_info@@@2@@std@@KAPAU_Node@12@"
            "PAU312@@Z") != "type_map_hero_info@tree_min":
        failures.append("MSVC map tree _Min key regressed")
    tree_member_cases = {
        "?insert@?$_Tree@HU?$pair@$$CBHUtype_map_hero_info@@@std@@":
            "type_map_hero_info@tree_insert",
        "?_Insert@?$_Tree@HU?$pair@$$CBHUtype_map_hero_info@@@std@@":
            "type_map_hero_info@tree_node_insert",
        "?_Dec@const_iterator@?$_Tree@HU?$pair@$$CBHUtype_map_hero_info@@@std@@":
            "type_map_hero_info@tree_const_iterator_dec",
        "?_Inc@const_iterator@?$_Tree@HU?$pair@$$CBHUtype_map_hero_info@@@std@@":
            "type_map_hero_info@tree_const_iterator_inc",
        "?_Inc@const_iterator@?$_Tree@UTCacheMapKey@ResourceManager@@"
        "U?$pair@$$CBUTCacheMapKey@ResourceManager@@PAVresource@@@std@@":
            "tcachemapkey@tree_const_iterator_inc",
        "?_Inc@const_iterator@?$_Tree@UTPoint@@UTPoint@@":
            "tpoint@tree_const_iterator_inc",
        "?_Inc@const_iterator@?$_Tree@V?$basic_string@D"
        "U?$char_traits@D@std@@V?$allocator@D@2@@std@@V12@":
            "string@tree_const_iterator_inc",
        "?_Inc@const_iterator@?$_Tree@PAVCImmEnclosure@@"
        "U?$pair@QAVCImmEnclosure@@UtagRECT@@@std@@":
            "cimmenclosure@tree_const_iterator_inc",
    }
    for mangled, expected in tree_member_cases.items():
        if _demangle_key(mangled) != expected:
            failures.append(f"MSVC {expected} key regressed")
    if _demangle_key(
            "?put@?$basic_ostream@DU?$char_traits@D@std@@@std@@"
            "QAEAAV12@D@Z") != "char@ostream_put":
        failures.append("MSVC basic_ostream<char>::put key regressed")
    if _demangle_key(
            "??6std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@"
            "AAV10@PBD@Z") != "char@ostream_insert_cstr":
        failures.append("MSVC ostream<char> c-string insertion key regressed")
    if _demangle_key(
            "?_Insertion_sort_1@std@@YIXPAPAX0V"
            "CampaignHeaderPointerLess@@0@Z") != \
            "campaignheaderpointerless@insertion_sort_1":
        failures.append("MSVC campaign pointer insertion-sort key regressed")
    if _demangle_key(
            "?xsputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@"
            "MAEHPBDH@Z") != "char@streambuf_xsputn":
        failures.append("MSVC basic_streambuf<char>::xsputn key regressed")
    if _demangle_key(
            "?assign@?$basic_string@DU?$char_traits@D@std@@"
            "V?$allocator@D@2@@std@@QAEAAV12@PBDI@Z") != \
            "char@basic_string_assign_ptr_size":
        failures.append("MSVC basic_string<char>::assign(ptr,size) key regressed")
    unrelated_tree_key = _demangle_key(
        "?_Erase@?$_Tree@HU?$pair@$$CBHUtype_map_hero_info@@@std@@")
    if unrelated_tree_key in tree_member_cases.values():
        failures.append("uncontracted MSVC tree member gained a tree key")
    if _demangle_key(
            "?_Destroy@?$vector@VTTimedEvent@@V?$allocator@VTTimedEvent@@"
            "@std@@@std@@IAEXPAVTTimedEvent@@0@Z") != \
            "ttimedevent@vector_destroy":
        failures.append("MSVC vector _Destroy key regressed")
    if _demangle_key(
            "?_Ucopy@?$vector@VTSeerHut@@V?$allocator@VTSeerHut@@@std@@"
            "@std@@IAEPAVTSeerHut@@PBV3@0PAV3@@Z") != \
            "tseerhut@vector_ucopy":
        failures.append("MSVC vector _Ucopy key regressed")
    if _demangle_key(
            "?_Ufill@?$vector@VTQuestGuard@@V?$allocator@VTQuestGuard@@"
            "@std@@@std@@IAEXPAVTQuestGuard@@IABV3@@Z") != \
            "tquestguard@vector_ufill":
        failures.append("MSVC vector _Ufill key regressed")
    if _demangle_key(
            "??4?$vector@W4TArtifact@@V?$allocator@W4TArtifact@@@std@@@std"
            "@@QAEAAV01@ABV01@@Z") != "tartifact@vector_copy_assign":
        failures.append("MSVC vector copy-assignment key regressed")
    if _demangle_key(
            "?_Construct@std@@YIXPAVMonsterData@@ABV2@@Z") != \
            "monsterdata@std_construct":
        failures.append("MSVC std::_Construct key regressed")
    if _demangle_key(
            "?copy@std@@YIPAUtype_university@@PAU2@00@Z") != \
            "type_university@std_copy":
        failures.append("MSVC std::copy key regressed")
    if _demangle_key("?copy@std@@YIPAHPAH00@Z") != "int@std_copy":
        failures.append("MSVC std::copy<int> key regressed")
    if _demangle_key("?copy@std@@YIPAHPBH0PAH@Z") != \
            "const_int@std_copy":
        failures.append("MSVC std::copy<const int> key regressed")
    if _demangle_key("??4MonsterData@@QAEAAV0@ABV0@@Z") != \
            "monsterdata_monsterdata_operator":
        failures.append("MSVC implicit copy-assignment key regressed")
    source_equal = OPERATOR_EQUAL_RE.search(
        "bool type_point::operator==(const type_point& arg) const")
    if source_equal is None:
        failures.append("source operator== declarator was not recognized")
    else:
        source_equal_key = IDENT_RE.sub(
            "_", f"{source_equal.group(1)}::operator_equal").strip("_")
        if source_equal_key.lower() != "type_point_operator_equal":
            failures.append("source operator== key regressed")
    if _demangle_key("??8type_point@@QBE_NABU0@@Z") != \
            "type_point_operator_equal":
        failures.append("MSVC operator== key regressed")
    source_not_equal = OPERATOR_NOT_EQUAL_RE.search(
        "bool type_point::operator!=(const type_point& arg) const")
    if source_not_equal is None:
        failures.append("source operator!= declarator was not recognized")
    else:
        source_not_equal_key = IDENT_RE.sub(
            "_", f"{source_not_equal.group(1)}::operator_not_equal").strip("_")
        if source_not_equal_key.lower() != "type_point_operator_not_equal":
            failures.append("source operator!= key regressed")
    if OPERATOR_EQUAL_RE.search(
            "type_point& type_point::operator=(const type_point& arg)"):
        failures.append("source operator= was confused with operator==")
    if _demangle_key("??9type_point@@QBE_NABU0@@Z") != \
            "type_point_operator_not_equal":
        failures.append("MSVC operator!= key regressed")
    if OPERATOR_NOT_EQUAL_RE.search(
            "type_point& type_point::operator=(const type_point& arg)"):
        failures.append("source operator= was confused with operator!=")
    if _demangle_key("??8other_point@@QBE_NABU0@@Z") == \
            "type_point_operator_equal":
        failures.append("MSVC operator== key ignored its owning class")
    if _demangle_key(
            "??0logic_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@"
            "std@@V?$allocator@D@2@@1@@Z") != "logic_error_logic_error":
        failures.append("MSVC named class constructor key regressed")
    if _demangle_key("??1TreasureData@@QAE@XZ") != \
            "treasuredata_treasuredata@dtor":
        failures.append("MSVC implicit destructor key regressed")

    # 4. raw arguments must survive masking: two DIFFERENT literals of
    #    equal length mask to the same blanks, so a pooled-agreement
    #    check reading masked values would call them equal
    pair = '''DATA_COMPGEN(0x00660018, a, "%s %s")
              DATA_COMPGEN(0x00660018, b, "%d %d")'''
    head, _arity, _proto = MACRO_HEADS["DATA_COMPGEN"]
    got = macro_invocations(mask_lexical_noise(pair), head, pair)
    if len({r[2] for _s, _e, _a, r in got}) != 2:
        failures.append("masked values hid two different string literals")

    # 5. the completeness oracle, driven directly
    src_site = {"DATA_COMPGEN": {0x260000: ["src/t.cpp:1"]}}
    if completeness_problems(src_site, {"data": {0x260000}}, floor={}):
        failures.append("clean sample did not pass")
    if not any("FATAL" in p for p in
               completeness_problems(src_site, {"data": set()}, floor={})):
        failures.append("a lost src site was not detected")
    header_va = {"VA": {0xbbb60: ["include/game.h:753"]}}
    if not any("FATAL" in p for p in completeness_problems(
            header_va, {"func": set()}, floor={})):
        failures.append("a header VA site was not detected")
    # the enumerated floor: reported as debt, never fatal, never silent
    got = completeness_problems(header_va, {"func": set()},
                                floor={0xbbb60: "known"})
    if not got or any("FATAL" in p for p in got):
        failures.append("a floored header VA must be reported, and not fatal")
    if not completeness_problems({}, {}, floor={0xbbb60: "known"}):
        failures.append("a stale floor entry was not detected")
    if any("FATAL" in p for p in completeness_problems(
            header_va, {"func": {0xbbb60}}, floor={})):
        failures.append("a header VA already carried was wrongly fatal")
    if completeness_problems({"DATA": {0x298b20: ["include/hero.h:916"]}},
                             {"data": set()}, floor={}):
        failures.append("a header DATA site was wrongly reported as lost")
    if not any("FATAL" in p for p in completeness_problems(
            {"DATA": {0x298b20: ["include/hero.h:916", "src/hero.cpp:12"]}},
            {"data": set()}, floor={})):
        failures.append("a src DATA site was excused by a header twin")

    # 6. pooled agreement - the fact the model's coalesce rests on
    agree = {("DATA_COMPGEN", 0x260000): {
        "advmgr": ("x", '"%s %s"', "advmgr.cpp:1"),
        "events": ("x", '"%s %s"', "events.cpp:2")}}
    if pooled_agreement_problems(agree):
        failures.append("agreeing pooled claims were reported")
    clash = {("DATA_COMPGEN", 0x260000): {
        "advmgr": ("x", '"%s %s"', "advmgr.cpp:1"),
        "events": ("x", '"%d %d"', "events.cpp:2")}}
    if not any("FATAL" in p for p in pooled_agreement_problems(clash)):
        failures.append("two values on one pooled address not detected")
    renamed = {("DATA_COMPGEN", 0x260000): {
        "advmgr": ("x", '"%s %s"', "advmgr.cpp:1"),
        "events": ("y", '"%s %s"', "events.cpp:2")}}
    got = pooled_agreement_problems(renamed)
    if not got or any("FATAL" in p for p in got):
        failures.append("a pooled rename must be reported, and not fatal")
    return failures


def _census_functions():
    from homm3.retail_labels import censuses
    return censuses.functions()


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm3 labels", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", action="append",
                    help="extract one unit (repeatable)")
    ap.add_argument("--all", action="store_true",
                    help="extract every src/ unit")
    ap.add_argument("-j", "--jobs", type=int, default=None,
                    help="clang probe parallelism (default: cpu count)")
    ap.add_argument("--selftest", action="store_true",
                    help="run the completeness gate's negative control "
                         "and exit")
    a = ap.parse_args(argv)
    if a.selftest:
        broken = selftest()
        for line in broken:
            print(f"SELFTEST BROKEN: {line}", file=sys.stderr)
        print("selftest OK" if not broken else "selftest FAILED")
        return 2 if broken else 0
    if not a.unit and not a.all:
        ap.error("pick --unit U or --all")
    changed, pruned, problems = run(a.unit if not a.all else None, a.jobs)
    fatal = []
    if a.unit is None:
        # The gate proves it can fail before it judges the tree.
        broken = selftest()
        if broken:
            fatal += [f"completeness SELFTEST BROKEN: {b}" for b in broken]
        else:
            problems = problems + check_completeness()
            fatal += [p for p in problems if "FATAL" in p]
    for problem in problems:
        print(f"[labels] {problem}", file=sys.stderr)
    if fatal:
        print(f"[labels] {len(fatal)} lost label(s) - a macro site that "
              f"reaches no claim names nothing", file=sys.stderr)
        return 1
    print(f"[labels] {len(changed)} fragment(s) changed"
          + (f", {len(pruned)} pruned" if pruned else "")
          + f" -> {FRAGMENTS.relative_to(common.HOMM3_DIR)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
