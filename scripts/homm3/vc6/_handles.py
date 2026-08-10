"""homm3.vc6._handles - the front-end symbol-HANDLE order model (phase 5).

C1XX numbers every symbol it creates from ONE global u16 counter (the IL
handle counter, docs/vc6/il-format.md section 2); C2's codegen is a
deterministic function of the IL bytes those numbers are baked into (the
killer result, il-format.md section 5).  This module models HOW the
numbers are assigned from source, so the two wall classes that share that
root - the B1 register-swap residue and the C1 include-set sensitivity -
become statements about declarations instead of "converge the TU".

Measured assignment order (2026-08-10, pinned C1XX 12.00.8472, game
profile, probe battery under docs/vc6/handle-order.md):

* STRICTLY TOP-TO-BOTTOM AT PARSE POINT.  A declaration shifts every
  symbol created after it by its own cost and nothing before it (a
  struct AFTER a function raises the high-water +9 and shifts that
  function's handles +0).  Costs are additive (two structs = +18).
* Within a function definition: parameters in source order, then `this`
  (a real per-function symbol record in `sy`), then the file symbol
  (once per TU, at the FIRST function body), then the block symbol,
  then locals in creation order.  `this` is NOT source-spellable and is
  always minted between the parameters and the body - the relative
  creation order params < this < locals is parse-FIXED.
* Redeclarations are free: a repeated forward tag / typedef never mints
  a second handle.

Measured per-declaration cost (high-water delta), the DECL_COST table
below; the flagship +9 (`struct probe0_t { int a; };`, il-format.md) is
tag(1) + class-definition overhead(7) + member(1).  The overhead 7 is
consistent with injected-class-name(1) + implicit default ctor(1) +
copy ctor(1+1 param) + dtor(1) + operator=(1+1 param) under the same
"function = 1 + #params" rule measured at file scope; the TOTAL is the
measured fact, the decomposition is the consistent reading.

Honesty bounds:
* predict_decl_cost / the scanner cover the C-like declaration subset
  (aggregates, enums, typedefs, function decls, variables) plus the
  measured C++ extras (methods, virtual, static members, explicit
  dtor).  Templates and anything unrecognized cost None and are
  reported as UNPREDICTED, never guessed.
* Absolute handle prediction is exact only for include-free simple TUs
  (validated against captures); for rich TUs the validated currency is
  the DELTA - what a source change shifts, by how much, and from which
  symbol on.  The capture (via homm3.vc6.il) stays the oracle.
"""
from __future__ import annotations

import difflib
import re
from dataclasses import dataclass, field
from pathlib import Path

from homm3.vc6 import _il

# ---------------------------------------------------------------------------
# measured constants (probe battery, docs/vc6/handle-order.md section 2)
# ---------------------------------------------------------------------------

BASE_CPP = 0xD5   # first user handle, C++ front end, game profile, no includes
BASE_C = 0x58     # first user handle, C front end (C1.DLL), /O2 /ML /Gr
CLASS_OVERHEAD = 7  # C++ class-DEFINITION overhead beyond the tag (see above)

# Per-declaration handle cost, measured as gl high-water deltas:
#   tag_fwd        struct S;                      +1   (repeats free)
#   typedef        typedef int T;                 +1   (repeats free)
#   var            int g; / extern int g;         +1   per declarator
#   enum_def       enum E { .. };                 +1 + #enumerators
#   fn_decl        int f(int, int);               +1 + #params
#   aggregate def  struct/class/union { .. };     +1 (tag) + 7 (C++ overhead)
#                                                 + 1 per data member
#                                                 + (1 + #params) per method
#                                                 + 3 once if any virtual
#                                                 + 4 for an explicit dtor
#                                                 + 2 per static data member
#                                                 + 0 per base class
#                                                 (+ nested aggregates recurse)
#   fn_def         params(+1 each) + fn(+1) + block(+1) + locals(+1 each)
#                  + file symbol once per TU at the FIRST body (+1)
#   C mode         aggregate def = 1 (tag) + 1 per member (no C++ overhead)
VIRTUAL_BONUS = 3
EXPLICIT_DTOR_BONUS = 4
STATIC_MEMBER_COST = 2


