"""homm3.vc6.reg_model - `why-reg`: guided oracle search over register knobs.

Which known source knob moves a divergent REGISTER BINDING toward the
reference? v1 contains NO reverse-engineered allocator model (that is the
Phase 4 predictor): it takes the behavior catalog's explained-lever knob
classes (docs/vc6/behavior-catalog.md, B family plus C3/C4), applies each as
a concrete source mutation, and lets the REAL pinned compiler judge every
candidate. **The VC6 compile is the sole verdict on whether a mutation
helps** - site discovery only decides what to try, never what is true.

Site discovery is REGEX-GUIDED over the target function's body only (a
deliberately narrow-but-reliable v1; libclang rewriting was rejected for
fragility on VC6-era C++). Mutation classes implemented, with the catalog
IDs they exercise:

  unname-local     delete `T n = expr;`, re-spell uses as `(expr)`
                   (B13 cached-member-vs-reload; B14 un-name a flag;
                    B15 char-local -> in-place re-read)
  name-expr        introduce `T __wr = expr;` for a repeated member/index
                   chain (B13/B15) or a `x != 0` flag (B14 pseudo-order)
  reorder-decls    swap adjacent independent declarations / plain stores
                   (B6 first-declared-lowest; C3 statement order)
  hoist-zero       repeated `= 0;` stores through one named zero local (B8)
  chain/split      `a = b = e;` <-> two separate stores, both orders (C4)

Pipeline per candidate: compile via homm3.core.cc_wrap with the game
profile (config/units.toml `game_o2_ml_gr_windows` + /GR-, the profile
every b*/c*/d* probe pins) + /FAs, slice fn F from the COFF object with
homm3.sema._asm's llvm-objdump path, and score against the reference with
_align.distance (unpaired register-visible masked-instruction slots; 0 =
register-visible exact). The reference is either a second compiled TU
(--against-src, hermetic) or retail F via the sema machinery (--against
UNIT:FN - the delinked target object when one exists, else capstone over
the image; the image producer carries cross-producer syntax skew, see
_align's caveat).

rc: 0 = zero divergence reached (already, or by a mutation - the winning
edit is printed as a diff for manual application, never auto-applied);
1 = improved-but-not-exact or no mutation helped; 2 = error (die).
Scratch: build/vc6/whyreg/{base,ref,mut}/.

v2 - the model path (phase 4). `run_model` replaces the blind sweep with
the reverse-engineered allocator model (homm3.vc6._regmodel, RE'd from
the pinned C2.DLL; docs/vc6/regalloc.md): ONE preference order
EAX ECX EDX ESI EDI EBX EBP walked first-fit per pseudo in creation
order, callee-saveds starting at ESI for call-crossing values. For a
B1-diagnosed function it reads both sides' callee-saved DEFINITION
tables, derives each side's allocator processing order from the bindings,
names the transposed pair, and proposes the ONE creation-order edit the
model predicts flips the binding - then compiles just that candidate
(up to --tries, default 1) instead of the ~20-50 mutation sweep. When the
transposed pair is not source-nameable (parameter/`this` vs call result:
the alias lever is copy-propagated, measured 2026-08-10) it says so and
attributes the residual to the front-end handle-state class (C1) instead
of burning compiles. Invoke: `python3 -m homm3.vc6.reg_model <src> --fn F
(--against U:F | --against-src FILE) [--tries N] [--il-order]`, or any
future `why-reg --model` flag (run_why forwards when args.model is set).
"""
from __future__ import annotations

import difflib
import json
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

from homm3.sema import _asm
from homm3.vc6 import _align, _common, _regmodel, _source, _unit

SCRATCH = _common.REPO / "build/vc6/whyreg"

# Game C++ profile: config/units.toml `game_o2_ml_gr_windows` (minus the
# build-plumbing /nologo /c, added at invocation) + /GR- per the catalog's
# profile-under-test (0.6: no COL pointers / no __RTDynamicCast in retail).
GAME_FLAGS = ["/O2", "/Ob2", "/Oy-", "/Op", "/ML", "/Gr", "/GX", "/GR-",
              "/D_WINDOWS"]

MAX_MUTATIONS = 64

# --- compile + slice ---------------------------------------------------------------


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
    """The one public text symbol of *obj* matching --fn. Prefers an exact
    decorated/undecorated identity; falls back to unique substring."""
    names = _asm._public_text_symbols(obj)
    strong = [n for n in names
              if n == fn or n.startswith(f"?{fn}@@")
              or re.fullmatch(rf"[@_]?{re.escape(fn)}(@\d+)?", n)]
    if len(strong) == 1:
        return strong[0]
    subs = sorted(n for n in names if fn in n)
    if len(strong) > 1:
        _common.die(f"--fn {fn!r} is ambiguous in {obj.name}: "
                    + ", ".join(sorted(strong)))
    if len(subs) == 1:
        return subs[0]
    if not subs:
        _common.die(f"--fn {fn!r} matches no public text symbol of "
                    f"{obj.name}; symbols: "
                    + ", ".join(sorted(names)[:12]))
    _common.die(f"--fn {fn!r} is ambiguous in {obj.name}: "
                + ", ".join(subs[:12]))


def _fn_text(obj: Path, fn: str) -> tuple:
    """(disassembly text, resolved symbol) for fn inside a compiled obj."""
    sym = _resolve_symbol(obj, fn)
    return _asm.objdump(obj, sym, 0), sym


