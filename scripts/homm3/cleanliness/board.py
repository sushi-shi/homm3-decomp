#!/usr/bin/env python3
"""homm3.cleanliness.board - the source-quality scoreboard + down-only ratchet.

The gruntz board ported at today's size: metrics counted over src/ +
include/ with comments and string/char literals stripped first (so the
extensive `//` annotations and literals never inflate a count), committed
floors in config/cleanliness-baseline.tsv, and a RATCHET rule - a metric
may only go DOWN. `homm3 build` dies when a ratcheted metric rises above
its floor, and rolls floors with min(count, floor) so a regression stays
visible instead of being blessed. `board --update` is the one deliberate
way to move a floor - lower by fixing the tree, higher only as an
explicit, reviewed act.

The rows (all ratcheted; floors start at the tree's current counts):

  C-style casts       BANNED at floor 0 (gruntz cast-metric-policy):
                      every cast is a named C++ cast. Shapes:
                      numeric/builtin casts, pointer casts, casts applied
                      to `this`. Known gap (accepted, as in gruntz): a
                      value cast to a bare class/enum name `(eHero)x` is
                      regex-indistinguishable from parenthesized
                      expressions - caught in review.
  reinterpret_casts   named-cast DEBT, ratcheted down: each one is often
                      the only lexical evidence of an integer carrier or
                      a mis-modeled member view. static_cast is free.
  volatile qualifiers BANNED at floor 0. `volatile` is not an admissible
                      way to home a temporary, perturb VC6's register
                      allocator, retain a dead store, or spend an inline
                      budget. Recover the real helper/type/lifetime first;
                      an inline-depth probe may diagnose an over-expansion,
                      but it must not be retained. A Dreamcast line gap plus
                      a meaningful release-elided invariant may use
                      HOMM3_RELEASE_VERIFY; a gap by itself is not proof.
  inline-gate         BANNED at floor 0. `INLINE_GATE(...)` is a no-op marker
  artifacts           invented by the decompilation, not original source.
  inline-depth pins   source-false compiler-steering debt. Local experiments
                      are useful, but committed pins only ratchet down while
                      the natural declaration/TU/compiler state is recovered.
  cpp extern decls    a declaration re-spelled in a consumer .cpp instead
                      of living in its OWNER's header. Fix: declare once
                      in the owner header and #include it.
  .cpp-local views    a struct/class DEFINITION inside a .cpp is a
                      per-TU view of a class whose one true shape belongs
                      in a header. Fix: move the definition to include/.
  .cpp-local enums    the enum twin: a cross-TU domain stranded in a .cpp
                      gets re-declared by the next TU that needs it.
  casts to enum types static_cast<E>(...) where E is any enum declared in
                      the tree. An enum-to-enum cast (and its lexically
                      indistinguishable int-to-enum sibling) usually
                      means two mis-modeled domains. A single cast on a line
                      may carry the narrow
                      `HOMM3_ENUM_CAST_REVISION_BOUNDARY` admission when a
                      retail-only ordinal must cross a DC-proven enum ABI;
                      every ordinary cast remains ratcheted at zero.
  magic case labels   a `case <number>:` is an un-named member of some
                      domain. Fix: declare the enum, case on enumerators.
  unnamed domain      `x == 0x36` - the comparison twin of magic case
  compares            labels. `== 0`/`== 1` are exempt (null/bool tests).
  view preprocessors  per-TU reconstruction scaffolds whose directive
                      identifier ends in `_VIEW`. Recovered declarations,
                      helpers, layouts and source order become canonical;
                      this debt ratchets to zero and may never rise.
  per-TU              the same scaffold under any other suffix: an
  preprocessor        object-like `#define HOMM3_X` at the top of a .cpp
  scaffolds           paired with `#if defined(HOMM3_X)` / `#ifdef` forks
                      around declarations, fields, helpers, enums,
                      layouts, friends or inline-ness so only chosen TUs
                      see a recovered fact. The rule, precisely: any
                      identifier `HOMM3_[A-Z0-9_]+` inside an #if /
                      #ifdef / #ifndef / #elif / #define / #undef logical
                      directive, EXCEPT include guards (`..._H`),
                      function-like macro names (any `#define NAME(`
                      anywhere in the tree, so a use of
                      HOMM3_MAKE_DPLAY_ERROR inside a DPERR_* constant is
                      not a scaffold) and whatever include/va.h defines
                      (the annotation/verify machinery). Ratchets to zero.

Every invocation self-tests first: each metric's embedded positive
samples must be detected and its negatives must count zero, so the gate
proves it can still fail before it judges the tree.
"""
from __future__ import annotations