# ---------------------------------------------------------------------------
# the file-scope declaration scanner (honest C-like subset)
# ---------------------------------------------------------------------------

@dataclass
class Decl:
    kind: str          # tag_fwd typedef var enum_def fn_decl fn_def aggregate
    name: str
    text: str          # normalized one-line spelling (truncated)
    cost: int | None   # None = unpredicted kind
    note: str = ""


_LINE_COMMENT = re.compile(r"//[^\n]*")
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
_PREPROC = re.compile(r"(?m)^\s*#[^\n]*(\\\n[^\n]*)*")
_STRING = re.compile(r'"(?:[^"\\\n]|\\.)*"|' + r"'(?:[^'\\\n]|\\.)*'")


def _strip(text: str) -> str:
    text = _BLOCK_COMMENT.sub(" ", text)
    text = _LINE_COMMENT.sub(" ", text)
    text = _PREPROC.sub(" ", text)
    return _STRING.sub('""', text)


_NEEDS_SEMI = re.compile(
    r"^\s*(?:typedef\s+)?(?:struct|class|union|enum)\b|=[^{]*$")


def _split_top(text: str) -> list[str]:
    """Split file-scope text into declaration chunks.  A chunk ends at a
    ';' at brace depth 0; a '}' returning to depth 0 also ends it UNLESS
    the chunk is an aggregate/enum (declarators may follow) or an
    initializer (`= {..}`) - those wait for their ';'."""
    out, buf, depth = [], [], 0
    for ch in text:
        buf.append(ch)
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                head = "".join(buf)
                if not _NEEDS_SEMI.match(head.split("{", 1)[0]):
                    out.append(head.strip())   # function definition body
                    buf = []
        elif ch == ";" and depth == 0:
            chunk = "".join(buf).strip()
            if chunk and chunk != ";":
                out.append(chunk)
            buf = []
        elif ch == "\n" and depth == 0 and buf and "".join(buf).strip() == "":
            buf = []
    tail = "".join(buf).strip()
    if tail:
        out.append(tail)
    return out


_AGG_HEAD = re.compile(
    r"^(?:typedef\s+)?(struct|class|union)\s*([A-Za-z_]\w*)?\s*(?::[^{;]*)?([{;])")
_ENUM_HEAD = re.compile(r"^(?:typedef\s+)?enum\s*([A-Za-z_]\w*)?\s*([{;])")
_FN_SIG = re.compile(r"([A-Za-z_~][\w:]*)\s*\(([^()]*)\)\s*(?:const\s*)?([;{])")


def _count_params(paramlist: str) -> int:
    p = paramlist.strip()
    if p in ("", "void"):
        return 0
    return p.count(",") + 1


def _body_of(chunk: str) -> str | None:
    i = chunk.find("{")
    if i < 0:
        return None
    depth, j = 0, i
    for j in range(i, len(chunk)):
        if chunk[j] == "{":
            depth += 1
        elif chunk[j] == "}":
            depth -= 1
            if depth == 0:
                return chunk[i + 1:j]
    return chunk[i + 1:]


def _aggregate_cost(body: str, cxx: bool = True,
                    tag: str = "") -> tuple[int, list[str]]:
    """Cost of one aggregate DEFINITION (tag + overhead + members),
    recursing into nested aggregates.  *tag* names the class so explicit
    ctors are recognized (measured: `S();` replaces the implicit default
    ctor - net +0; a parametric ctor is extrapolated as +#params)."""
    notes: list[str] = []
    cost = 1 + (CLASS_OVERHEAD if cxx else 0)
    virtual_seen = False
    for member in _split_top(body):
        m = _AGG_HEAD.match(member)
        if m and m.group(3) == "{":
            inner, inotes = _aggregate_cost(_body_of(member) or "", cxx,
                                            m.group(2) or "")
            cost += inner
            notes += inotes
            continue
        fm = _FN_SIG.search(member)
        if fm and not member.startswith("enum"):
            fname = fm.group(1)
            params = _count_params(fm.group(2))
            if fname.startswith("~"):
                n = EXPLICIT_DTOR_BONUS
                notes.append("explicit dtor +4 (measured; net over the "
                             "implicit dtor it replaces)")
            elif fname == tag or member.strip().startswith(fname + "("):
                n = params   # ctor: (1 + #params) - the implicit defctor's 1
                if params:
                    notes.append("parametric ctor cost extrapolated "
                                 "(+#params; calibrated point is S() = +0)")
            else:
                n = 1 + params
            if "virtual" in member and not virtual_seen:
                virtual_seen = True
                n += VIRTUAL_BONUS
            cost += n
            continue
        if member.startswith("static"):
            cost += STATIC_MEMBER_COST
            continue
        # data member(s): one per declarator
        cost += member.count(",") + 1
    return cost, notes