def _reference_side(args) -> tuple:
    """(text, label) of the reference F. --against-src compiles a second
    TU (hermetic); --against UNIT:FN goes through the sema machinery -
    delinked target object when present, else capstone over the image."""
    if args.against_src:
        ref_src = Path(args.against_src).resolve()
        if not ref_src.is_file():
            _common.die(f"--against-src missing: {ref_src}")
        obj, tail = _compile_tu(ref_src, SCRATCH / "ref",
                                _wine_dir(ref_src.parent))
        if obj is None:
            _common.die(f"reference TU failed to compile:\n{tail}")
        text, sym = _fn_text(obj, args.fn)
        return text, f"compiled {ref_src.name} ({sym})"

    spec = args.against
    if ":" in spec:
        unit_hint, fnspec = spec.split(":", 1)
    else:
        unit_hint, fnspec = None, spec
    # Prefer the delinked target obj directly, by unit hint: the symbol lives
    # in the obj even when it is not (yet) in the carve roster
    # (build/gen/symbol_names.csv). This is what lets a freshly-reconstructed
    # function be diagnosed before it is admitted to the universe.
    if unit_hint:
        tobj = _asm.TARGET / f"{unit_hint}.c.obj"
        if tobj.is_file():
            try:
                sym = _resolve_symbol(tobj, fnspec)
                return (_asm.objdump(tobj, sym, 0),
                        f"delinked {unit_hint}.c.obj ({sym})")
            except SystemExit:
                pass  # not in this obj - fall through to roster / image
    from homm3.sema.context import get_context
    ctx = get_context()
    try:
        name, unit, rva, size, ordinal = ctx.symbols.resolve_fn(fnspec)
    except SystemExit:
        _common.die(
            f"no retail reference for {fnspec!r}: not a matching symbol in "
            f"the delinked target obj ({unit_hint}.c.obj) nor the carve roster."
            " If the base and retail mangled names differ, the reconstruction's"
            " signature does not match retail - fix that before diagnosing.")
    if unit_hint and unit_hint != unit:
        print(f"[note: {name} belongs to unit {unit or '(none)'}, "
              f"not {unit_hint} - using the resolved unit]")
    target_obj = _asm.TARGET / f"{unit}.c.obj"
    if target_obj.is_file():
        return (_asm.objdump(target_obj, name, ordinal),
                f"delinked {unit}.c.obj ({name})")
    if not size:
        _common.die(f"{name} has no recorded size - cannot carve its "
                    "image span")
    return (_asm.image_text(ctx, rva, size, name),
            f"retail image @ 0x{rva:x} ({name}, {size} B; capstone "
            "producer - expect some cross-producer skew)")


# --- mutation library (regex-guided; the compiler is the verdict) ------------------

_DECL = re.compile(
    r"^(?P<ind>[ \t]*)"
    r"(?P<type>(?:const[ \t]+|unsigned[ \t]+|signed[ \t]+)*"
    r"(?:int|long|short|char|bool|float|double|unsigned)(?:[ \t]*\*+)?)[ \t]+"
    r"(?P<name>\w+)[ \t]*(?:=[ \t]*(?P<init>[^;]+?))?[ \t]*;[ \t]*(?://.*)?$")
_CHAIN_EXPR = re.compile(r"\b\w+(?:->\w+|\.\w+|\[\w+\])+")
_FLAG_EXPR = re.compile(
    r"\b\w+(?:->\w+|\.\w+|\[\w+\])*[ \t]*[!=]=[ \t]*0(?![\dxX])")
_ZERO_STORE = re.compile(r"(?<![=!<>+\-*/&|^])=[ \t]*0[ \t]*;")
_CHAIN_ASSIGN = re.compile(
    r"^(?P<ind>[ \t]*)(?P<a>[\w>\-.\[\]]+?)[ \t]*=[ \t]*"
    r"(?P<b>[\w>\-.\[\]]+?)[ \t]*=[ \t]*(?P<e>[^;=]+?)[ \t]*;[ \t]*$")
_SIMPLE_ASSIGN = re.compile(
    r"^(?P<ind>[ \t]*)(?P<lhs>[\w>\-.\[\]]+?)[ \t]*=[ \t]*"
    r"(?P<e>[^;=]+?)[ \t]*;[ \t]*$")
_SIMPLE_VALUE = re.compile(r"-?(?:\w|->|\.|\[|\])+")


# Body location is _source's job (shared with why-branch): it demangles the
# special names (??0 / ??1 / operators) v1 could not spell, walks the full
# definition grammar (member-initialiser list, cv / throw() / calling-
# convention suffixes) and searches a MASKED view of the TU so a
# `#if 0  // @carcass` stub can never be mistaken for the real body. These
# aliases keep this module's internal call sites unchanged.
_source_name = _source.source_names
_fn_body_span = _source.body_span


def _reassigned(tail: str, name: str) -> bool:
    return re.search(
        rf"(?:\+\+|--)[ \t]*{name}\b"
        rf"|\b{name}[ \t]*(?:\+\+|--|(?:[-+*/%&|^]|<<|>>)?=(?!=))",
        tail) is not None


def _addr_taken(body: str, name: str) -> bool:
    return re.search(rf"(?<![&\w])&[ \t]*{name}\b", body) is not None


def _expr_pattern(expr: str):
    """Whole-occurrence matcher: not a prefix/suffix of a longer chain."""
    return re.compile(r"(?<![\w.>\]])" + re.escape(expr)
                      + r"(?!\s*(?:\[|\(|\.|->)|\w)")


def _expr_written(body: str, pat) -> bool:
    for m in pat.finditer(body):
        if re.match(r"[ \t]*(?:(?:[-+*/%&|^]|<<|>>)?=(?!=)|\+\+|--)",
                    body[m.end():]):
            return True
        pre = body[max(0, m.start() - 2):m.start()]
        if pre.endswith(("++", "--")) or (pre.endswith("&")
                                          and not pre.endswith("&&")):
            return True
    return False


def _decl_line_at(body: str, pos: int) -> tuple:
    """(line_start, indent) of the line containing pos."""
    start = body.rfind("\n", 0, pos) + 1
    indent = re.match(r"[ \t]*", body[start:]).group(0)
    return start, indent


def _stmt_line_at(body: str, pos: int) -> tuple:
    """(line_start, indent) of the STATEMENT containing pos - scans back to
    the nearest ';', '{' or '}' so a declaration can be inserted before a
    multi-line expression without landing mid-parenthesis."""
    head = max(body.rfind(c, 0, pos) for c in ";{}")
    start = body.find("\n", head, pos)
    start = head + 1 if start < 0 else start + 1
    indent = re.match(r"[ \t]*", body[start:]).group(0) \
        or re.match(r"[ \t]*", body[body.rfind("\n", 0, pos) + 1:]).group(0)
    return start, indent


def _mut_unname(body: str):
    for m in _iter_decl(body):
        name, init = m.group("name"), m.group("init")
        if not init:
            continue
        tail = body[m.end():]
        if _reassigned(tail, name) or _addr_taken(body, name):
            continue
        uses = len(re.findall(rf"\b{name}\b", tail))
        decl_type = m.group("type")
        if uses == 0:
            yield (f"delete unused local '{name}'", "B13",
                   body[:m.start()] + body[m.end():])
            continue
        if uses > 1 and re.search(r"[()]|\+\+|--", init):
            continue  # duplicating a call / side effect changes semantics
        catalog = ("B15" if "char" in decl_type else
                   "B14" if re.search(r"[!=]=[ \t]*0(?![\dxX])", init) else
                   "B13")
        repl = "(" + init.strip() + ")"
        new_tail = re.sub(rf"\b{name}\b", lambda _m: repl, tail)
        yield (f"unname local '{name}' (re-spell uses as {repl})",
               catalog, body[:m.start()] + new_tail)


