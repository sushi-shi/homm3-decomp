"""homm3.vc6.flow_model - `why-branch`: guided oracle search over
control-flow spellings.

Which known source spelling reproduces the reference's JUMPS? The
control-flow twin of reg_model (`why-reg`): same compile -> slice ->
reference -> diagnose -> search -> rank flow, same --against/--against-src
contract, same rc 0/1/2 convention - but the residual it hunts is a CFG /
branch-shape divergence (loop rotation, merged vs duplicated exits,
ternary-vs-branch, switch emission order), NOT a register binding. v1
contains NO reverse-engineered flow-graph model (the fg.c/lg.c RE track is
a later phase): it takes the behavior catalog's explained D-levers
(docs/vc6/behavior-catalog.md, D family), applies each as a concrete
source mutation, and lets the REAL pinned compiler judge every candidate.
**The VC6 compile is the sole verdict on whether a mutation helps** -
mutants that do not compile are discarded by the oracle, never reasoned
about; site discovery only decides what to TRY, never what is true.

Site discovery is REGEX-GUIDED over the target function's body only (the
same deliberately narrow-but-reliable v1 stance as reg_model; libclang
rewriting was rejected there for fragility on VC6-era C++ and the verdict
carries over). Mutation classes implemented, with catalog IDs:

  loop-form        `while (c) {..}` -> `while (1) { if (!(c)) break; ..}`
                   / `for (;;) {..}` / `do {..} while (c)` / explicit
                   goto-loop, and the reverse rotation (D1/D2); induction
                   type short/int/long + descending `i-- > 0` (D10)
  merge-return     adjacent guard returns with one value -> goto-INTO-the-
                   shared-block (the path.cpp merged-return layout) or the
                   `||` merge (D4)
  nest-exits       flat `if (c) return V;` chain + final return -> nested
                   single-`return V` (the check_shipyard 8-branch/2-ret
                   shape, D5)
  ternary          `if (c) return A; return B;` <-> `return c ? A : B;`,
                   and assignment selectors both directions (D8)
  bool-flag        flag declaration type unsigned char <-> int/long (D13)
  case-order       adjacent case-block swaps + reversal, break-terminated
                   switches only (D9: emission order is source order)

Two loop rewrites are NOT semantics-preserving in general (`do..while`
changes the zero-trip case; descending induction reverses iteration
order). They stay in the menu deliberately: in this domain the RETAIL
BYTES are the semantic ground truth - a rewrite that reproduces them is
evidence the original source had that form - and every winning edit is
only ever proposed, never landed. Such labels say so ("retail arbitrates").

Pipeline per candidate: compile via homm3.core.cc_wrap with the game
profile (config/units.toml `game_o2_ml_gr_windows` + /GR-) + /FAs, slice
fn F from the COFF object with homm3.sema._asm's llvm-objdump path, and
score against the reference with _flow.distance (unpaired flow kinds +
unpaired branch tokens + ret delta; 0 = branch sequences agree - whatever
is left then is register allocation / spelling: hand off to why-reg).
The reference is either a second compiled TU (--against-src, hermetic)
or retail F via the sema machinery (--against UNIT:FN - the delinked
target object when one exists, else capstone over the image).

rc: 0 = zero branch-shape divergence reached (already, or by a mutation -
the winning edit prints as a diff for manual application, never
auto-applied); 1 = improved-but-not-exact or no mutation helped; 2 =
error (die). Scratch: build/vc6/whybranch/{base,ref,mut}/.
"""
from __future__ import annotations

import difflib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

from homm3.sema import _asm
from homm3.vc6 import _common, _flow, _source

SCRATCH = _common.REPO / "build/vc6/whybranch"

# Game C++ profile: config/units.toml `game_o2_ml_gr_windows` (minus the
# build-plumbing /nologo /c, added at invocation) + /GR- per the catalog's
# profile-under-test - identical to reg_model's.
GAME_FLAGS = ["/O2", "/Ob2", "/Oy-", "/Op", "/ML", "/Gr", "/GX", "/GR-",
              "/D_WINDOWS"]