def scan_decls(text: str, cxx: bool = True) -> list[Decl]:
    """File-scope declarations of *text* with their measured handle costs.

    Repeated forward tags / typedefs are recognized as free.  Unrecognized
    chunks (templates, macros the strip left behind, ...) become
    kind='unknown', cost=None - reported, never guessed.
    """
    out: list[Decl] = []
    seen: set[tuple[str, str]] = set()
    for chunk in _split_top(_strip(text)):
        one = " ".join(chunk.split())
        label = one[:64]
        m = _AGG_HEAD.match(one)
        if m:
            kind, name, brace = m.group(1), m.group(2) or "<anon>", m.group(3)
            if brace == ";" and "{" not in one:
                key = ("tag", name)
                cost = 0 if key in seen else 1
                seen.add(key)
                out.append(Decl("tag_fwd", name, label, cost,
                                "repeat, free" if cost == 0 else ""))
                continue
            cost, notes = _aggregate_cost(_body_of(one) or "", cxx,
                                          m.group(2) or "")
            seen.add(("tag", name))
            if one.startswith("typedef"):
                cost += 1  # the typedef name itself
            # trailing declarators after '}': `} x, y;`
            tail = one[one.rfind("}") + 1:].strip(" ;")
            if tail and not one.startswith("typedef"):
                cost += tail.count(",") + 1
            out.append(Decl("aggregate", name, label, cost, "; ".join(notes)))
            continue
        m = _ENUM_HEAD.match(one)
        if m:
            if m.group(2) == ";":
                key = ("tag", m.group(1) or "<anon>")
                cost = 0 if key in seen else 1
                seen.add(key)
                out.append(Decl("tag_fwd", m.group(1) or "<anon>", label, cost))
                continue
            body = _body_of(one) or ""
            n_enum = len([e for e in body.split(",") if e.strip()])
            cost = 1 + n_enum + (1 if one.startswith("typedef") else 0)
            out.append(Decl("enum_def", m.group(1) or "<anon>", label, cost))
            continue
        if one.startswith("typedef"):
            name = one.rstrip(";").split()[-1].lstrip("*&")
            key = ("typedef", name)
            cost = 0 if key in seen else 1
            seen.add(key)
            out.append(Decl("typedef", name, label, cost,
                            "repeat, free" if cost == 0 else ""))
            continue
        fm = _FN_SIG.search(one)
        if fm:
            params = _count_params(fm.group(2))
            if fm.group(3) == "{" or one.endswith("}"):
                # definition: params + fn + block (+ locals, unpredicted here)
                out.append(Decl("fn_def", fm.group(1), label, params + 2,
                                "+1/block +1/local inside the body, "
                                "+1 file symbol at the TU's first body"))
            else:
                out.append(Decl("fn_decl", fm.group(1), label, 1 + params))
            continue
        if re.match(r"^(?:extern\s+|static\s+|const\s+|unsigned\s+|signed\s+)*"
                    r"[A-Za-z_]\w*[\w\s*&]*[A-Za-z_]\w*(\s*\[[^\]]*\])*"
                    r"(\s*=[^;]*)?;?$", one):
            ndecl = one.split("=")[0].count(",") + 1
            out.append(Decl("var", one.rstrip(";").split()[-1].lstrip("*&"),
                            label, ndecl))
            continue
        out.append(Decl("unknown", "?", label, None, "UNPREDICTED kind"))
    return out


def predict_decl_cost(decl_text: str, cxx: bool = True) -> dict:
    """Predicted high-water delta for *decl_text* added at file scope.

    Returns {'total': int|None, 'decls': [Decl], 'unpredicted': [labels]}.
    total is None when any chunk is unpredicted."""
    decls = scan_decls(decl_text, cxx)
    unpred = [d.text for d in decls if d.cost is None]
    total = None if unpred else sum(d.cost for d in decls)
    return {"total": total, "decls": decls, "unpredicted": unpred}