def _iter_decl(body: str):
    """_DECL over each line of the body, with match spans in body space."""
    pos = 0
    for line in body.split("\n"):
        m = _DECL.match(line)
        if m:
            yield _Span(m, pos)
        pos += len(line) + 1


class _Span:
    """A line-local re.Match re-based into body coordinates."""

    def __init__(self, m, base):
        self._m, self._base = m, base

    def group(self, *a):
        return self._m.group(*a)

    def start(self):
        return self._base + self._m.start()

    def end(self):
        return self._base + self._m.end()


def _mut_name_expr(body: str):
    seq = 0
    chains = Counter(m.group(0) for m in _CHAIN_EXPR.finditer(body))
    for expr, n in chains.most_common(4):
        if n < 2:
            continue
        pat = _expr_pattern(expr)
        if _expr_written(body, pat):
            continue
        first = pat.search(body)
        if first is None:
            continue
        line_start, indent = _decl_line_at(body, first.start())
        types = ["long"] + (["char"] if expr.endswith("]") else [])
        for ty in types:
            tmp = f"__wr{seq}"
            seq += 1
            catalog = "B15" if ty == "char" else "B13"
            substituted = pat.sub(tmp, body[line_start:])
            yield (f"name '{expr}' as a {ty} local", catalog,
                   body[:line_start] + f"{indent}{ty} {tmp} = {expr};\n"
                   + substituted)
    for m in list(_FLAG_EXPR.finditer(body))[:3]:
        expr = m.group(0)
        line_start, indent = _stmt_line_at(body, m.start())
        line_end = body.find("\n", line_start)
        line = body[line_start:line_end if line_end != -1 else len(body)]
        if _DECL.match(line):
            continue  # already a named flag - unname covers the reverse
        tmp = f"__wrf{seq}"
        seq += 1
        pat = re.compile(re.escape(expr))
        yield (f"name flag '{expr}' as a long local", "B14",
               body[:line_start] + f"{indent}long {tmp} = {expr};\n"
               + pat.sub(tmp, body[line_start:]))


def _crossref(expr: str, line: str) -> bool:
    """Does `line` reference `expr` (whole-occurrence)? The reorder/merge
    hazard test: a swap is unsafe when one statement names what the other
    stores. A merely SHARED base object (r->left vs r->top) is safe and is
    exactly the shape C3/C4 exist for, so no bag-of-identifiers test."""
    return _expr_pattern(expr).search(line) is not None


def _mut_reorder(body: str):
    lines = body.split("\n")
    for i in range(len(lines) - 1):
        m1, m2 = _DECL.match(lines[i]), _DECL.match(lines[i + 1])
        if m1 and m2 and lines[i] != lines[i + 1]:
            n1, n2 = m1.group("name"), m2.group("name")
            if _crossref(n1, lines[i + 1]) or _crossref(n2, lines[i]):
                continue  # one decl's init reads the other's name
            swapped = lines[:i] + [lines[i + 1], lines[i]] + lines[i + 2:]
            yield (f"swap adjacent decls '{n1}'/'{n2}'", "B6/C3",
                   "\n".join(swapped))
            continue
        m1, m2 = _SIMPLE_ASSIGN.match(lines[i]), _SIMPLE_ASSIGN.match(lines[i + 1])
        if not (m1 and m2) or lines[i] == lines[i + 1] \
                or _DECL.match(lines[i]) or _DECL.match(lines[i + 1]):
            continue
        l1, l2 = m1.group("lhs"), m2.group("lhs")
        if l1 == l2 \
                or not _SIMPLE_VALUE.fullmatch(m1.group("e")) \
                or not _SIMPLE_VALUE.fullmatch(m2.group("e")):
            continue  # calls/side effects in a swapped rhs change semantics
        if _crossref(l1, lines[i + 1]) or _crossref(l2, lines[i]):
            continue  # one store's target is read by the other statement
        swapped = lines[:i] + [lines[i + 1], lines[i]] + lines[i + 2:]
        yield (f"swap adjacent stores '{l1}'/'{l2}'", "C3",
               "\n".join(swapped))


def _mut_reorder_store_runs(body: str):
    """Move one store across an independent three-to-six statement run.

    Adjacent swaps are insufficient for C3 handle-state plateaus: the
    scheduler may recover its original order after one swap while a value
    created at the opposite end of the source run changes the later ready
    queue.  Keep this bounded to small, plain, pairwise-independent store
    runs; the pinned compiler remains the verdict for every candidate.
    """
    lines = body.split("\n")
    i = 0
    while i < len(lines):
        run = []
        j = i
        while j < len(lines) and len(run) < 6:
            m = _SIMPLE_ASSIGN.match(lines[j])
            if not m or _DECL.match(lines[j]) \
                    or not _SIMPLE_VALUE.fullmatch(m.group("e")):
                break
            run.append((lines[j], m.group("lhs")))
            j += 1
        if len(run) >= 3 and all(
                not _crossref(run[a][1], run[b][0])
                for a in range(len(run)) for b in range(len(run)) if a != b):
            block = [line for line, _lhs in run]
            for src in range(len(block)):
                for dst in range(len(block)):
                    if src == dst:
                        continue
                    moved = block[:]
                    line = moved.pop(src)
                    moved.insert(dst, line)
                    yield (f"move store '{run[src][1]}' from {src} to {dst} "
                           f"in {len(block)}-store run", "C3",
                           "\n".join(lines[:i] + moved + lines[j:]))
            reversed_block = list(reversed(block))
            yield (f"reverse independent {len(block)}-store run", "C3",
                   "\n".join(lines[:i] + reversed_block + lines[j:]))
        i = max(j, i + 1)


def _mut_reorder_wide(body: str):
    """Slot-coloring (B5/B6): move a declaration to the front/back of its
    contiguous declaration block - non-adjacent permutations the adjacent-swap
    reorder cannot reach. VC6 colours plain-int frame slots first-declared-
    lowest, so the block order steers the stack layout."""
    lines = body.split("\n")
    i = 0
    while i < len(lines):
        if not _DECL.match(lines[i]):
            i += 1
            continue
        j = i
        while j < len(lines) and _DECL.match(lines[j]):
            j += 1
        block = lines[i:j]
        if len(block) >= 3:
            names = [_DECL.match(b).group("name") for b in block]
            if not any(_crossref(names[k], block[-1])
                       or _crossref(names[-1], block[k])
                       for k in range(len(block) - 1)):
                yield (f"move decl '{names[-1]}' to front of block", "B5/B6",
                       "\n".join(lines[:i] + [block[-1]] + block[:-1]
                                 + lines[j:]))
            if not any(_crossref(names[0], block[k])
                       or _crossref(names[k], block[0])
                       for k in range(1, len(block))):
                yield (f"move decl '{names[0]}' to back of block", "B5/B6",
                       "\n".join(lines[:i] + block[1:] + [block[0]]
                                 + lines[j:]))
        i = j