MAX_MUTATIONS = 48

# --- compile + slice (deliberate mirror of reg_model's plumbing; kept local so
# --- the two solvers evolve independently and neither reaches into the other) ------


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
    from homm3.sema.context import get_context
    ctx = get_context()
    name, unit, rva, size, ordinal = ctx.symbols.resolve_fn(fnspec)
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
            "producer - branch kinds are producer-robust, but inline "
            "jump tables clip the stream)")


# --- mutation library (regex-guided; the compiler is the verdict) ------------------


# Body location is _source's job (shared with why-reg). v1 searched the
# MANGLED symbol verbatim, which never appears in source, so why-branch's
# guided search could only ever run on an already-demangled `--fn` - every
# real member definition, and the whole constructor population, died on
# "cannot locate the body". _source demangles the name, walks the
# member-initialiser list and masks `#if 0  // @carcass` regions.
_fn_body_span = _source.body_span


def _match(s: str, i: int, open_ch: str, close_ch: str):
    """Index of the close matching s[i] == open_ch, or None. Not comment/
    string aware - a mangled mutant simply fails to compile and the oracle
    discards it."""
    depth = 0
    while i < len(s):
        if s[i] == open_ch:
            depth += 1
        elif s[i] == close_ch:
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return None


def _invert(cond: str) -> str:
    if "&&" in cond or "||" in cond:
        return f"!({cond})"
    m = re.fullmatch(r"(.+?)\s*==\s*0", cond)
    if m:
        return m.group(1).strip()
    m = re.fullmatch(r"(.+?)\s*!=\s*0", cond)
    if m:
        return m.group(1).strip() + " == 0"
    m = re.fullmatch(r"![ \t]*(\w[\w>\-.\[\]]*)", cond)
    if m:
        return m.group(1)
    return f"!({cond})"


def _mut_loop_form(body: str):
    """D1/D2: the loop-form family, both directions."""
    sites = 0
    for m in re.finditer(r"\bwhile[ \t]*\(", body):
        if sites >= 3:
            break
        cp = _match(body, m.end() - 1, "(", ")")
        if cp is None:
            continue
        cond = body[m.end():cp].strip()
        j = cp + 1
        while j < len(body) and body[j].isspace():
            j += 1
        if j >= len(body) or body[j] != "{":
            continue  # single-statement loop body: v1 requires braces
        bp = _match(body, j, "{", "}")
        if bp is None:
            continue
        inner = body[j + 1:bp]
        head, tail = body[:m.start()], body[bp + 1:]
        sites += 1

        if cond in ("1", "true"):
            # reverse direction: top-tested -> the rotatable plain form
            g = re.match(r"\s*if[ \t]*\(", inner)
            if not g:
                continue
            gp = _match(inner, g.end() - 1, "(", ")")
            if gp is None:
                continue
            brk = re.match(r"[ \t]*break[ \t]*;", inner[gp + 1:])
            if brk is None:
                continue
            inv = _invert(inner[g.end():gp].strip())
            rest = inner[gp + 1 + brk.end():]
            yield (f"rotate top-tested loop to `while ({inv})`", "D1/D2",
                   head + f"while ({inv}) {{{rest}}}" + tail)
            yield (f"bottom-test loop as `do..while ({inv})` (zero-trip "
                   "semantics - retail arbitrates)", "D2",
                   head + f"do {{{rest}}} while ({inv});" + tail)
            continue

        guards = [f"if (!({cond})) break;"]
        if not re.search(r"[<>!=]=|[<>]|&&|\|\|", cond):
            guards.append(f"if (({cond}) == 0) break;")
        for g in guards:
            yield (f"unrotate loop: `while (1) {{ {g} .. }}`", "D2",
                   head + "while (1) { " + g + inner + "}" + tail)
        yield (f"unrotate loop: `for (;;) {{ if (!({cond})) break; .. }}`",
               "D2", head + "for (;;) { " + f"if (!({cond})) break;"
               + inner + "}" + tail)
        yield (f"bottom-test loop as `do..while ({cond})` (zero-trip "
               "semantics - retail arbitrates)", "D2",
               head + f"do {{{inner}}} while ({cond});" + tail)
        yield ("explicit goto-loop transcription", "D1/D2",
               head + "{ __wb_top: if (!(" + cond + ")) goto __wb_out;"
               + inner + " goto __wb_top; __wb_out:; }" + tail)