def predict_handles(tu_source: str, cxx: bool = True) -> dict:
    """Absolute handle ranges per file-scope declaration, base BASE_CPP.

    EXACT only for include-free simple TUs (the validated class); rich TUs
    get the same walk with 'approximate': True.  Function definitions place
    params/fn/block but not their locals (locals extend the range)."""
    decls = scan_decls(tu_source, cxx)
    base = BASE_CPP if cxx else BASE_C
    counter = base
    rows, first_body_seen, approximate = [], False, False
    for d in decls:
        if d.cost is None:
            approximate = True
            rows.append({"decl": d, "start": counter, "end": None})
            continue
        cost = d.cost
        if d.kind == "fn_def" and not first_body_seen:
            first_body_seen = True
            cost += 1  # the file symbol, minted at the TU's first body
        rows.append({"decl": d, "start": counter, "end": counter + cost - 1})
        counter += cost
        if d.kind == "fn_def":
            approximate = True  # locals/literals inside the body not counted
    return {"base": base, "rows": rows, "highwater_at_least": counter,
            "approximate": approximate}


# ---------------------------------------------------------------------------
# capture plumbing (the oracle side; imports wine machinery lazily)
# ---------------------------------------------------------------------------

@dataclass
class HandleCapture:
    src: Path
    highwater: int | None
    gl_names: list[dict] = field(default_factory=list)   # {off,handle,name}
    sy_names: list[dict] = field(default_factory=list)
    streams: dict = field(default_factory=dict)          # suffix -> Path


def capture_handles(src: Path, flags: list[str] | None = None,
                    workdir: Path | None = None,
                    shadow: dict[str, str] | None = None) -> HandleCapture:
    """One IL capture of *src*, read back as handle tables (gl + sy scans).
    Wine/toolchain gating happens inside homm3.vc6.il."""
    from homm3.vc6 import il as _iltap
    _iltap._ensure_wine_env()
    _iltap._gate_subjects()
    flags = list(flags or _iltap.DEFAULT_FLAGS)
    workdir = workdir or (_iltap.IL_DIR / "handles" / src.stem)
    cap = _iltap.capture(src, flags, workdir, shadow=shadow)
    gl = cap["gl"].read_bytes()
    sy = cap["sy"].read_bytes()
    hw = _il.gl_highwater(gl)
    return HandleCapture(src=src, highwater=hw,
                         gl_names=_il.scan_names(gl, hw, min_len=3),
                         sy_names=_il.scan_names(sy, hw, min_len=1),
                         streams=cap)


def creation_table(cap: HandleCapture) -> list[dict]:
    """gl+sy named records merged, sorted by handle = creation order."""
    rows = [{"handle": r["handle"], "name": r["name"], "stream": s}
            for s, names in (("gl", cap.gl_names), ("sy", cap.sy_names))
            for r in names]
    rows.sort(key=lambda r: (r["handle"], r["stream"]))
    dedup, seen = [], set()
    for r in rows:
        key = (r["handle"], r["name"])
        if key not in seen:
            seen.add(key)
            dedup.append(r)
    return dedup


# ---------------------------------------------------------------------------
# handle_delta - the diff/advisor (which handles shifted, why)
# ---------------------------------------------------------------------------

def _added_removed(text_a: str, text_b: str) -> tuple[str, str]:
    """Line-level added/removed text B vs A (attribution input)."""
    add, rem = [], []
    for ln in difflib.unified_diff(text_a.splitlines(), text_b.splitlines(),
                                   lineterm="", n=0):
        if ln.startswith("+") and not ln.startswith("+++"):
            add.append(ln[1:])
        elif ln.startswith("-") and not ln.startswith("---"):
            rem.append(ln[1:])
    return "\n".join(add), "\n".join(rem)