def _mut_hoist_zero(body: str):
    hits = list(_ZERO_STORE.finditer(body))
    if len(hits) < 2:
        return
    line_start, indent = _decl_line_at(body, hits[0].start())
    yield ("hoist repeated `= 0` stores through a named zero local", "B8",
           body[:line_start] + f"{indent}int __wrz = 0;\n"
           + _ZERO_STORE.sub("= __wrz;", body[line_start:]))


def _mut_chain(body: str):
    lines = body.split("\n")
    for i, line in enumerate(lines):
        m = _CHAIN_ASSIGN.match(line)
        if m and not _DECL.match(line):
            ind, a, b, e = m.group("ind", "a", "b", "e")
            if not _SIMPLE_VALUE.fullmatch(e):
                continue
            for first, second, tag in ((a, b, f"'{a}' first"),
                                       (b, a, f"'{b}' first")):
                new = lines[:i] + [f"{ind}{first} = {e}; {second} = {e};"] \
                    + lines[i + 1:]
                yield (f"split chain '{a} = {b} = {e}' into two stores, "
                       f"{tag}", "C4", "\n".join(new))
    for i in range(len(lines) - 1):
        m1 = _SIMPLE_ASSIGN.match(lines[i])
        m2 = _SIMPLE_ASSIGN.match(lines[i + 1])
        if not (m1 and m2) or _DECL.match(lines[i]) or _DECL.match(lines[i + 1]):
            continue
        if m1.group("e") != m2.group("e") \
                or not _SIMPLE_VALUE.fullmatch(m1.group("e")):
            continue
        lhs1, lhs2, e = m1.group("lhs"), m2.group("lhs"), m1.group("e")
        if lhs1 == lhs2 or _crossref(lhs1, lines[i + 1]) \
                or _crossref(lhs2, lines[i]):
            continue
        ind = m1.group("ind")
        for outer, inner in ((lhs1, lhs2), (lhs2, lhs1)):
            new = lines[:i] + [f"{ind}{outer} = {inner} = {e};"] \
                + lines[i + 2:]
            yield (f"chain '{lhs1}'/'{lhs2}' as '{outer} = {inner} = {e}'",
                   "C4", "\n".join(new))


def _mutations(src_text: str, fn: str) -> list:
    """Candidate mutants: [{'label','catalog','text'}], deduped, capped."""
    span = _fn_body_span(src_text, fn)
    if span is None:
        return []  # body not locatable -> diagnosis-only (run_why notes it)
    open_b, close_b = span
    body = src_text[open_b + 1:close_b]
    out, seen = [], {body}
    for gen in (_mut_unname, _mut_name_expr, _mut_reorder,
                _mut_reorder_store_runs, _mut_reorder_wide,
                _mut_hoist_zero, _mut_chain):
        for label, catalog, new_body in gen(body):
            if new_body in seen:
                continue
            seen.add(new_body)
            out.append({"label": label, "catalog": catalog,
                        "text": src_text[:open_b + 1] + new_body
                        + src_text[close_b:]})
            if len(out) >= MAX_MUTATIONS:
                return out
    return out


# --- the search + report -----------------------------------------------------------