_FOR_HDR = re.compile(
    r"\bfor[ \t]*\([ \t]*(?P<ty>(?:unsigned[ \t]+)?(?:short|int|long|char))"
    r"[ \t]+(?P<v>\w+)[ \t]*=[ \t]*(?P<init>[^;()]+?)[ \t]*;"
    r"[ \t]*(?P<cond>[^;()]*?)[ \t]*;[ \t]*(?P<step>[^;()]*?)[ \t]*\)")


def _mut_for_induction(body: str):
    """D10: induction-variable type and direction."""
    n = 0
    for m in _FOR_HDR.finditer(body):
        if n >= 3:
            break
        n += 1
        ty, v = m.group("ty"), m.group("v")
        for nt in ("short", "int", "long"):
            if nt == ty:
                continue
            yield (f"induction '{v}' as {nt} (was {ty})", "D10",
                   body[:m.start("ty")] + nt + body[m.end("ty"):])
        mc = re.fullmatch(rf"{re.escape(v)}[ \t]*<[ \t]*(\w+)",
                          m.group("cond"))
        if (mc and m.group("init").strip() == "0"
                and m.group("step").replace(" ", "") in (f"++{v}", f"{v}++")):
            hdr = f"for ({ty} {v} = {mc.group(1)}; {v}-- > 0;)"
            yield (f"descending induction `{v}-- > 0` (iteration ORDER "
                   "reversed - retail arbitrates)", "D10",
                   body[:m.start()] + hdr + body[m.end():])


_GRET = re.compile(r"^(?P<ind>[ \t]*)if[ \t]*\((?P<cond>[^{};]+)\)[ \t]*"
                   r"return\b(?P<val>[^;]*);[ \t]*$")
_PRET = re.compile(r"^(?P<ind>[ \t]*)(?:else[ \t]+)?return\b"
                   r"(?P<val>[^;]*);[ \t]*$")


def _mut_merge_return(body: str):
    """D4: adjacent guard returns with one value -> the merged-block-
    between-the-guards placement (goto INTO the block) or the || merge."""
    lines = body.split("\n")
    for i in range(len(lines) - 1):
        m1, m2 = _GRET.match(lines[i]), _GRET.match(lines[i + 1])
        if not (m1 and m2):
            continue
        if m1.group("val").strip() != m2.group("val").strip():
            continue
        ind, c1 = m1.group("ind"), m1.group("cond").strip()
        c2, val = m2.group("cond").strip(), m1.group("val")
        merged = [f"{ind}if ({c1}) goto __wb_merge;",
                  f"{ind}if ({c2})",
                  f"{ind}__wb_merge: return{val};"]
        yield (f"merge guard returns via goto-into-block ('{c1}' / '{c2}')",
               "D4", "\n".join(lines[:i] + merged + lines[i + 2:]))
        yield (f"merge guard returns via `||` ('{c1}' / '{c2}')", "D4",
               "\n".join(lines[:i] + [f"{ind}if (({c1}) || ({c2})) "
                                      f"return{val};"] + lines[i + 2:]))