def handle_delta(src_a: Path, src_b: Path,
                 flags: list[str] | None = None,
                 shadow_a: dict[str, str] | None = None,
                 shadow_b: dict[str, str] | None = None,
                 workdir: Path | None = None) -> dict:
    """Capture A and B, report which handles shifted, by how much, from
    which symbol on - and attribute the shift to the declaration diff.

    The attribution diffs the SOURCE texts (and any shadow header pair)
    through the cost model; 'explained' is predicted == measured.  Equal-
    length capture dirs keep resolved paths comparable (il.capture's own
    discipline)."""
    from homm3.vc6 import il as _iltap
    base = workdir or (_iltap.IL_DIR / "delta")
    cap_a = capture_handles(src_a, flags, base / "a", shadow_a)
    cap_b = capture_handles(src_b, flags, base / "b", shadow_b)
    rep: dict = {
        "src_a": str(src_a), "src_b": str(src_b),
        "highwater": [cap_a.highwater, cap_b.highwater],
        "hw_delta": (None if None in (cap_a.highwater, cap_b.highwater)
                     else cap_b.highwater - cap_a.highwater),
    }
    names_a, names_b = cap_a.gl_names, cap_b.gl_names
    if [r["name"] for r in names_a] == [r["name"] for r in names_b]:
        shifts = [(ra["name"], ra["handle"], rb["handle"] - ra["handle"])
                  for ra, rb in zip(names_a, names_b)
                  if ra["handle"] != rb["handle"]]
        stable = [(ra["name"], ra["handle"])
                  for ra, rb in zip(names_a, names_b)
                  if ra["handle"] == rb["handle"]]
        rep["names_identical"] = True
        rep["n_shifted"] = len(shifts)
        rep["shift_deltas"] = sorted({d for _, _, d in shifts})
        rep["first_shifted"] = shifts[0] if shifts else None
        # localization: creation-point of the cause = between the highest
        # stable handle below the first shifted one and the first shifted
        if shifts:
            h0 = min(h for _, h, _ in shifts)
            below = [(n, h) for n, h in stable if h < h0]
            rep["last_stable_before"] = max(below, key=lambda t: t[1]) \
                if below else None
    else:
        rep["names_identical"] = False
        sa = {r["name"] for r in names_a}
        sb = {r["name"] for r in names_b}
        rep["added_names"] = sorted(sb - sa)[:8]
        rep["removed_names"] = sorted(sa - sb)[:8]

    # attribution: source diff + shadow diff through the cost model
    add_txt, rem_txt = _added_removed(src_a.read_text(), src_b.read_text())
    for name in sorted(set(shadow_a or {}) | set(shadow_b or {})):
        a_hdr = (shadow_a or {}).get(name, "")
        b_hdr = (shadow_b or {}).get(name, "")
        if a_hdr != b_hdr:
            ha, hr = _added_removed(a_hdr, b_hdr)
            add_txt += ("\n" + ha if ha else "")
            rem_txt += ("\n" + hr if hr else "")
    pred_add = predict_decl_cost(add_txt) if add_txt.strip() else \
        {"total": 0, "decls": [], "unpredicted": []}
    pred_rem = predict_decl_cost(rem_txt) if rem_txt.strip() else \
        {"total": 0, "decls": [], "unpredicted": []}
    predicted = (None if pred_add["total"] is None or pred_rem["total"] is None
                 else pred_add["total"] - pred_rem["total"])
    rep["attribution"] = {
        "added": [(d.text, d.cost) for d in pred_add["decls"]],
        "removed": [(d.text, d.cost) for d in pred_rem["decls"]],
        "unpredicted": pred_add["unpredicted"] + pred_rem["unpredicted"],
        "predicted_delta": predicted,
    }
    rep["explained"] = (predicted is not None and rep.get("hw_delta") is not None
                        and predicted == rep["hw_delta"])
    return rep