import re
import sys

from homm3.core import common

REPO = common.HOMM3_DIR
ROOTS = ("src", "include")
EXTS = {".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}
_CPP = {".c", ".cpp", ".cc", ".cxx"}
BASELINE = REPO / "config/cleanliness-baseline.tsv"

_BLOCK = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE = re.compile(r"//[^\n]*")
_STR = re.compile(r'"(?:\\.|[^"\\\n])*"')
# Char literals must CLOSE within a few chars: the carcass is full of MSVC
# backtick names (`scalar deleting destructor'(unsigned __flags)) whose
# lone apostrophe must not swallow the rest of the line (the labels.py
# masker lesson).
_CHR = re.compile(r"'(?:\\.|[^'\\\n]){1,4}'")
_ENUM_CAST_REVISION_MARKER = "HOMM3_ENUM_CAST_REVISION_BOUNDARY"
_ENUM_CAST_REVISION_COMMENT = re.compile(
    r"/\*\s*" + _ENUM_CAST_REVISION_MARKER + r"\s*\*/")


def _strip(text: str) -> str:
    # Preserve the one auditable enum-cast admission token while stripping
    # every other comment.  The enum scanner below accepts it only when the
    # source line contains exactly one enum cast, so it cannot blanket-waive a
    # line containing several unrelated domain conversions.
    text = _ENUM_CAST_REVISION_COMMENT.sub(
        " " + _ENUM_CAST_REVISION_MARKER + " ", text)
    text = _BLOCK.sub(" ", text)
    text = _LINE.sub("", text)
    text = _STR.sub(" ", text)
    text = _CHR.sub(" ", text)
    return text


# --- C-style cast detectors -------------------------------------------------------
# The lookbehind (?<![\w>)]) excludes call/decl parens (`foo(int)`,
# `sizeof(char*)`) and the second link of a cast chain (same fidelity as
# gruntz). The lookahead requires an expression to FOLLOW, excluding
# parameter lists (`void f(int);`) and grouping parens.
_EXPR_START = r"(?=[\w(&*~!'\"-])"
_NUMERIC_CAST = re.compile(
    r"(?<![\w>)])\(\s*(?:const\s+|unsigned\s+|signed\s+)*"
    r"(?:int|char|short|long|float|double|void|bool|size_t|__int64"
    r"|BYTE|WORD|DWORD)"
    r"(?:\s+(?:int|char|long|double))*\s*\)\s*" + _EXPR_START)
_PTR_CAST = re.compile(
    r"(?<![\w>)])\(\s*(?:const\s+|unsigned\s+|signed\s+)*"
    r"[A-Za-z_]\w*(?:::\w+)*(?:\s*<[^()<>;]*>)?(?:\s+const)?"
    r"\s*\*+\s*(?:const\s*)?\)\s*" + _EXPR_START)
_THIS_CAST = re.compile(r"\)\s*this\b")


def _c_cast_sites(code: str, _ctx) -> list:
    out = {}
    for pattern in (_NUMERIC_CAST, _PTR_CAST, _THIS_CAST):
        for m in pattern.finditer(code):
            out[m.start()] = None
    return sorted(out)


def _regex_sites(pattern):
    return lambda code, _ctx: [m.start() for m in pattern.finditer(code)]


# --- the other ratcheted shapes (gruntz spellings) --------------------------------
_REINTERPRET = re.compile(r"\breinterpret_cast\s*<")
_VOLATILE = re.compile(r"\bvolatile\b")
_CPP_EXTERN = re.compile(r"^[ \t]*extern\b", re.MULTILINE)
# struct/class DEFINITION (name then body brace, optional base clause) -
# not forward decls, not elaborated uses (`class TBar* p;`).
_CPP_LOCAL_DEF = re.compile(
    r"\b(?:struct|class)\s+\w+\b(?:\s+final)?\s*(?::[^;{]*)?\{")
_CPP_LOCAL_ENUM = re.compile(
    r"^[ \t]*(?:typedef[ \t]+)?enum\b[ \t]*\w*\s*\{", re.MULTILINE)
_MAGIC_CASE = re.compile(
    r"^[ \t]*case[ \t]+(?:0[xX][0-9a-fA-F]+|-?[0-9]+)[ \t]*:", re.MULTILINE)
_UNNAMED_COMPARE = re.compile(
    r"[=!]=[ \t]*(?:0[xX](?!0\b|1\b)[0-9a-fA-F]+|(?!0\b|1\b)[0-9]+)\b")
_INLINE_GATE = re.compile(r"\bINLINE_GATE\s*\(")
_INLINE_DEPTH_ZERO = re.compile(
    r"^[ \t]*#[ \t]*pragma[ \t]+inline_depth[ \t]*\([ \t]*0[ \t]*\)",
    re.MULTILINE)

# One preprocessor directive may span several physical lines. Count every
# view identifier in the complete logical directive, while `_strip` keeps
# comments and quoted fixture text from becoming false source artifacts.
_PP_LOGICAL = re.compile(
    r"^[ \t]*\#[^\r\n]*(?:\\\r?\n[^\r\n]*)*", re.MULTILINE)
_VIEW_DIRECTIVE_IDENT = re.compile(r"\b[A-Z][A-Z0-9_]*_VIEW\b")


def _view_preprocessor_sites(code: str, _ctx) -> list:
    out = []
    for directive in _PP_LOGICAL.finditer(code):
        out.extend(directive.start() + match.start()
                   for match in _VIEW_DIRECTIVE_IDENT.finditer(
                       directive.group()))
    return out

# Per-TU scaffolds under any other suffix. Only the conditional and
# (un)definition directives can gate visibility; #include / #pragma /
# #else / #endif never carry the identifier that decides a fork.
_SCAFFOLD_DIRECTIVE = re.compile(
    r"^[ \t]*\#[ \t]*(?:if|ifdef|ifndef|elif|define|undef)\b")
_SCAFFOLD_IDENT = re.compile(r"\bHOMM3_[A-Z0-9_]+\b")
_FUNCLIKE_DEFINE = re.compile(
    r"^[ \t]*\#[ \t]*define[ \t]+([A-Za-z_]\w*)\(", re.MULTILINE)
_ANY_DEFINE = re.compile(
    r"^[ \t]*\#[ \t]*define[ \t]+([A-Za-z_]\w*)", re.MULTILINE)
VA_HEADER = REPO / "include/va.h"


def _legit_pp_names(sources) -> frozenset:
    """Tree-wide pre-pass: every function-like macro name plus every
    name include/va.h defines. Both are legitimate wherever they appear
    in a directive (a use of a function-like macro inside another
    #define is still that macro, not a scaffold)."""
    names = set()
    for path, code in sources:
        names.update(_FUNCLIKE_DEFINE.findall(code))
        if path == VA_HEADER:
            names.update(_ANY_DEFINE.findall(code))
    return frozenset(names)


def _scaffold_preprocessor_sites(code: str, ctx) -> list:
    legit = (ctx.get("pp_legit", frozenset())
             | frozenset(_FUNCLIKE_DEFINE.findall(code)))
    out = []
    for directive in _PP_LOGICAL.finditer(code):
        text = directive.group()
        if not _SCAFFOLD_DIRECTIVE.match(text):
            continue
        for match in _SCAFFOLD_IDENT.finditer(text):
            name = match.group()
            if name.endswith("_H") or name in legit:
                continue
            out.append(directive.start() + match.start())
    return out

# The enum-cast row needs the tree's declared enum NAMES (collected in a
# pre-pass over the stripped sources) - the registry travels in ctx.
_ENUM_DECL = re.compile(r"\benum\s+([A-Za-z_]\w*)")


def _enum_cast_pattern(names) -> re.Pattern | None:
    if not names:
        return None
    return re.compile(r"static_cast\s*<\s*(?:const\s+)?(?:"
                      + "|".join(map(re.escape, sorted(names)))
                      + r")\s*>")


def _enum_cast_sites(code: str, ctx) -> list:
    pattern = ctx.get("enum_cast_re")
    if pattern is None:
        return []
    out = []
    for match in pattern.finditer(code):
        line_start = code.rfind("\n", 0, match.start()) + 1
        line_end = code.find("\n", match.end())
        if line_end < 0:
            line_end = len(code)
        line = code[line_start:line_end]
        admitted = (_ENUM_CAST_REVISION_MARKER in line
                    and len(pattern.findall(line)) == 1)
        if not admitted:
            out.append(match.start())
    return out


# (label, sites(code, ctx)->[offset], cpp_only, how-to-fix note)
METRICS = (
    ("C-style casts", _c_cast_sites, False,
     "spell it as a named C++ cast (static_cast / reinterpret_cast)"),
    ("reinterpret_casts", _regex_sites(_REINTERPRET), False,
     "model the real type instead; reinterpret_cast debt only drains"),
    ("volatile qualifiers", _regex_sites(_VOLATILE), False,
     "`volatile` is not a codegen lever. Diagnose with `homm3 dreamcast "
     "show <selector>`, `homm3 vc6 predict-inline <selector>`, and `homm3 "
     "sema diff <selector> --structure` plus `--source`. Then: (1) restore "
     "the Dreamcast-proven helper, type, local lifetime, or statement order; "
     "(2) use statement-scoped `#pragma inline_depth(0)` only as an "
     "uncommitted diagnostic, then recover the natural compiler state; or "
     "(3) when a Dreamcast line gap and a real "
     "release-elided invariant agree, spell that invariant as "
     "`HOMM3_RELEASE_VERIFY(expression)`. Retained verifies need an evidence "
     "comment, retail checkpoint, and flattening negative "
     "control. Never replace volatile with self-assignment, dead code, or "
     "synthetic caller mass; accept a non-MAX current dip while recovering "
     "the true source"),
    ("inline-gate artifacts", _regex_sites(_INLINE_GATE), False,
     "remove INLINE_GATE: it is a source-false no-op wrapper"),
    ("inline-depth pins", _regex_sites(_INLINE_DEPTH_ZERO), False,
     "do not add committed compiler-steering pins; use them only as local "
     "diagnostics, then recover the natural declaration/body/TU state"),
    ("cpp extern decls", _regex_sites(_CPP_EXTERN), True,
     "declare it ONCE in the owner's header and #include that - a "
     "consumer .cpp never re-declares"),
    (".cpp-local views", _regex_sites(_CPP_LOCAL_DEF), True,
     "the type's one true shape belongs in include/ - move it"),
    (".cpp-local enums", _regex_sites(_CPP_LOCAL_ENUM), True,
     "a cross-TU domain stranded in a .cpp gets re-declared by the next "
     "TU that needs it - move the enum to include/"),
    ("casts to enum types", _enum_cast_sites, False,
     "a cast into an enum domain usually means two mis-modeled enums - "
     "unify or re-model them; raise the floor only via `board --update`"),
    ("magic case labels", _regex_sites(_MAGIC_CASE), False,
     "declare the domain enum and case on its enumerators"),
    ("unnamed domain compares", _regex_sites(_UNNAMED_COMPARE), False,
     "name the domain member: declare/extend the enum and compare on it"),
    ("view preprocessor artifacts", _view_preprocessor_sites, False,
     "make the recovered declaration/body canonical and remove the per-TU "
     "view guard plus its companion fork"),
    ("per-TU preprocessor scaffolds", _scaffold_preprocessor_sites, False,
     "make the recovered declaration/layout/body canonical for EVERY "
     "consumer, then delete the object-like `#define HOMM3_X` and its "
     "#if/#ifdef/#else fork; never add a new per-TU guard, and reconcile "
     "two arms to the one shape retail bytes + the DC dump prove"),
)

# All rows ratchet: floors only move down (raises are explicit --update).
RATCHET = {label for label, _, _, _ in METRICS}
_FIX = {label: fix for label, _, _, fix in METRICS}


def count(per_file: bool = False):
    sources = []
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix not in EXTS or not path.is_file():
                continue
            try:
                code = _strip(path.read_text(errors="ignore"))
            except OSError:
                continue
            sources.append((path, code))

    names = frozenset(name for _path, code in sources
                      for name in _ENUM_DECL.findall(code))
    ctx = {"enums": names, "enum_cast_re": _enum_cast_pattern(names),
           "pp_legit": _legit_pp_names(sources)}

    totals = {label: 0 for label, _, _, _ in METRICS}
    offenders = []
    for path, code in sources:
        for label, sites, cpp_only, _fix in METRICS:
            if cpp_only and path.suffix not in _CPP:
                continue
            found = sites(code, ctx)
            totals[label] += len(found)
            if found and per_file:
                for off in found:
                    line = code.count("\n", 0, off) + 1
                    snippet = code[off:off + 48].split("\n")[0]
                    offenders.append((label,
                                      f"{path.relative_to(REPO)}:{line}: "
                                      f"{snippet}"))
    rows = [(label, totals[label]) for label, _, _, _ in METRICS]
    return (rows, offenders) if per_file else rows


def load_baseline() -> dict[str, int]:
    if not BASELINE.is_file():
        return {}
    out = {}
    for line in BASELINE.read_text().splitlines():
        if line.startswith("#") or "\t" not in line:
            continue
        label, n = line.rsplit("\t", 1)
        try:
            out[label] = int(n)
        except ValueError:
            pass
    return out


def save_baseline(rows) -> None:
    head = ("# MANUALLY RATCHETED - homm3.cleanliness.board floors; build\n"
            "# rolls them DOWN-only, `board --update` is the only bless.\n")
    BASELINE.write_text(
        head + "".join(f"{label}\t{n}\n" for label, n in rows))


def merge_downonly(rows) -> list:
    """min(count, committed floor) for ratcheted rows; a label missing
    from `rows` keeps its floor (unmeasured is not zero)."""
    base = load_baseline()
    out = []
    for label, n in rows:
        floor = base.get(label)
        out.append((label, min(n, floor) if label in RATCHET
                    and floor is not None else n))
    measured = {label for label, _ in rows}
    out.extend((label, n) for label, n in base.items()
               if label not in measured)
    return out


# --- the embedded negative controls -----------------------------------------------
# {label: (must_detect, must_pass)} - each metric proves it can fail on
# every invocation before it judges the tree. The selftest ctx declares
# eHero/eTown so the enum-cast row is exercised with a fixed registry.

_SAMPLES = {
    "C-style casts": (
        ("x = (int)y;",
         "p = (TCreature *)q;",
         "s = (const char*)buf;",
         "return (unsigned char)v;",
         "t = (DWORD)ticks;",
         "(void)unused_result;",
         "((advManager*)this)->DoEvent();",
         "n = (size_t)len;"),
        ("void f(int);",
         "int DoTownKnob(unsigned char up);",
         "n = sizeof(int);",
         "if (x) y = 1;",
         "call(a, b);",
         "q = static_cast<int>(x);",
         "r = reinterpret_cast<TCreature *>(p);",
         "VA(0x00405de0, 0xD)",
         "void* strip::`scalar deleting destructor'(unsigned __flags)",
         "typedef void (*PFN)(int);",
         "z = (a) - b;",
         'printf("(int)x");',
         "// a comment saying (char*)cast")),
    "reinterpret_casts": (
        ("p = reinterpret_cast<TCreature *>(q);",
         "h = reinterpret_cast < HWND >(w);"),
        ("q = static_cast<int>(x);",
         "// reinterpret_cast<int> named in prose",
         "my_reinterpret_caster(x);")),
    "volatile qualifiers": (
        ("volatile int homed = value;",
         "int* volatile forced_register = pointer;",
         "const volatile unsigned char* device = address;"),
        ("int stable = value;",
         "int volatile_count = 0;",
         'trace("volatile int homed");',
         "// volatile was a rejected negative control")),
    "inline-gate artifacts": (
        ("INLINE_GATE(call());",
         "#define INLINE_GATE(statement) statement"),
        ("call();",
         "my_INLINE_GATE_counter();",
         'trace("INLINE_GATE(call())");',
         "// INLINE_GATE(call()) was removed")),
    "inline-depth pins": (
        ("#pragma inline_depth(0)\ncall();\n#pragma inline_depth()",
         "# pragma inline_depth ( 0 )"),
        ("#pragma inline_depth()",
         "#pragma inline_depth(1)",
         "// #pragma inline_depth(0) was a probe")),
    "cpp extern decls": (
        ("extern int g_heroCount;",
         '  extern "C" void mm_init();'),
        ("int externalize();",
         "// extern lives in the owner header",
         "internal_extern_helper();")),
    ".cpp-local views": (
        ("struct TFoo { int a; };",
         "class advPopup : public TWindow {"),
        ("struct TFoo;",
         "class TBar* p;",
         "enum eHero { KNIGHT };")),
    ".cpp-local enums": (
        ("enum eHero { KNIGHT };",
         "typedef enum { RES_A, RES_B } TRes;",
         "enum eTown\n{"),
        ("enum eHero;",
         "int enumerate(void);",
         "// enum eGone { X }; retired")),
    "casts to enum types": (
        ("h = static_cast<eHero>(t);",
         "t = static_cast< const eTown >(x);",
         "h = static_cast<eHero>(t); q = static_cast<eTown>(x); "
         "/* HOMM3_ENUM_CAST_REVISION_BOUNDARY */"),
        ("n = static_cast<int>(h);",
         "q = static_cast<eOther>(x);",
         "p = static_cast<eHero*>(v);",
         "h = static_cast<eHero>(t) "
         "/* HOMM3_ENUM_CAST_REVISION_BOUNDARY */;")),
    "magic case labels": (
        ("case 5:",
         "  case 0x1f:  DoThing(); break;",
         "case -1:"),
        ("case eHero::KNIGHT:",
         "case KNIGHT_ID:",
         "int case5 = 1;",
         "// case 3: retired")),
    "unnamed domain compares": (
        ("if (x == 0x36)",
         "while (kind != 7)",
         "ok = id == 12;"),
        ("if (x == 0)",
         "if (x == 1)",
         "while (p != 0)",
         "// compare == 55 in prose")),
}

# Build the samples in pieces so the repository's literal user-facing
# census remains honest while the runtime selftest still feeds complete
# directives to the parser.
_VIEW_WORD = "VI" + "EW"
_VIEW_DEFINE_SAMPLE = (
    "#define HOMM3_SAMPLE_"
    + _VIEW_WORD)
_VIEW_UNDEF_SAMPLE = (
    "#undef HOMM3_SAMPLE_"
    + _VIEW_WORD)
_VIEW_IFDEF_SAMPLE = (
    "#if"
    + "def HOMM3_SAMPLE_"
    + _VIEW_WORD)
_VIEW_IFNDEF_SAMPLE = (
    "#ifn"
    + "def HOMM3_SAMPLE_"
    + _VIEW_WORD)
_VIEW_IF_SAMPLE = (
    "#if defined(HOMM3_SAMPLE_"
    + _VIEW_WORD
    + ") || \\\n    defined(HOMM3_SECOND_"
    + _VIEW_WORD
    + ")")
_VIEW_ELIF_SAMPLE = (
    "#elif defined(HOMM3_SAMPLE_"
    + _VIEW_WORD
    + ")")
_VIEW_WORLD_CONSTANT = (
    "#define "
    + _VIEW_WORD
    + "_WORLD_TILE_SCALE_FULL 16.0f")
_VIEW_NAMED_INCLUDE_GUARD = (
    "#ifndef HOMM3_"
    + _VIEW_WORD
    + "ARMYWINDOW_H")
_VIEW_COMMENT_SAMPLE = (
    "// #define HOMM3_COMMENT_"
    + _VIEW_WORD)
_SAMPLES["view preprocessor artifacts"] = (
    (_VIEW_DEFINE_SAMPLE, _VIEW_UNDEF_SAMPLE, _VIEW_IFDEF_SAMPLE,
     _VIEW_IFNDEF_SAMPLE, _VIEW_IF_SAMPLE, _VIEW_ELIF_SAMPLE),
    (_VIEW_WORLD_CONSTANT, _VIEW_NAMED_INCLUDE_GUARD,
     "#if defined(HOMM3_SAMPLE_DECLS)",
     _VIEW_COMMENT_SAMPLE))

# The scaffold row's samples are built the same way: the prefix is split
# so a tree-wide grep for the real identifiers never lands on this file.
_SCAFFOLD_PREFIX = "HOMM3" + "_"
_SCAFFOLD_DECLS = _SCAFFOLD_PREFIX + "SAMPLE_DECLS"
_SCAFFOLD_INLINE = _SCAFFOLD_PREFIX + "SAMPLE_INLINE"
_SCAFFOLD_LAYOUT = _SCAFFOLD_PREFIX + "SECOND_LAYOUT"
_SCAFFOLD_ERROR = _SCAFFOLD_PREFIX + "SAMPLE_ERROR"
_SCAFFOLD_VERIFY = _SCAFFOLD_PREFIX + "SAMPLE_VERIFY"
_SCAFFOLD_RELEASE_VERIFY = _SCAFFOLD_PREFIX + "RELEASE_VERIFY"
_SAMPLES["per-TU preprocessor scaffolds"] = (
    ("#define " + _SCAFFOLD_DECLS,
     "#undef " + _SCAFFOLD_DECLS,
     "#if" + "def " + _SCAFFOLD_DECLS,
     "#ifn" + "def " + _SCAFFOLD_INLINE,
     "#if defined(" + _SCAFFOLD_DECLS + ") || \\\n    defined("
     + _SCAFFOLD_LAYOUT + ")",
     "#elif defined(" + _SCAFFOLD_DECLS + ")",
     "#if !defined(" + _SCAFFOLD_INLINE + ")",
     "  # define " + _SCAFFOLD_DECLS + " 1"),
    ("#ifn" + "def " + _SCAFFOLD_PREFIX + "SAMPLE_H\n#define "
     + _SCAFFOLD_PREFIX + "SAMPLE_H",
     "#define " + _SCAFFOLD_ERROR + "(code) (0x88770000UL + (code))\n"
     "#define DPERR_SAMPLE " + _SCAFFOLD_ERROR + "(5)",
     "#define " + _SCAFFOLD_VERIFY + "(expression) \\\n    "
     + _SCAFFOLD_RELEASE_VERIFY + "(expression)",
     _SCAFFOLD_RELEASE_VERIFY + "(x == 1);",
     "#pragma " + _SCAFFOLD_DECLS,
     "#include <va.h>",
     "#else\n#endif",
     "int " + _SCAFFOLD_DECLS + " = 1;",
     "// #define " + _SCAFFOLD_PREFIX + "COMMENT_DECLS"))


def selftest() -> list[str]:
    failures = []
    counters = {label: sites for label, sites, _, _ in METRICS}
    names = frozenset({"eHero", "eTown"})
    # Mirror count()'s pre-pass: va.h's verify macro is legitimate.
    ctx = {"enums": names, "enum_cast_re": _enum_cast_pattern(names),
           "pp_legit": frozenset({_SCAFFOLD_RELEASE_VERIFY})}
    for label, (positives, negatives) in _SAMPLES.items():
        sites = counters[label]
        for sample in positives:
            if len(sites(_strip(sample), ctx)) < 1:
                failures.append(f"{label}: MISSED positive {sample!r}")
        for sample in negatives:
            if len(sites(_strip(sample), ctx)) != 0:
                failures.append(f"{label}: FALSE POSITIVE {sample!r}")
    missing = set(counters) - set(_SAMPLES)
    failures.extend(f"{label}: NO SELFTEST SAMPLES" for label in sorted(missing))
    volatile_fix = _FIX["volatile qualifiers"]
    for required in ("dreamcast show", "predict-inline", "uncommitted",
                     "HOMM3_RELEASE_VERIFY", "synthetic caller mass"):
        if required not in volatile_fix:
            failures.append(
                f"volatile qualifiers: repair diagnostic lost {required!r}")
    return failures


# --- entry points -----------------------------------------------------------------

def check_and_roll(write: bool) -> list[str]:
    """The build-tail gate: selftest, count, compare ratcheted rows to
    their floors; on `write` roll the baseline down-only. Returns fatal
    violation lines (empty = pass); prints the scoreboard line."""
    broken = selftest()
    if broken:
        return [f"cleanliness SELFTEST BROKEN: {b}" for b in broken]
    rows, offenders = count(per_file=True)
    base = load_baseline()
    violations = []
    for label, n in rows:
        floor = base.get(label)
        if label in RATCHET and floor is not None and n > floor:
            violations.append(
                f"cleanliness ratchet violated: {label} rose {floor} -> {n}"
                f" ({_FIX[label]})")
            hits = [f"  {where}" for lbl, where in offenders if lbl == label]
            violations.extend(hits[:10])
    if violations:
        return violations
    if write:
        save_baseline(merge_downonly(rows))
    cells = ", ".join(f"{label} {n}" for label, n in rows)
    print(f"[build] cleanliness: {cells} (floors hold)")
    return []


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    broken = selftest()
    if broken:
        for b in broken:
            print(f"SELFTEST BROKEN: {b}", file=sys.stderr)
        return 2
    if "--selftest" in argv:
        total = sum(len(p) + len(n) for p, n in _SAMPLES.values())
        print(f"selftest OK ({len(_SAMPLES)} metrics, {total} samples)")
        return 0
    rows, offenders = count(per_file=True)
    if "--update" in argv:
        save_baseline(rows)
        print("cleanliness baseline blessed: "
              + ", ".join(f"{label} {n}" for label, n in rows))
        return 0
    base = load_baseline()
    rc = 0
    for label, n in rows:
        floor = base.get(label)
        delta = (f" ({n - floor:+d} vs floor)"
                 if floor is not None and n != floor else "")
        print(f"{label}\t{n}{delta}")
        if label in RATCHET and floor is not None and n > floor:
            rc = 1
            print(f"  fix: {_FIX[label]}")
    for label, where in offenders:
        print(f"  [{label}] {where}")
    return rc


if __name__ == "__main__":
    sys.exit(main())