def _mut_nest_exits(body: str):
    """D5: a flat chain of >=2 same-value guard returns + a final return
    -> nested ifs with ONE textual fail return (the 2-ret merged shape)."""
    lines = body.split("\n")
    i = 0
    while i < len(lines):
        m0 = _GRET.match(lines[i])
        if not m0:
            i += 1
            continue
        val = m0.group("val").strip()
        run = [m0]
        j = i + 1
        while j < len(lines):
            mg = _GRET.match(lines[j])
            if not mg or mg.group("val").strip() != val:
                break
            run.append(mg)
            j += 1
        mp = _PRET.match(lines[j]) if j < len(lines) else None
        if len(run) >= 2 and mp and mp.group("val").strip() != val:
            ind = run[0].group("ind")
            new = []
            for k, mg in enumerate(run):
                new.append(ind + "    " * k
                           + f"if (!({mg.group('cond').strip()})) {{")
            new.append(ind + "    " * len(run) + f"return{mp.group('val')};")
            for k in range(len(run) - 1, -1, -1):
                new.append(ind + "    " * k + "}")
            new.append(f"{ind}return {val};")
            yield (f"nest {len(run)} flat guard returns into one "
                   f"`return {val}` exit", "D5",
                   "\n".join(lines[:i] + new + lines[j + 1:]))
        i = j if j > i else i + 1


def _split_ternary(e: str):
    """(cond, a, b) of a TOP-LEVEL `?:` in expression e, else None."""
    if "::" in e:
        return None
    depth = 0
    for idx, ch in enumerate(e):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif ch == "?" and depth == 0:
            tern = d2 = 0
            for k in range(idx + 1, len(e)):
                c2 = e[k]
                if c2 in "([":
                    d2 += 1
                elif c2 in ")]":
                    d2 -= 1
                elif c2 == "?" and d2 == 0:
                    tern += 1
                elif c2 == ":" and d2 == 0:
                    if tern == 0:
                        return e[:idx], e[idx + 1:k], e[k + 1:]
                    tern -= 1
            return None
    return None


_GASGN = re.compile(r"^(?P<ind>[ \t]*)if[ \t]*\((?P<cond>[^{};]+)\)[ \t]*"
                    r"(?P<lhs>[\w>\-.\[\]]+)[ \t]*=[ \t]*"
                    r"(?P<a>[^;=][^;]*?)[ \t]*;[ \t]*$")
_EASGN = re.compile(r"^(?P<ind>[ \t]*)else[ \t]+(?P<lhs>[\w>\-.\[\]]+)"
                    r"[ \t]*=[ \t]*(?P<b>[^;=][^;]*?)[ \t]*;[ \t]*$")
_ASGN_LINE = re.compile(r"^(?P<ind>[ \t]*)(?P<lhs>[\w>\-.\[\]]+)[ \t]*="
                        r"[ \t]*(?P<e>[^;=][^;]*?)[ \t]*;[ \t]*$")