def format_delta_report(rep: dict) -> list[str]:
    out = []
    hw = rep["highwater"]
    out.append(f"handle high-water {hw[0]:#x} -> {hw[1]:#x} "
               f"({rep['hw_delta']:+d})" if None not in hw
               else "handle high-water unreadable")
    if rep.get("names_identical"):
        if rep.get("n_shifted"):
            n, h, d = rep["first_shifted"]
            out.append(f"{rep['n_shifted']} named handle(s) shifted "
                       f"({', '.join(f'{x:+d}' for x in rep['shift_deltas'])}), "
                       f"first: {n[:48]} @{h:#x}")
            if rep.get("last_stable_before"):
                sn, sh = rep["last_stable_before"]
                out.append(f"cause enters between {sn[:40]} @{sh:#x} (stable) "
                           f"and the first shifted symbol - that is its "
                           f"creation point")
        else:
            out.append("symbol names and handles identical")
    else:
        out.append(f"symbol name sets differ: +{rep.get('added_names')} "
                   f"-{rep.get('removed_names')}")
    att = rep["attribution"]
    for txt, cost in att["added"]:
        out.append(f"  added:   {txt[:56]:<58} predicted {cost:+d}"
                   if cost is not None else f"  added:   {txt[:56]} UNPREDICTED")
    for txt, cost in att["removed"]:
        out.append(f"  removed: {txt[:56]:<58} predicted {-cost:+d}"
                   if cost is not None else f"  removed: {txt[:56]} UNPREDICTED")
    if att["predicted_delta"] is not None:
        out.append(f"model prediction {att['predicted_delta']:+d} vs measured "
                   f"{rep['hw_delta']:+d} -> "
                   + ("EXPLAINED" if rep["explained"] else "NOT explained"))
    else:
        out.append("model prediction unavailable (unpredicted declaration "
                   "kind in the diff)")
    return out


# ---------------------------------------------------------------------------
# advise_handle_move - the why-reg v2 hook
# ---------------------------------------------------------------------------

def advise_handle_move(unit: str, fn: str, target_value: str) -> dict:
    """Given why-reg v2's 'value V must be created earlier' on a B1 swap:
    say which source change renumbers V's pseudo earlier, or prove none
    exists.  Pure source analysis + the measured order facts - no compile.

    Returns {'movable': bool|None, 'verdict': str, 'levers': [str],
    'evidence': [str]}.  Callers (why-reg --model) should invoke this
    behind try/except - a missing model must never break why-reg."""
    evidence = [
        "creation order within a function is parse-fixed: parameters in "
        "source order, then `this` (a real per-function sy symbol), then "
        "the block, then locals (measured: p@0xe0 < this@0xe2 < v@0xe5)",
        "file-scope declarations shift only LATER symbols (+cost each, "
        "additive); they never permute the relative order inside a function",
        "uniform handle shifts +1..+12 and +64/128/256/512 before "
        "ai_tactical get_simple_attack_effect leave the whole TU's "
        "instructions byte-identical (counters/line-info aside) - the "
        "binding is FLAT in the shift dimension at that locality",
    ]
    src_text, params = "", []
    try:
        from homm3.vc6 import _unit
        src = _unit.source_for_unit(unit)
        if src and src.is_file():
            src_text = src.read_text()
        try:
            from homm3.vc6.reg_model import _fn_params
            params = _fn_params(src_text, fn) if src_text else []
        except Exception:
            params = []
    except Exception:
        pass

    if target_value == "this":
        return {
            "movable": False,
            "verdict": (
                "`this` is minted between the parameters and the body "
                "locals - no declaration, include, or spelling change can "
                "create a local's handle before it, and uniform handle "
                "shifts are measured inert on the codegen; the divergence "
                "is C2-side handle STATE, not handle ORDER (catalog C1 "
                "class, not source-reachable)"),
            "levers": [],
            "evidence": evidence,
        }
    if params and target_value in params:
        return {
            "movable": False,
            "verdict": (
                f"'{target_value}' is a parameter: parameter handles follow "
                "the source parameter order, which the ABI fixes - "
                "reordering it would change the function's signature, not "
                "its allocation order"),
            "levers": [],
            "evidence": evidence,
        }
    # a local: its handle follows its textual creation point in the body
    return {
        "movable": True,
        "verdict": (
            f"'{target_value}' is a body local: its handle is minted at its "
            "textual declaration/first-binding point - moving that point "
            "earlier in the body renumbers its pseudo earlier (the B13/B14 "
            "naming levers work exactly this way)"),
        "levers": [
            f"move the declaration of '{target_value}' (or bind its "
            "expression to a named local) above the competing values' "
            "declarations",
            "if it competes with a parameter or `this`, no body spelling "
            "wins - that pair is the C1 handle-state class",
        ],
        "evidence": evidence,
    }