def _slug(label: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", label.lower()).strip("_")[:32]


def _resolve_unit(args) -> str | None:
    """The matching unit for --against UNIT:FN, when its build flags are
    known - the trigger for in-unit fidelity. --against-src stays hermetic
    (game profile), and an unknown unit falls back to the game profile too."""
    if not getattr(args, "against", None):
        return None
    spec = args.against
    fnspec = spec.split(":", 1)[1] if ":" in spec else spec
    try:
        from homm3.sema.context import get_context
        _n, unit, *_ = get_context().symbols.resolve_fn(fnspec)
    except (Exception, SystemExit):
        # resolve_fn dies (SystemExit) on a name outside the carve roster;
        # _reference_side still reaches the delinked obj by unit hint, so a
        # roster miss must not kill the run here (measured 2026-08-10).
        return None
    return unit if unit and _unit.flags_for_unit(unit) else None


def _prepare(args):
    """Shared head of run_why / run_model: compile (or reuse) the base,
    resolve the reference, parse both streams.  Returns a dict."""
    src = Path(args.src).resolve()
    if not src.is_file():
        _common.die(f"source missing: {src}")
    for sub in ("base", "ref", "mut"):
        shutil.rmtree(SCRATCH / sub, ignore_errors=True)

    unit = _resolve_unit(args)
    if unit:  # in-unit fidelity: base == the obj the ratchet scored,
        # but only when *src* IS the unit's manifest source - a modified
        # copy must be compiled, not silently swapped for the tree's obj.
        manifest_src = _unit.source_for_unit(unit)
        base_obj = _unit.base_obj(unit) \
            if manifest_src and manifest_src.resolve() == src else None
        if base_obj is None:
            base_obj, tail = _unit.compile_text(
                src.read_text(), unit, SCRATCH / "base", "base")
            if base_obj is None:
                _common.die(f"base TU failed to compile:\n{tail}")
    else:  # hermetic (--against-src) or unknown unit: game profile
        base_obj, tail = _compile_tu(src, SCRATCH / "base",
                                     _wine_dir(src.parent))
        if base_obj is None:
            _common.die(f"base TU failed to compile:\n{tail}")
    base_text, base_sym = _fn_text(base_obj, args.fn)
    ref_text, ref_label = _reference_side(args)

    base_recs = _align.parse_side(base_text)
    ref_recs = _align.parse_side(ref_text)
    if not base_recs:
        _common.die(f"no instructions parsed for {base_sym} in the base obj")
    if not ref_recs:
        _common.die("no instructions parsed on the reference side")
    return {"src": src, "unit": unit, "base_obj": base_obj,
            "base_sym": base_sym, "ref_label": ref_label,
            "base_recs": base_recs, "ref_recs": ref_recs,
            "base_text": base_text, "ref_text": ref_text}


def run_why(args) -> int:
    if getattr(args, "model", False) or getattr(args, "predict", False):
        return run_model(args)
    pre = _prepare(args)
    src, unit = pre["src"], pre["unit"]
    base_obj, base_sym = pre["base_obj"], pre["base_sym"]
    base_text, ref_text = pre["base_text"], pre["ref_text"]
    ref_label = pre["ref_label"]
    base_recs, ref_recs = pre["base_recs"], pre["ref_recs"]
    base_dist = _align.distance(base_recs, ref_recs)
    findings, stats = _align.diagnose(base_recs, ref_recs,
                                      base_text, ref_text)

    results = []
    src_text = src.read_text()
    body_found = _fn_body_span(src_text, args.fn) is not None
    if base_dist > 0 and body_found:
        mut_dir = SCRATCH / "mut"
        include_dir = _wine_dir(src.parent)
        for i, mut in enumerate(_mutations(src_text, args.fn)):
            stem = f"m{i:02d}_{_slug(mut['label'])}"
            mut_dir.mkdir(parents=True, exist_ok=True)
            if unit:
                obj, tail = _unit.compile_text(mut["text"], unit, mut_dir, stem)
                mut_src = mut_dir / f"{stem}.cpp"
            else:
                mut_src = mut_dir / f"{stem}.cpp"
                mut_src.write_text(mut["text"])
                obj, tail = _compile_tu(mut_src, mut_dir, include_dir)
            row = {"label": mut["label"], "catalog": mut["catalog"],
                   "file": str(mut_src.relative_to(_common.REPO)),
                   "distance": None, "delta": None, "status": ""}
            if obj is None:
                row["status"] = "compile-error (discarded)"
            else:
                try:
                    text, _sym = _fn_text(obj, args.fn)
                except SystemExit:
                    row["status"] = "fn not found post-mutation (discarded)"
                    results.append(row)
                    continue
                dist = _align.distance(_align.parse_side(text), ref_recs)
                row["distance"] = dist
                row["delta"] = dist - base_dist
                row["status"] = ("EXACT" if dist == 0 else
                                 "improved" if dist < base_dist else
                                 "no change" if dist == base_dist else
                                 "worse")
            results.append(row)

    scored = sorted((r for r in results if r["distance"] is not None),
                    key=lambda r: (r["distance"], r["label"]))
    winner = scored[0] if scored and scored[0]["distance"] < base_dist \
        else None
    exact = base_dist == 0 or (winner is not None
                               and winner["distance"] == 0)
    rc = 0 if exact else 1

    if getattr(args, "json", False):
        print(json.dumps({
            "src": str(src), "fn": args.fn, "symbol": base_sym,
            "reference": ref_label, "base_distance": base_dist,
            "stats": stats, "diagnosis": findings,
            "mutations": results,
            "winner": winner and winner["label"], "rc": rc}, indent=2))
        return rc

    print(f"[why-reg] {args.fn} ({base_sym}) in {src.name}")
    print(f"[base]      {base_obj.relative_to(_common.REPO)}  "
          f"{stats['base_n']} instruction(s)")
    print(f"[reference] {ref_label}  {stats['target_n']} instruction(s)")
    print(f"[distance]  {base_dist} unpaired masked-instruction slot(s) "
          "(0 = register-visible exact)")
    print("[diagnosis]")
    if findings:
        for f in findings:
            print(f"  {f['catalog']:<7} {f['kind']:<12} {f['detail']}")
    else:
        print("  (no recognized register-binding pattern - the streams "
              "differ but match no cataloged signature)"
              if base_dist else "  (none - already register-visible exact)")
    if base_dist == 0:
        print("[why-reg] base already matches the reference at the "
              "register-visible grade; nothing to search.")
        return 0

    if not body_found:
        print(f"[guided search] SKIPPED - {_source.explain_miss(src_text, args.fn)}"
              f"\n                (searched {src.name} for "
              f"{' / '.join(_source_name(args.fn)) or '<no source name>'}); "
              "diagnosis-only - apply the indicated knob by hand.")
        return 1
    print(f"[guided search] {len(results)} candidate mutation(s) from the "
          "catalog knob classes"
          + ("" if results else " - no applicable sites found"))
    if results:
        print(f"  {'mutation':<52} {'catalog-id':<10} {'Ddistance':>9} "
              f"{'new-divergences':>15}")
        ranked = scored + [r for r in results if r["distance"] is None]
        for r in ranked:
            if r["distance"] is None:
                print(f"  {r['label'][:52]:<52} {r['catalog']:<10} "
                      f"{'-':>9} {'-':>15}  {r['status']}")
            else:
                print(f"  {r['label'][:52]:<52} {r['catalog']:<10} "
                      f"{r['delta']:>+9} {r['distance']:>15}  "
                      f"{r['status']}")
    if winner is None:
        print("[why-reg] no mutation moved the divergence toward the "
              "reference - the knob is outside this library (or the "
              "divergence is not source-addressable; see the catalog's "
              "open B classes).")
        return 1
    print(f"[recommended edit] {winner['label']} ({winner['catalog']}) -> "
          f"{winner['distance']} slot(s)"
          + (" - register-visible EXACT" if winner["distance"] == 0 else
             " - best improvement, not exact"))
    mut_text = (_common.REPO / winner["file"]).read_text()
    diff = difflib.unified_diff(
        src_text.splitlines(), mut_text.splitlines(),
        fromfile=str(src.name), tofile=f"{src.name} (mutated)", lineterm="")
    for ln in diff:
        print("  " + ln)
    print("[why-reg proposes this edit; it never applies source changes]")
    return rc


# --- v2: the model-backed predictor (phase 4) ---------------------------------------
#
# Facts consumed here (RE'd + measured; docs/vc6/regalloc.md):
#   preference walk  EAX ECX EDX ESI EDI EBX EBP, first-fit
#                    (C2.DLL .databe 0xadff4, walked at regasg.c 0x8be6c/0x8c1dc)
#   callee slice     call-crossing pseudos start at ESI -> EDI -> EBX;
#                    the 4th is frame-homed (measured, pinned SP3 CL)
#   order            pseudo creation order; front-end symbol handles
#                    (IL `sy` stream) for named locals
#   B1 signature     same schedule + same def slots, the ESI/EDI(/EBX)
#                    bindings permuted = the two front-end states feed the
#                    allocator the same pseudos in a different order

_CS_DEF = re.compile(
    r"^(?:mov|lea|xor|movzx|movsx|or|and) (esi|edi|ebx)\b")
_MASM_CS_DEF = re.compile(
    r"^\t(?:mov|lea|xor|movzx|movsx|or|and)\s+(esi|edi|ebx),\s*(.+)$")
_MASM_SRCLINE = re.compile(r"^; (\d+)\s+:(.*)$")
_MASM_NAME = re.compile(r"_(\w+)\$\[")


def _first_defs(recs: list) -> list[dict]:
    """First DEFINITION of each callee-saved family, in stream order:
    [{'reg', 'slot', 'vis'}].  push/pop (save-restore) never counts."""
    out, seen = [], set()
    for i, r in enumerate(recs):
        m = _CS_DEF.match(r["vis"])
        if m and m.group(1) not in seen:
            seen.add(m.group(1))
            out.append({"reg": m.group(1), "slot": i, "vis": r["vis"]})
    return out


def _listing_defs(asm_path: Path, fn: str,
                  params: list[str] | None = None) -> list[dict]:
    """The same first-defs read from the /FAs MASM listing, with source
    attribution: [{'reg','masm','line','text','value'}].  `value` is the
    best-effort source name of the defined value: a `_name$[` frame operand,
    `this` for `mov r, ecx` in a member body, else the assignment target
    parsed from the preceding source-line comment."""
    if not asm_path.is_file():
        return []
    lines = asm_path.read_text(errors="replace").splitlines()
    # the PROC block for fn (match on the mangled symbol when present)
    lo = hi = None
    names = _source_name(fn)  # [] for a compiler-generated entity
    for i, ln in enumerate(lines):
        if "PROC" in ln and (fn in ln or (names and names[-1] in ln)):
            lo = i
        elif lo is not None and "ENDP" in ln:
            hi = i
            break
    if lo is None:
        return []
    out, seen = [], set()
    cur_line, cur_text = None, ""
    for ln in lines[lo:hi or len(lines)]:
        ms = _MASM_SRCLINE.match(ln)
        if ms:
            cur_line, cur_text = int(ms.group(1)), ms.group(2).strip()
            continue
        m = _MASM_CS_DEF.match(ln)
        if not m or m.group(1) in seen:
            continue
        seen.add(m.group(1))
        operand = m.group(2)
        value = None
        mn = _MASM_NAME.search(operand)
        entry_reg = operand.strip()
        if mn:
            value = mn.group(1)
        elif entry_reg == "ecx" and "@@QAE" in fn:
            value = "this"
        elif entry_reg in ("ecx", "edx") and params:
            # free __fastcall (/Gr): ecx/edx carry the first two parameters
            k = 0 if entry_reg == "ecx" else 1
            value = params[k] if k < len(params) else None
        else:
            md = _DECL.match("    " + cur_text)
            if md:
                value = md.group("name")
            else:
                ma = re.match(r"\s*([A-Za-z_]\w*)\s*=", cur_text)
                if ma:
                    value = ma.group(1)
        out.append({"reg": m.group(1), "masm": ln.strip(),
                    "line": cur_line, "text": cur_text, "value": value})
    return out


# Parameter names come from _source's RECORDED parameter span. Scanning
# backwards from the body (v1) reads the last `(...)` before the brace,
# which on a constructor is the last member-INITIALISER, not the parameter
# list - `: type_bottom_view_window(parent)` is indistinguishable from a
# parameter list in reverse.
_fn_params = _source.params


def _mut_alias_param(src_text: str, fn: str, param: str):
    """`<type> __wrp = param;` at body top + substitute uses: hoists the
    parameter's pseudo creation ahead of every local (B14's mechanism
    applied to a parameter).  Known limit, measured 2026-08-10 on ai.cpp
    get_attack_change: VC6 copy-propagates the alias, so this lever often
    does NOT move the binding - the compile is the verdict."""
    span = _fn_body_span(src_text, fn)
    if span is None:
        return
    head = src_text[max(0, span[0] - 600):span[0]]
    m = re.search(rf"([\w:<>]+(?:\s*[*&]+\s*|\s+))(?:const\s+)?"
                  rf"{re.escape(param)}\s*[,)]", head)
    ty = (m.group(1).strip() if m else "long")
    if m and "*" in m.group(0):
        ty = "const " + ty + ("" if ty.endswith("*") else "*")
    body = src_text[span[0] + 1:span[1]]
    new_body = ("\n    " + ty + " __wrp = " + param + ";"
                + re.sub(rf"\b{re.escape(param)}\b", "__wrp", body))
    yield (f"alias parameter '{param}' into a first-created local",
           "B14/B1", new_body)


def _model_analysis(base_recs, ref_recs, findings) -> dict:
    """Interpret a binding finding through the allocator model."""
    out = {"applies": False}
    binding = next((f for f in findings if f["kind"] == "binding"), None)
    if binding is None:
        return out
    base_defs = _first_defs(base_recs)
    ref_defs = _first_defs(ref_recs)
    klass = "scratch" if binding["catalog"].startswith("B10") else "callee"
    out.update({
        "applies": True,
        "catalog": binding["catalog"],
        "klass": klass,
        "base_defs": base_defs,
        "ref_defs": ref_defs,
    })
    if klass == "scratch":
        out["swapped"] = []
        out["verdict"] = (
            "caller-saved (EAX/ECX/EDX) preference divergence - the walk "
            "tries EAX first, so the value the reference materializes in "
            "EAX was its FIRST-created scratch pseudo; the lever is B14: "
            "bind that value to a name so its pseudo is created before its "
            "competitors'")
        return out
    # pair k-th def with k-th def (B1 premise: same schedule, same def slots)
    pairs = list(zip(base_defs, ref_defs))
    swapped = [(b, r) for b, r in pairs if b["reg"] != r["reg"]]
    out["pairs"] = pairs
    out["swapped"] = swapped
    if not swapped:
        out["verdict"] = ("bindings agree at every first definition - the "
                          "divergence is past the first defs (not the B1 "
                          "minimum slice)")
        return out
    # allocator processing order per side = defs sorted by preference rank
    rank = {r: i for i, r in enumerate(_regmodel.PREFERENCE)}
    base_order = sorted(range(len(base_defs)),
                        key=lambda k: rank[base_defs[k]["reg"]])
    ref_order = sorted(range(len(ref_defs)),
                       key=lambda k: rank[ref_defs[k]["reg"]])
    out["base_processing"] = base_order   # def-ordinals, first-processed first
    out["ref_processing"] = ref_order
    first_ref = ref_order[0] if ref_order else None
    out["want_first"] = first_ref
    out["verdict"] = (
        "definition slots and order agree on both sides; the "
        f"{'/'.join(sorted(set(b['reg'] for b, _ in swapped)))} bindings are "
        "permuted, so the two compiles fed the allocator the SAME pseudos in "
        "a DIFFERENT processing order (front-end state) - the model's lever "
        "is pseudo creation order: the value the reference binds to "
        f"{ref_defs[first_ref]['reg'] if first_ref is not None else '?'} "
        "must become the earliest-created call-crossing pseudo")
    return out


def _rank_for_model(muts: list[dict], klass: str,
                    pair_values: list[str]) -> list[dict]:
    """Model-fit ranking of the mutation library: keep only creation-order
    movers, targeted at the swapped pair's source names when known.
    *klass* is 'callee' (ESI/EDI/EBX permuted) or 'scratch' (EAX/ECX/EDX):
    the flag-naming lever (B14) is the scratch-class prescription, the
    alias/declaration-order levers are the callee-class ones."""
    def score(m):
        label, cat = m["label"], m["catalog"]
        named = any(v and re.search(rf"\b{re.escape(v)}\b", label)
                    for v in pair_values)
        if klass == "scratch":
            if cat.startswith("B14") and "flag" in label:
                return 0 if named or not pair_values else 1
            if cat.startswith("B13") and label.startswith("name "):
                return 2 if named else 4
            if cat.startswith("B6") or cat.startswith("B5") or cat == "C3":
                return 3 if named else 5
            return 9
        if "alias parameter" in label:
            return 0 if named else 2
        if cat.startswith("B6") or cat.startswith("B5") or cat == "C3":
            return 1 if named else 4
        if cat.startswith("B13") and label.startswith("name "):
            return 3 if named else 5
        if cat.startswith("B14") and "flag" in label:
            return 6
        return 9
    return sorted((m for m in muts if score(m) < 9), key=score)


def _il_order_note(src: Path, fn: str) -> list[str]:
    """Evidence channel: the front end's symbol-handle creation order around
    fn, from a fresh IL capture (docs/vc6/il-format.md).  Best-effort - rich
    TUs have heuristic framing; this ANNOTATES, the compile decides."""
    try:
        from homm3.vc6 import il as _iltap
        from homm3.vc6 import _il as _ilf
    except ImportError as exc:  # pragma: no cover
        return [f"(IL tap unavailable: {exc})"]
    try:
        cap = _iltap.capture(src, list(_iltap.DEFAULT_FLAGS),
                             IL_SCRATCH := (SCRATCH / "il"))
    except SystemExit:
        return ["(IL capture failed - front end error on this TU)"]
    gl = cap["gl"].read_bytes()
    sy = cap["sy"].read_bytes()
    hw = _ilf.gl_highwater(gl)
    fn_recs = [r for r in _ilf.scan_names(gl, hw, min_len=3)
               if r["name"].startswith("?")]
    mine = [r for r in fn_recs if fn in r["name"]]
    if not mine:
        return ["(function not found in the gl symbol scan)"]
    h = mine[0]["handle"]
    later = [r["handle"] for r in fn_recs if r["handle"] > h]
    nxt = min(later) if later else (hw or h + 0x80)
    prev_c = [r["handle"] for r in fn_recs if r["handle"] < h]
    prev = max(prev_c) if prev_c else 0
    locals_ = [r for r in _ilf.scan_names(sy, hw, min_len=1)
               if prev < r["handle"] < nxt]
    locals_.sort(key=lambda r: r["handle"])
    row = ", ".join(f"{r['name']}@{r['handle']:#x}" for r in locals_[:16])
    return [f"fn handle {h:#x}; local symbols in CREATION (handle) order: "
            f"{row or '(none scanned)'}"]


def _handle_advice(args, value):
    """The handle-order model's verdict on whether `value`'s pseudo can be
    renumbered by a source change (movable lever) or is C1-capped. Behind
    try/except: a missing/failing model must NEVER break why-reg."""
    try:
        from homm3.vc6 import _handles
        against = getattr(args, "against", "") or ""
        unit = against.split(":", 1)[0] if ":" in against else None
        if not unit:
            return None
        return _handles.advise_handle_move(unit, args.fn, value)
    except Exception:
        return None


def run_model(args) -> int:
    """why-reg v2: model-backed predictor.  One (or --tries) targeted
    compile(s) instead of the blind sweep."""
    tries = max(1, min(int(getattr(args, "tries", 1) or 1), 3))
    pre = _prepare(args)
    src, unit = pre["src"], pre["unit"]
    base_obj, base_sym = pre["base_obj"], pre["base_sym"]
    base_recs, ref_recs = pre["base_recs"], pre["ref_recs"]
    base_dist = _align.distance(base_recs, ref_recs)
    findings, stats = _align.diagnose(base_recs, ref_recs,
                                      pre["base_text"], pre["ref_text"])

    report = {"src": str(src), "fn": args.fn, "symbol": base_sym,
              "reference": pre["ref_label"], "base_distance": base_dist,
              "stats": stats, "diagnosis": findings, "mode": "model"}

    print(f"[why-reg v2] {args.fn} ({base_sym}) in {src.name}  [model path]")
    print(f"[reference]  {pre['ref_label']}")
    print(f"[distance]   {base_dist} unpaired register-visible slot(s)")
    if base_dist == 0:
        print("[why-reg v2] already register-visible exact.")
        if getattr(args, "json", False):
            print(json.dumps(report, indent=2))
        return 0

    print("[model] preference walk EAX ECX EDX ESI EDI EBX EBP, first-fit "
          "per pseudo in creation order (C2 .databe:0xadff4, regasg.c "
          "0x8be6c/0x8c1dc; docs/vc6/regalloc.md)")
    analysis = _model_analysis(base_recs, ref_recs, findings)
    report["analysis"] = {k: v for k, v in analysis.items()
                          if k not in ("base_defs", "ref_defs", "pairs")}
    if not analysis["applies"]:
        print("[model] no register-binding divergence diagnosed - the "
              "divergence is not in this model's slice (see the A/D "
              "families); falling back is v1's job (`why-reg` without "
              "--model).")
        if getattr(args, "json", False):
            print(json.dumps(report, indent=2))
        return 1

    # the definition tables
    for side, defs in (("base", analysis["base_defs"]),
                       ("ref ", analysis["ref_defs"])):
        row = "  ".join(f"#{k}:{d['reg']}@{d['slot']}"
                        for k, d in enumerate(defs))
        print(f"[defs] {side}  {row}")

    # source attribution from the base /FAs listing; the ratchet's base obj
    # carries none, so compile a listing copy (byte-faithful per _unit's
    # proof) - the one extra compile buys the value names the model needs.
    listing = base_obj.with_suffix(".asm")
    if not listing.is_file() and unit:
        lobj, _tail = _unit.compile_text(src.read_text(), unit,
                                         SCRATCH / "model", "base_listing")
        if lobj is not None:
            listing = lobj.with_suffix(".asm")
    src_text0 = src.read_text()
    ldefs = _listing_defs(listing, base_sym,
                          _fn_params(src_text0, args.fn)) \
        if listing.is_file() else []
    values = {d["reg"]: d.get("value") for d in ldefs}
    if ldefs:
        for d in ldefs:
            print(f"[value] {d['reg']} <- {d['value'] or '?'}  "
                  f"(line {d['line']}: {d['text'][:60]})")
    print(f"[model] {analysis['verdict']}")

    if getattr(args, "il_order", False):
        for note in _il_order_note(src, args.fn):
            print(f"[il]    {note}")

    klass = analysis.get("klass", "callee")
    swapped = analysis.get("swapped") or []
    if klass == "callee" and not swapped:
        if getattr(args, "json", False):
            print(json.dumps(report, indent=2))
        return 1
    want = analysis.get("want_first")
    want_reg = analysis["ref_defs"][want]["reg"] \
        if klass == "callee" and want is not None else None
    want_value = None
    if klass == "callee" and want is not None and want < len(ldefs):
        # base's k-th def is the same value as ref's k-th def (B1 premise)
        want_value = ldefs[want].get("value")
    pair_values = [v for v in (values.get(r) for r in ("esi", "edi", "ebx"))
                   if v] if ldefs else []
    if want_value:
        print(f"[model->edit] make '{want_value}' the first-created "
              f"call-crossing pseudo (reference binds it to {want_reg})")

    # candidate edits: the library's creation-order movers + the param alias
    src_text = src_text0
    span = _fn_body_span(src_text, args.fn)
    muts = []
    if span is not None:
        body = src_text[span[0] + 1:span[1]]
        _ = body
        muts = _mutations(src_text, args.fn)
        params = _fn_params(src_text, args.fn)
        if want_value and want_value in params:
            for label, cat, new_body in _mut_alias_param(
                    src_text, args.fn, want_value):
                muts.append({"label": label, "catalog": cat,
                             "text": src_text[:span[0] + 1] + new_body
                             + src_text[span[1]:]})
        elif want_value == "this":
            adv = _handle_advice(args, want_value)
            if adv:
                print(f"[handle-order] {adv['verdict']}")
            else:
                print("[model] the value that must move first is `this` - no "
                      "statement-local spelling creates its pseudo earlier "
                      "(alias is copy-propagated, measured); the divergence "
                      "lives in the front-end handle state (catalog C1 class).")
    ranked = _rank_for_model(muts, klass,
                             [want_value] if want_value else pair_values)
    report["candidates"] = [m["label"] for m in ranked[:tries]]
    if not ranked:
        print("[why-reg v2] CAPPED by the model: the transposed pair is not "
              "source-nameable (parameter/`this` vs expression value) - "
              "the processing-order divergence is the front-end "
              "handle-state class (C1), not a statement-level knob. "
              "0 mutation compiles spent.")
        adv = _handle_advice(args, want_value) if want_value else None
        if adv and adv.get("levers"):
            print("[handle-order] movable lever(s): "
                  + "; ".join(adv["levers"][:2]))
        elif adv:
            print(f"[handle-order] {adv['verdict']}")
        if getattr(args, "json", False):
            print(json.dumps(report, indent=2))
        return 1

    print(f"[model] {len(ranked)} candidate(s) pass the model filter; "
          f"compiling top {min(tries, len(ranked))} "
          f"(v1 would sweep ~{len(muts)}):")
    mut_dir = SCRATCH / "mut"
    best = None
    results = []
    for m in ranked[:tries]:
        stem = f"v2_{_slug(m['label'])}"
        mut_dir.mkdir(parents=True, exist_ok=True)
        if unit:
            obj, tail = _unit.compile_text(m["text"], unit, mut_dir, stem)
        else:
            p = mut_dir / f"{stem}.cpp"
            p.write_text(m["text"])
            obj, tail = _compile_tu(p, mut_dir, _wine_dir(src.parent))
        if obj is None:
            print(f"  {m['label'][:56]:<58} compile-error (discarded)")
            results.append({"label": m["label"], "distance": None})
            continue
        try:
            text, _s = _fn_text(obj, args.fn)
        except SystemExit:
            print(f"  {m['label'][:56]:<58} fn not found (discarded)")
            results.append({"label": m["label"], "distance": None})
            continue
        dist = _align.distance(_align.parse_side(text), ref_recs)
        status = ("EXACT" if dist == 0 else
                  "improved" if dist < base_dist else "no improvement")
        print(f"  {m['label'][:56]:<58} distance {dist} ({dist - base_dist:+d})"
              f"  {status}")
        results.append({"label": m["label"], "distance": dist,
                        "file": str((mut_dir / f'{stem}.cpp')
                                    .relative_to(_common.REPO))})
        if dist < base_dist and (best is None or dist < best["distance"]):
            best = results[-1]
    report["results"] = results
    report["winner"] = best and best["label"]
    rc = 0 if (best and best["distance"] == 0) else 1
    if best:
        print(f"[recommended edit] {best['label']} -> {best['distance']} "
              "slot(s)" + (" - register-visible EXACT"
                           if best["distance"] == 0 else ""))
        mut_text = (_common.REPO / best["file"]).read_text()
        for ln in difflib.unified_diff(
                src_text.splitlines(), mut_text.splitlines(),
                fromfile=src.name, tofile=f"{src.name} (model edit)",
                lineterm=""):
            print("  " + ln)
        print("[why-reg proposes this edit; it never applies source changes]")
    else:
        print("[why-reg v2] the model-proposed edit(s) did not move the "
              "binding - the creation-order lever is copy-propagated or "
              "the order is fed by front-end handle state (C1 class); "
              "capped, with "
              f"{sum(1 for r in results if r['distance'] is not None)} "
              "compile(s) spent (v1's blind sweep would spend "
              f"~{len(muts)}).")
    if getattr(args, "json", False):
        print(json.dumps(report, indent=2))
    return rc


def main(argv=None) -> int:
    """Standalone CLI for the v2 model path (the `homm3 vc6 why-reg`
    subcommand keeps v1 as its default; pass --sweep here to force v1)."""
    import argparse
    ap = argparse.ArgumentParser(
        prog="python3 -m homm3.vc6.reg_model",
        description="why-reg v2 - model-backed register-binding predictor")
    ap.add_argument("src")
    ap.add_argument("--fn", required=True)
    ref = ap.add_mutually_exclusive_group(required=True)
    ref.add_argument("--against", metavar="UNIT:FN")
    ref.add_argument("--against-src", dest="against_src", metavar="FILE")
    ap.add_argument("--sweep", action="store_true",
                    help="run the v1 guided sweep instead of the model path")
    ap.add_argument("--tries", type=int, default=1,
                    help="model-ranked candidates to compile (default 1, max 3)")
    ap.add_argument("--il-order", dest="il_order", action="store_true",
                    help="also print the IL symbol-handle creation order")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    args.model = not args.sweep
    rc = run_why(args)
    import shlex
    _common.log_invocation(rc, shlex.join(
        ["python3", "-m", "homm3.vc6.reg_model", *(argv or sys.argv[1:])]))
    return rc


if __name__ == "__main__":
    sys.exit(main())