def _mut_ternary(body: str):
    """D8: ternary <-> branch, on return expressions and assignment
    selectors, both directions."""
    lines = body.split("\n")
    out = 0
    for i in range(len(lines)):
        if out >= 8:
            return
        # if (c) return A; / [else] return B;  ->  return c ? A : B;
        if i + 1 < len(lines):
            m1, m2 = _GRET.match(lines[i]), _PRET.match(lines[i + 1])
            if m1 and m2 and m1.group("val").strip() \
                    and m2.group("val").strip():
                ind, c = m1.group("ind"), m1.group("cond").strip()
                a, b = m1.group("val").strip(), m2.group("val").strip()
                out += 1
                yield (f"collapse returns to ternary `({c}) ? .. : ..`",
                       "D8", "\n".join(
                           lines[:i] + [f"{ind}return ({c}) ? ({a}) : ({b});"]
                           + lines[i + 2:]))
            # if (c) x = A; / else x = B;  ->  x = c ? A : B;
            g1, g2 = _GASGN.match(lines[i]), _EASGN.match(lines[i + 1])
            if g1 and g2 and g1.group("lhs") == g2.group("lhs"):
                ind, c = g1.group("ind"), g1.group("cond").strip()
                lhs = g1.group("lhs")
                out += 1
                yield (f"collapse `{lhs}` selector to ternary", "D8",
                       "\n".join(lines[:i]
                                 + [f"{ind}{lhs} = ({c}) ? "
                                    f"({g1.group('a')}) : ({g2.group('b')});"]
                                 + lines[i + 2:]))
        # return c ? A : B;  ->  if (c) return A; return B;
        mp = _PRET.match(lines[i])
        if mp and not _GRET.match(lines[i]):
            parts = _split_ternary(mp.group("val").strip())
            if parts:
                c, a, b = (p.strip() for p in parts)
                ind = mp.group("ind")
                out += 1
                yield (f"unfold return ternary `({c}) ? ..` into branches",
                       "D8", "\n".join(
                           lines[:i] + [f"{ind}if ({c}) return {a};",
                                        f"{ind}return {b};"]
                           + lines[i + 1:]))
        # x = c ? A : B;  ->  if (c) x = A; else x = B;
        ma = _ASGN_LINE.match(lines[i])
        if ma:
            parts = _split_ternary(ma.group("e"))
            if parts:
                c, a, b = (p.strip() for p in parts)
                ind, lhs = ma.group("ind"), ma.group("lhs")
                out += 1
                yield (f"unfold `{lhs}` ternary into if/else", "D8",
                       "\n".join(lines[:i]
                                 + [f"{ind}if ({c}) {lhs} = {a};",
                                    f"{ind}else {lhs} = {b};"]
                                 + lines[i + 1:]))


_FLAG_DECL = re.compile(
    r"^(?P<ind>[ \t]*)(?P<ty>unsigned[ \t]+char|signed[ \t]+char|char|int"
    r"|long|bool)[ \t]+(?P<n>\w+)[ \t]*=[ \t]*"
    r"(?P<init>[^;]*(?:[!=]=|<|>)[^;]*?)[ \t]*;[ \t]*$")


def _mut_bool_flag(body: str):
    """D13: flip a comparison-initialized flag between the byte spelling
    (`unsigned char f = e != 0;` -> setcc) and the int compare."""
    lines = body.split("\n")
    hits = 0
    for i, ln in enumerate(lines):
        m = _FLAG_DECL.match(ln)
        if not m:
            continue
        hits += 1
        if hits > 3:
            break
        ty = re.sub(r"[ \t]+", " ", m.group("ty"))
        targets = (("int", "long") if ("char" in ty or ty == "bool")
                   else ("unsigned char",))
        for nt in targets:
            yield (f"flag '{m.group('n')}' as {nt} (was {ty})", "D13",
                   "\n".join(lines[:i]
                             + [f"{m.group('ind')}{nt} {m.group('n')} = "
                                f"{m.group('init')};"] + lines[i + 1:]))


_CASE_LABEL = re.compile(r"(?:case\b[^:]*|default[ \t]*):")


def _split_cases(sbody: str):
    """(head lines, [case blocks as line lists]) or None. Each depth-0
    `case`/`default` label line starts its own block; a stacked label is
    a label-only block, which fails _terminated and skips the switch -
    fallthrough is never reordered."""
    lines = sbody.split("\n")
    depth = 0
    starts = []
    for i, ln in enumerate(lines):
        if depth == 0 and _CASE_LABEL.match(ln.strip()):
            starts.append(i)
        depth += ln.count("{") - ln.count("}")
    if len(starts) < 2:
        return None
    bounds = starts + [len(lines)]
    return lines[:starts[0]], [lines[bounds[k]:bounds[k + 1]]
                               for k in range(len(starts))]


def _terminated(block: list) -> bool:
    for ln in reversed(block):
        s = ln.strip()
        if not s or s.strip("}").strip() == "":
            continue
        s = s.rstrip("}").strip()
        return bool(re.search(r"(?:\bbreak|\breturn\b[^;]*|\bgoto[ \t]+\w+"
                              r"|\bcontinue)[ \t]*;$", s))
    return False


def _mut_case_order(body: str):
    """D9: emission order is source order - permute break-terminated case
    blocks (adjacent swaps + full reversal)."""
    done = 0
    for m in re.finditer(r"\bswitch[ \t]*\(", body):
        if done >= 2:
            break
        cp = _match(body, m.end() - 1, "(", ")")
        if cp is None:
            continue
        j = cp + 1
        while j < len(body) and body[j].isspace():
            j += 1
        if j >= len(body) or body[j] != "{":
            continue
        bp = _match(body, j, "{", "}")
        if bp is None:
            continue
        split = _split_cases(body[j + 1:bp])
        if split is None:
            continue
        head, blocks = split
        if not all(_terminated(b) for b in blocks):
            continue  # fallthrough somewhere - reorder would change meaning
        done += 1

        def rebuild(order):
            newlines = head + [ln for k in order for ln in blocks[k]]
            return body[:j + 1] + "\n".join(newlines) + body[bp:]

        idx = list(range(len(blocks)))
        for k in range(min(len(blocks) - 1, 4)):
            order = idx[:]
            order[k], order[k + 1] = order[k + 1], order[k]
            yield (f"swap case blocks #{k}/#{k + 1} (emission order)",
                   "D9", rebuild(order))
        if len(blocks) > 2:
            yield ("reverse case-block order (emission order)", "D9",
                   rebuild(idx[::-1]))


def _mutations(src_text: str, fn: str) -> list:
    """Candidate mutants: [{'label','catalog','text'}], deduped, capped.

    Callers must have checked `_fn_body_span` first: a missing body is a
    diagnosable state (compiler-generated entity, fenced carcass, body in
    another TU), not a harness error, and run_why reports it as one.
    """
    span = _fn_body_span(src_text, fn)
    if span is None:
        return []
    open_b, close_b = span
    body = src_text[open_b + 1:close_b]
    out, seen = [], {body}
    for gen in (_mut_loop_form, _mut_for_induction, _mut_merge_return,
                _mut_nest_exits, _mut_ternary, _mut_bool_flag,
                _mut_case_order):
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


def run_why(args) -> int:
    src = Path(args.src).resolve()
    if not src.is_file():
        _common.die(f"source missing: {src}")
    for sub in ("base", "ref", "mut"):
        shutil.rmtree(SCRATCH / sub, ignore_errors=True)

    base_obj, tail = _compile_tu(src, SCRATCH / "base",
                                 _wine_dir(src.parent))
    if base_obj is None:
        _common.die(f"base TU failed to compile:\n{tail}")
    base_text, base_sym = _fn_text(base_obj, args.fn)
    ref_text, ref_label = _reference_side(args)

    base_prof = _flow.profile(base_text)
    ref_prof = _flow.profile(ref_text)
    if not base_prof["n"]:
        _common.die(f"no instructions parsed for {base_sym} in the base obj")
    if not ref_prof["n"]:
        _common.die("no instructions parsed on the reference side")
    base_dist = _flow.distance(base_prof, ref_prof)
    findings, stats = _flow.diagnose(base_prof, ref_prof)

    results = []
    src_text = src.read_text()
    body_found = _fn_body_span(src_text, args.fn) is not None
    if base_dist > 0 and body_found:
        mut_dir = SCRATCH / "mut"
        include_dir = _wine_dir(src.parent)
        for i, mut in enumerate(_mutations(src_text, args.fn)):
            mut_src = mut_dir / f"m{i:02d}_{_slug(mut['label'])}.cpp"
            mut_dir.mkdir(parents=True, exist_ok=True)
            mut_src.write_text(mut["text"])
            row = {"label": mut["label"], "catalog": mut["catalog"],
                   "file": str(mut_src.relative_to(_common.REPO)),
                   "distance": None, "delta": None, "status": ""}
            obj, err = _compile_tu(mut_src, mut_dir, include_dir)
            if obj is None:
                row["status"] = "compile-error (discarded)"
            else:
                try:
                    text, _sym = _fn_text(obj, args.fn)
                except SystemExit:
                    row["status"] = "fn not found post-mutation (discarded)"
                    results.append(row)
                    continue
                dist = _flow.distance(_flow.profile(text), ref_prof)
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

    print(f"[why-branch] {args.fn} ({base_sym}) in {src.name}")
    print(f"[base]      {base_obj.relative_to(_common.REPO)}  "
          f"{stats['base_n']} instruction(s), {stats['base_branches']} "
          f"branch(es), {stats['base_rets']} ret(s), "
          f"{stats['base_blocks']} block(s)")
    print(f"[reference] {ref_label}  {stats['target_n']} instruction(s), "
          f"{stats['target_branches']} branch(es), {stats['target_rets']} "
          f"ret(s), {stats['target_blocks']} block(s)")
    print(f"[distance]  {base_dist} branch-shape disagreement(s) "
          "(unpaired flow kinds + unpaired branch tokens + ret delta; "
          "0 = branch sequences agree)")
    print("[diagnosis]")
    if findings:
        for f in findings:
            print(f"  {f['catalog']:<9} {f['kind']:<13} {f['detail']}")
    else:
        print("  (no recognized control-flow pattern - the shapes differ "
              "but match no cataloged D signature)"
              if base_dist else "  (none - branch shapes already agree)")
    if base_dist == 0:
        print("[why-branch] base already matches the reference at the "
              "branch-shape grade; whatever residual remains is a register "
              "binding / instruction selection - hand off to `why-reg`.")
        return 0

    if not body_found:
        print(f"[guided search] SKIPPED - {_source.explain_miss(src_text, args.fn)}"
              f"\n                (searched {src.name} for "
              f"{' / '.join(_source.source_names(args.fn)) or '<no source name>'}); "
              "diagnosis-only - apply the indicated D-lever by hand.")
        return 1
    print(f"[guided search] {len(results)} candidate mutation(s) from the "
          "catalog D-lever classes"
          + ("" if results else " - no applicable sites found"))
    if results:
        print(f"  {'mutation':<56} {'catalog-id':<10} {'Ddistance':>9} "
              f"{'new-distance':>12}")
        ranked = scored + [r for r in results if r["distance"] is None]
        for r in ranked:
            if r["distance"] is None:
                print(f"  {r['label'][:56]:<56} {r['catalog']:<10} "
                      f"{'-':>9} {'-':>12}  {r['status']}")
            else:
                print(f"  {r['label'][:56]:<56} {r['catalog']:<10} "
                      f"{r['delta']:>+9} {r['distance']:>12}  "
                      f"{r['status']}")
    if winner is None:
        print("[why-branch] no mutation moved the branch shape toward the "
              "reference - the lever is outside this library (or the "
              "divergence is not source-addressable; see the catalog's "
              "open D classes, e.g. D3 threading and D6 retail-side "
              "duplication).")
        return 1
    print(f"[recommended edit] {winner['label']} ({winner['catalog']}) -> "
          f"{winner['distance']} disagreement(s)"
          + (" - branch-shape EXACT (hand any residual to why-reg)"
             if winner["distance"] == 0 else
             " - best improvement, not exact"))
    mut_text = (_common.REPO / winner["file"]).read_text()
    diff = difflib.unified_diff(
        src_text.splitlines(), mut_text.splitlines(),
        fromfile=str(src.name), tofile=f"{src.name} (mutated)", lineterm="")
    for ln in diff:
        print("  " + ln)
    print("[why-branch proposes this edit; it never applies source changes]")
    return rc
