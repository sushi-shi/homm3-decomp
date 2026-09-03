#!/usr/bin/env python3
"""homm3.vc6.inline_model - `predict-inline`: /Ob2 inline diagnosis + model.

v1 (the default, `run_predict`) is an inline-DIVERGENCE diagnoser (the
A-family analogue of why-reg's register diagnosis): for a caller function it
compares the multiset of out-of-line CALL targets in our in-unit build
against retail, and reports which callees diverge -

    retail CALLS X but we inline it        -> we OVER-inline (A12 / budget)
    we CALL X but retail inlines it         -> we UNDER-inline (A8/A9 depth)
    call COUNT differs for X                 -> partial expansion (A9 mid-fn
                                                budget exhaustion; A10 depth-2)

v2 (phase 3, `--predict`) adds the PREDICTIVE model: the /Ob2 accept/reject
rule reverse-engineered from the pinned C2.DLL's inline.c (see
docs/vc6/inliner.md for the address ledger and the validation record).
`predict()` is pure and compiler-free; `--selftest` replays the validated
oracle cases; `--spec FILE.json` predicts an arbitrary caller's per-site
expand/call decisions with the budget trajectory; `--measure-cb` titrates a
callee's front-end size estimate with the real compiler. The v1 CLI surface
is unchanged - the vc6 CLI never passes the new flags, so `homm3 vc6
predict-inline` still runs the diagnoser.

rc: 0 = call multisets agree (no inline divergence), 1 = they diverge, 2 = err.
Model modes: 0 = predicted/selftest-pass, 1 = selftest-fail, 2 = error.
"""
from __future__ import annotations

import json
import re
from collections import Counter
from pathlib import Path

from homm3.sema import _asm
from homm3.vc6 import _common, _unit, reg_model

# a REL32 reloc names a call / tail-jump target; DIR32 is data - exclude it.
_REL32 = re.compile(r"IMAGE_REL_I386_REL32\s+(\S+)")
# CRT/compiler helpers are not "inline candidates" - drop the noise.
_HELPER = re.compile(r"^(__|_?_ftol|_?_alloca|_?_chkstk|@__security)")


def _called(text: str) -> Counter:
    out = Counter()
    for m in _REL32.finditer(text):
        sym = m.group(1)
        if _HELPER.match(sym):
            continue
        out[sym] += 1
    return out


# THE NAME ARTIFACT THIS PAIRING EXISTS FOR (2026-08-19, measured: 77 of the
# 135 plateaus that reported inline divergence were this and nothing else).
# The multisets above are keyed by SYMBOL NAME, and the two sides do not name
# the same function the same way. Retail's side is the delinked target, whose
# callee names come from the synth PDB: a callee we have not claimed carries a
# working label (`sub_f6570`, `game_a7250_sub00_127f60`) and a CRT callee
# carries its runtime-map label (`exe_new` for `??2@YAPAXI@Z`). Our base side
# emits the real mangled symbol. Compared by name, ONE call then books twice -
# an under-inline on our side and an over-inline on retail's - and because
# `_route` puts the inliner upstream of everything, the function's true
# register or control-flow diagnosis never gets printed. monsters_sell_out is
# the worked example: 8 under / 8 over, 47 calls against 47, and the real
# residual an edi<->esi role swap.
#
# The rule below is deliberately conservative. A name is unresolvable only if
# our side never emits it AT ALL; a name both sides emit is resolvable, so a
# count difference on it stays a real divergence. Only min(base-only,
# ref-only-synth) calls pair off, so a genuine surplus on either side still
# reports.
# The test is SYNTACTIC, not a provenance lookup, because the synth-PDB
# provenance classes do not separate the two cases: `runtime-map` covers both
# `?_Tidy@?$basic_string@...` (a real mangled name, comparable) and `exe_new`
# (a label), and `src-VA` covers flat carcass names like `hero_load` for
# functions whose real declarator our side calls `?load@hero@@QAEHPAV...`.
# What actually separates them is that MSVC can only ever emit a symbol
# starting with `?` (C++), `_` (C / compiler-generated) or `@` (fastcall).
# A callee whose retail-side name begins with a letter is therefore a label
# the compiled side cannot produce, whatever the PDB calls it.
_EMITTABLE = re.compile(r"^[?_@]")


def _unresolvable(sym: str) -> bool:
    return not _EMITTABLE.match(sym)


def divergence(base_calls: Counter, ref_calls: Counter):
    """Real inline divergence: (under, over, name_paired).

    `name_paired` counts calls present on BOTH sides that could only be matched
    by count, because retail names the callee with a label the compiled side
    cannot emit - they are not divergence and are excluded from under/over.
    """
    keys = set(base_calls) | set(ref_calls)
    shared = [s for s in keys if base_calls[s] and ref_calls[s]]
    under = sum(max(0, base_calls[s] - ref_calls[s]) for s in shared)
    over = sum(max(0, ref_calls[s] - base_calls[s]) for s in shared)
    base_only = sum(base_calls[s] for s in keys if not ref_calls[s])
    ref_only_label = sum(ref_calls[s] for s in keys
                         if not base_calls[s] and _unresolvable(s))
    ref_only_named = sum(ref_calls[s] for s in keys
                         if not base_calls[s] and not _unresolvable(s))
    paired = min(base_only, ref_only_label)
    return (under + base_only - paired,
            over + ref_only_label - paired + ref_only_named,
            paired)


def divergence_note(base_calls: Counter, ref_calls: Counter):
    """'N under-inline, M over-inline' once unresolvable names pair off, else None."""
    under, over, paired = divergence(base_calls, ref_calls)
    if not under and not over:
        return None
    note = ", ".join([f"{under} under-inline"] * bool(under)
                     + [f"{over} over-inline"] * bool(over))
    return note + (f" ({paired} name-unresolvable pair(s) discounted)"
                   if paired else "")


def effective_divergence(base_calls: Counter, ref_calls: Counter,
                         *, byte_exact: bool = False):
    """Divergence after the strongest available verdict.

    A current normalized 100% byte match outranks the two COFF relocation-name
    inventories.  The delinked target can legitimately have fewer named REL32
    records than the compiled object even though the normalized instructions
    and relocation semantics are exact.  In that case the residue is tooling
    bookkeeping, not an inline decision and certainly not a source fix.
    """
    under, over, paired = divergence(base_calls, ref_calls)
    return (0, 0, paired) if byte_exact else (under, over, paired)


def divergence_rows(base_calls: Counter, ref_calls: Counter):
    """Named rows behind :func:`divergence`, with synth pairing allocated.

    A base-only mangled symbol is a real under-inline unless one of retail's
    base-impossible synthesized labels can consume that call as a name-only
    pair.  The old CLI put *every* base-only symbol in ``name_unresolved``;
    that hid unambiguous cases such as two candidate ``GetArmyName`` calls
    against no retail-only synthesized labels at all.

    Return ``(over, under, paired_rows, unknown_over, paired_count)``.  Rows
    use the existing ``(symbol, base_count, retail_count)`` shape.  Pair
    allocation is deliberately deterministic but carries no semantic claim:
    paired symbols remain unknown, while any unconsumed count is actionable.
    """
    keys = set(base_calls) | set(ref_calls)
    over, under = [], []
    base_only, ref_only_labels = [], []

    for sym in sorted(keys):
        b, r = base_calls[sym], ref_calls[sym]
        if b == r:
            continue
        if b and r:
            (under if b > r else over).append((sym, b, r))
        elif b:
            base_only.append((sym, b))
        elif _unresolvable(sym):
            ref_only_labels.append((sym, r))
        else:
            over.append((sym, b, r))

    pair_budget = min(sum(n for _s, n in base_only),
                      sum(n for _s, n in ref_only_labels))
    paired_count = pair_budget
    paired_rows = []

    remaining_pairs = pair_budget
    for sym, count in base_only:
        paired = min(count, remaining_pairs)
        if paired:
            paired_rows.append((sym, paired, 0))
            remaining_pairs -= paired
        if count > paired:
            under.append((sym, count - paired, 0))

    unknown_over = []
    remaining_pairs = pair_budget
    for sym, count in ref_only_labels:
        paired = min(count, remaining_pairs)
        if paired:
            paired_rows.append((sym, 0, paired))
            remaining_pairs -= paired
        if count > paired:
            unknown_over.append((sym, 0, count - paired))

    return over, under, paired_rows, unknown_over, paired_count


def partition_external_count_rows(rows, defined_text):
    """Separate count deltas that cannot be inline decisions.

    If both sides call the same symbol, but that symbol has no code body in
    the candidate TU, C1 cannot expand it.  A differing count then comes from
    duplicated/merged call sites or revision semantics, not the inliner.
    Candidate-only and retail-only rows remain conservative because a missing
    visible body may itself be the declaration/include defect under study.
    """
    inline_rows, count_rows = [], []
    for row in rows:
        sym, base_n, retail_n = row
        if base_n and retail_n and sym not in defined_text:
            count_rows.append(row)
        else:
            inline_rows.append(row)
    return inline_rows, count_rows


def nested_frontiers(under_rows, over_rows, callee_calls):
    """Find reciprocal outer-call/nested-call inline boundaries.

    ``under_rows`` are callees our caller keeps more often than retail;
    ``over_rows`` are callees retail keeps more often than ours.  If the
    candidate's standalone outer body calls the latter, retail's source shape
    is the useful midpoint: expand the outer operation but stop at the nested
    helper.  Return ``(outer, inner, differing_sites)`` rows.
    """
    out = []
    for outer, base_n, retail_n in under_rows:
        outer_calls = callee_calls.get(outer, Counter())
        outer_delta = max(0, base_n - retail_n)
        for inner, inner_base_n, inner_retail_n in over_rows:
            inner_delta = max(0, inner_retail_n - inner_base_n)
            if outer_calls[inner]:
                out.append((outer, inner,
                            min(outer_delta, inner_delta,
                                outer_calls[inner])))
    return tuple(row for row in out if row[2])


def _callee_call_map(obj: Path, rows) -> dict[str, Counter]:
    """Out-of-line call maps for candidate callees present in this object."""
    public = _asm._public_text_symbols(obj)
    out = {}
    for symbol, _base_n, _retail_n in rows:
        if symbol in public:
            out[symbol] = _called(_asm.objdump(obj, symbol, 0))
    return out


def _current_report_score(unit: str | None, symbol: str,
                          base_obj: Path) -> float | None:
    """Fresh objdiff score for the exact build object being diagnosed."""
    if not unit:
        return None
    report = _common.REPO / "build/objdiff/report.json"
    try:
        report_mtime = report.stat().st_mtime_ns
        target_obj = _asm.TARGET / f"{unit}.c.obj"
        newest_input = max(
            base_obj.stat().st_mtime_ns,
            target_obj.stat().st_mtime_ns if target_obj.is_file() else 0)
        if report_mtime < newest_input:
            return None
        data = json.loads(report.read_text())
    except (OSError, ValueError, TypeError):
        return None
    for unit_row in data.get("units", []):
        name = (unit_row.get("name") or unit_row.get("id") or "").split("/")[-1]
        if name != unit:
            continue
        for function in unit_row.get("functions", []) or []:
            if function.get("name") == symbol:
                try:
                    return float(function.get("fuzzy_match_percent"))
                except (TypeError, ValueError):
                    return None
    return None


def _default_against(src: Path, fn: str) -> tuple[str, str] | None:
    """Infer ``(unit, UNIT:FN)`` for a manifest-owned source file.

    ``predict-inline`` is part of the ordinary matching loop, so requiring a
    caller to repeat the unit already named by ``config/units.toml`` is both
    noisy and error-prone.  An explicit ``--against`` still wins; sources
    outside the manifest remain ambiguous and must name their reference.
    """
    unit = _unit.unit_for_source(src)
    if not unit:
        return None
    return unit, f"{unit}:{fn}"


def run_predict(args) -> int:
    src = Path(args.src).resolve()
    if not src.is_file():
        _common.die(f"source missing: {src}")
    if not getattr(args, "against", None) \
            and not getattr(args, "against_src", None):
        inferred = _default_against(src, args.fn)
        if inferred is None:
            _common.die(
                "no implicit retail reference: source is not a units.toml "
                "unit; pass --against UNIT:FN or --against-src FILE")
        unit, args.against = inferred
    else:
        unit = reg_model._resolve_unit(args) \
            if getattr(args, "against", None) else None
    # base: the in-unit obj the ratchet scored (or a faithful compile)
    if unit and _unit.base_obj(unit):
        base_obj = _unit.base_obj(unit)
    elif unit:
        base_obj, tail = _unit.compile_text(src.read_text(), unit,
                                            reg_model.SCRATCH / "base", "base")
        if base_obj is None:
            _common.die(f"base failed to compile:\n{tail}")
    else:
        base_obj, tail = reg_model._compile_tu(
            src, reg_model.SCRATCH / "base", reg_model._wine_dir(src.parent))
        if base_obj is None:
            _common.die(f"base failed to compile:\n{tail}")

    base_text, base_sym = reg_model._fn_text(base_obj, args.fn)
    ref_text, ref_label = reg_model._reference_side(args)

    base_calls, ref_calls = _called(base_text), _called(ref_text)
    over, under, unresolved, unknown_over, _row_paired = divergence_rows(
        base_calls, ref_calls)
    defined_text = _asm._text_symbols(base_obj)
    over, over_call_counts = partition_external_count_rows(over, defined_text)
    under, under_call_counts = partition_external_count_rows(under,
                                                               defined_text)
    call_counts = over_call_counts + under_call_counts

    report_score = _current_report_score(unit, base_sym, base_obj)
    byte_exact = report_score is not None and report_score >= 99.9999
    raw_under, raw_over, paired = divergence(base_calls, ref_calls)
    real_under, real_over, _ = effective_divergence(
        base_calls, ref_calls, byte_exact=byte_exact)
    diverged = bool(real_under or real_over)
    rc = 1 if diverged else 0

    nested = ()
    if diverged and under and over:
        nested = nested_frontiers(
            under, over, _callee_call_map(base_obj, under))

    if getattr(args, "json", False):
        print(json.dumps({
            "fn": args.fn, "symbol": base_sym, "reference": ref_label,
            "over_inline": [{"callee": s, "base_calls": b, "retail_calls": r}
                            for s, b, r in over],
            "under_inline": [{"callee": s, "base_calls": b, "retail_calls": r}
                             for s, b, r in under],
            "name_unresolved": [{"callee": s, "base_calls": b,
                                 "retail_calls": r}
                                for s, b, r in unresolved],
            "over_inline_unresolved": [
                {"callee": s, "base_calls": b, "retail_calls": r}
                for s, b, r in unknown_over],
            "noninline_call_count": [
                {"callee": s, "base_calls": b, "retail_calls": r}
                for s, b, r in call_counts],
            "name_paired": paired,
            "raw_under": raw_under, "raw_over": raw_over,
            "under": real_under, "over": real_over,
            "byte_exact": byte_exact,
            "report_score": report_score,
            "nested_frontiers": [
                {"outer": outer, "inner": inner, "sites": sites}
                for outer, inner, sites in nested],
            "rc": rc}, indent=2))
        return rc

    print(f"[predict-inline] {args.fn} ({base_sym})")
    print(f"[reference] {ref_label}")
    print(f"[calls] base emits {sum(base_calls.values())} out-of-line call(s); "
          f"retail {sum(ref_calls.values())}")
    if unresolved:
        print(f"[names] {paired} call(s) pair off by COUNT and are NOT "
              "divergence - retail names an unclaimed callee with a synth "
              "label our side can never emit:")
        for s, b, r in sorted(unresolved, key=lambda x: -(x[1] + x[2])):
            side = f"base x{b}" if b else f"retail x{r}"
            print(f"    {s[:70]}  {side}")
    if byte_exact:
        print(f"[inline] current normalized byte score is {report_score:.4f}% "
              "- inline structure is exact. Any remaining REL32 name/count "
              "residue is delinker/name-map bookkeeping; do not change "
              "source for it.")
        return 0
    if not diverged:
        print("[inline] call multisets AGREE - inline structure matches; any "
              "residual is register/scheduling (-> why-reg).")
        return 0
    if under:
        print("[UNDER-inline] retail expands these; our CL keeps a call "
              "(A8/A9 depth/budget - our budget ran out or went too deep):")
        for s, b, r in sorted(under, key=lambda x: -(x[1] - x[2])):
            print(f"    {s[:70]}  base x{b} vs retail x{r}")
    if over:
        print("[OVER-inline] our CL expands these; retail keeps a call "
              "(A12 single-call-site / callee cheaper than retail's):")
        for s, b, r in sorted(over, key=lambda x: -(x[2] - x[1])):
            print(f"    {s[:70]}  base x{b} vs retail x{r}")
    if unknown_over:
        print("[OVER-inline, name unresolved] retail retains additional "
              "call(s) under synthesized labels; inspect them positionally "
              "before choosing a source site:")
        for s, b, r in sorted(unknown_over,
                              key=lambda x: -(x[2] - x[1])):
            print(f"    {s[:70]}  retail x{r}")
    if call_counts:
        print("[CALL-count, not inlining] both sides call these external "
              "symbols, and this TU defines no body C1 could expand. The "
              "difference is duplicated/merged sites or revision semantics:")
        for s, b, r in sorted(call_counts,
                              key=lambda x: -abs(x[2] - x[1])):
            print(f"    {s[:70]}  base x{b} vs retail x{r}")
        print("[fix] inspect the ordered call stream and source-labelled CFG "
              "for branch-arm duplication, a cross-jumped common tail, or a "
              "missing guarded call; do not apply an inline pragma.")
    if not under and not over and not unknown_over and not call_counts:
        print("[unknown] unmatched synthesized callee names leave a call-count "
              "residue, but no named inline decision can be correlated. This "
              "is not actionable evidence for changing C++.")
        print("[fix] run `homm3 sema diff <selector> --structure`; if flow and "
              "sizes agree, repair/admit the relocation or callee-name mapping "
              "instead of steering C1.")
        return rc
    if nested:
        print("[shape] reciprocal nested inline frontier: retail expands the "
              "outer operation at the differing site but retains its nested "
              "helper:")
        for outer, inner, sites in nested:
            print(f"    {outer[:56]} -> {inner[:56]}  site(s) x{sites}")
        print("[fix] inspect the Dreamcast line/scope rows immediately before "
              "that source or compiler-generated cleanup. Restore a missing "
              "real guard, helper, RAII boundary, or release-elided operation "
              "first; such a zero-byte source fact can move C1 to the exact "
              "outer-inline/inner-call midpoint.")
        print("[probe] `inline_depth(1)` over the cleanup region may confirm "
              "the nesting diagnosis, but an overshoot is a negative control, "
              "not permission for a global pragma, an exposed library "
              "internal, or synthetic caller mass.")
    print("[fix] first run `homm3 dreamcast show/asm --blocks` and `homm3 "
          "sema diff --structure/--source`; restore the evidenced helper, "
          "type, lifetime, and statement order before steering C1.")
    if over or unknown_over:
        print("[fix] over-inline at a DIRECT source call -> put only that "
              "statement between `#pragma inline_depth(0)` / "
              "`#pragma inline_depth()` and mark it `INLINE_GATE(...)`; "
              "retain it only with a flattening negative control. Use "
              "`auto_inline(off)` only when every affected call site proves "
              "the body unavailable.")
        print("[fix] over-inline at a NESTED callee absent from source (for "
              "example basic_string::_Tidy) -> do not expose library "
              "internals or add synthetic mass. Probe depth 1; if it is flat "
              "and depth 0 suppresses the required outer inline, keep the "
              "natural source and classify the compiler-state wall.")
    if under:
        print("[fix] under-inline -> restore the smallest Dreamcast-proven "
              "callee body/declaration and remeasure its callers; do not "
              "erase the helper boundary merely to improve one percentage. "
              "why-reg cannot reach these inliner decisions.")
    return rc


# ===========================================================================
# Phase 3: the predictive /Ob2 model - RE'd from C2.DLL 12.00.8447 inline.c.
# Every constant below is byte-proven at the cited rva (image base
# 0x10700000); docs/vc6/inliner.md is the ledger + validation record.
# ===========================================================================

BUDGET_FLOOR = 1000    # 0x19970: budget = max(2*caller_cb, 1000)
BUDGET_CAP = 35000     # 0x199da -> cold stub 0x93d28: mov eax,0x88b8
RUNNING_CAP = 35000    # 0x19f9f: cmp [0x1079f234],0x88b8; jg reject
SMALL_FREE = 0x28      # 0x19a8a/0x19fda: cb <= 40 skips the budget entirely
CANDIDACY_CB = 1000    # DAT_10799280, set to 0x3e8 by main.c at 0x7d6a7
DEPTH_DEFAULT = 8      # #pragma inline_depth default (site allowance byte;
                       # the 0x1b8 state tuple, collector 0x1a27c)


def _i16(v: int) -> int:
    """The IL size estimate is a SIGNED 16-bit field (movsx at 0x1995e /
    0x19f8c) - huge front-end estimates wrap negative (measured: the E3
    pad-4500 caller reverts to the default-budget answer)."""
    v &= 0xFFFF
    return v - 0x10000 if v >= 0x8000 else v


def _tdiv(a: int, b: int) -> int:
    """cdq+idiv truncates toward zero (0x1a0d0), unlike Python's floor."""
    q = abs(a) // abs(b)
    return -q if (a < 0) != (b < 0) else q


class Callee:
    """A callee as C2 sees it: the front-end IL symbol fields that feed the
    decision (sym+0x6d cb, sym+0x73 flag bits), plus its stored body's own
    candidate call sites (re-decided at every expansion - the stored body is
    the FRONT-END tuple stream, measured in E5)."""

    def __init__(self, name, cb, sites=(), forceinline=False, marked=False,
                 candidate=True):
        self.name = name
        self.cb = cb                    # C1XX size estimate (IL sym+0x6d)
        self.sites = list(sites)        # candidate Sites inside the body
        self.forceinline = forceinline  # flags & 0x2000
        self.marked = marked            # flags & 0x80 (inline-declared)
        self.candidate = candidate      # flags & 0x40 body-saved AND
        #                                 cb < 1000 (collector 0x1a27c);
        #                                 False = never queued (all sites call)


class Site:
    def __init__(self, callee, depth_allow=DEPTH_DEFAULT):
        self.callee = callee
        self.depth_allow = depth_allow  # inline_depth state at the site
        #                                 (low byte of the 0x1b8 tuple value)


def _expand(sites, depth, budget, state, out):
    """The sequential accept loop of 0x199fa. Returns (spent, budget)."""
    budget0 = budget
    n = len(sites)
    for k, site in enumerate(sites):
        nrem = n - k                       # counter at [E+0x20], dec 0x19eb7
        c = site.callee
        cb = _i16(c.cb)
        node = {"name": c.name, "cb": cb, "depth": depth,
                "budget_before": budget, "nested": []}
        out.append(node)
        if not c.candidate:
            node.update(action="call", reason="not-a-candidate "
                        "(front-end body-save gate / cb>=1000)")
            continue
        if depth > site.depth_allow:       # 0x19f7c
            node.update(action="call", reason="depth")
            continue
        if not c.forceinline:
            if budget < cb and cb > SMALL_FREE:   # 0x19f97 + 0x19a8a
                node.update(action="call", reason="budget")
                continue
            if state["running"] > RUNNING_CAP:    # 0x19f9f
                node.update(action="call", reason="running-cap")
                continue
        # accept
        if not c.forceinline:
            if cb > SMALL_FREE:
                budget -= cb               # 0x19bac
            state["running"] += cb         # 0x19fec
        sub_budget = _tdiv(budget, nrem)   # 0x1a0cc-0x1a0dd: depth+1,
        spent, _ = _expand(c.sites, depth + 1, sub_budget, state,
                           node["nested"])
        if not c.forceinline:
            budget -= spent                # 0x1a0f9-0x1a10a
            state["running"] += spent
        node.update(action="expand", reason="")
    return budget0 - budget, budget


def predict(caller_cb, sites):
    """Pure model of one function's /Ob2 inline pass (entry 0x1994f).

    caller_cb: the CALLER's own front-end size estimate (its IL sym+0x6d).
    sites: the caller's candidate call Sites in tuple-stream order (both
    branch arms - the collector walks the stream linearly).
    Returns {budget, running0, decisions, spent}."""
    cb = _i16(caller_cb)
    budget = 2 * cb
    if budget < BUDGET_FLOOR:
        budget = BUDGET_FLOOR
    elif budget > BUDGET_CAP:
        budget = BUDGET_CAP                # 0x93d28 clamp (NOT a bail)
    state = {"running": cb}                # DAT_1079f234 := caller_cb
    decisions: list = []
    spent, left = _expand(sites, 1, budget, state, decisions)
    return {"caller_cb": cb, "budget": budget, "spent": spent,
            "budget_left": left, "running_final": state["running"],
            "decisions": decisions}


# cb per simple statement, measured 2026-08-10: an 11-statement leaf titrates to
# cb in (142,166] (callerB budget-edge), i.e. ~13-15 cb/stmt; the front-end
# body-save gate sits at ~13 statements. Approximate - use for ballpark gaps.
CB_PER_STMT = 14


def budget_gap(caller_cb, sites):
    """For each site the budget STARVED (rejected, reason='budget'), how much
    more caller body would expand it. budget = 2*caller_cb, so raising the
    caller's cb by ceil(deficit/2) lifts the site's budget by `deficit`; that
    is ~deficit/(2*CB_PER_STMT) statements added to THE CALLER (the 'finish the
    caller' lever, quantified). The FIRST budget-starved site is the cleanest
    target (sequential spending makes later ones move together)."""
    rep = predict(caller_cb, sites)
    gaps = []
    for _depth, d in _flatten(rep["decisions"]):
        if d["action"] == "call" and d["reason"] == "budget":
            deficit = d["cb"] - d["budget_before"]
            gaps.append({
                "callee": d["name"], "cb": d["cb"],
                "budget_at_site": d["budget_before"],
                "deficit_cb": deficit,
                "grow_caller_cb_by": (deficit + 1) // 2,
                "approx_caller_statements": max(
                    1, round((deficit / 2) / CB_PER_STMT))})
    return {"budget": rep["budget"], "at_floor": rep["budget"] == BUDGET_FLOOR,
            "spent": rep["spent"], "starved_sites": gaps, "report": rep}


def _flatten(decisions, depth=0):
    for d in decisions:
        yield depth, d
        yield from _flatten(d["nested"], depth + 1)


def _print_report(rep) -> None:
    print(f"[model] caller_cb={rep['caller_cb']}  budget=2*cb clamped to "
          f"[{BUDGET_FLOOR},{BUDGET_CAP}] = {rep['budget']}  "
          f"spent={rep['spent']}  left={rep['budget_left']}  "
          f"running(final)={rep['running_final']}")
    for depth, d in _flatten(rep["decisions"]):
        mark = "EXPAND" if d["action"] == "expand" else "call  "
        why = f"  ({d['reason']})" if d["reason"] else ""
        print(f"  {'  ' * depth}{mark} {d['name']}  cb={d['cb']} "
              f"budget@site={d['budget_before']}{why}")


def _counts(rep, name):
    ex = sum(1 for _, d in _flatten(rep["decisions"])
             if d["name"] == name and d["action"] == "expand")
    ca = sum(1 for _, d in _flatten(rep["decisions"])
             if d["name"] == name and d["action"] == "call")
    return ex, ca


# ---------------------------------------------------------------------------
# --measure-cb: titrate a callee's cb with the real compiler.  The harness TU
# must contain the callee and a caller with N same-callee sites; with a small
# caller the budget is the 1000 floor, so
#     expanded = floor(1000 / cb)   (cb > 40; 0 expanded = not a candidate)
# and counting REJECTED sites (call + tail-jmp!) brackets cb.
# ---------------------------------------------------------------------------

_PROFILE = ["/O2", "/Ob2", "/Oy-", "/Op", "/ML", "/Gr", "/GX", "/GR-",
            "/D_WINDOWS"]
_MODEL_SCRATCH = _common.REPO / "build/vc6/inline-model"


def measure_cb(src: Path, callee: str, caller: str, n_sites: int):
    """Compile the harness TU; return (expanded, rejected, lo, hi) where
    cb in [lo, hi] (lo=None => cb <= 40; hi=None => not a candidate)."""
    import subprocess
    import sys as _sys
    _MODEL_SCRATCH.mkdir(parents=True, exist_ok=True)
    out = _MODEL_SCRATCH / (src.stem + ".obj")
    cmd = [_sys.executable, "-m", "homm3.core.cc_wrap",
           "--out", str(out), "--src", str(src),
           "--", "/c", *_PROFILE, "/FAs"]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    asm = _MODEL_SCRATCH / (src.stem + ".asm")
    if proc.returncode != 0 or not asm.is_file():
        _common.die("measure-cb compile failed:\n" +
                    "\n".join((proc.stdout + proc.stderr).splitlines()[-8:]))
    body, inside = [], False
    for line in asm.read_text(errors="replace").splitlines():
        if not inside and caller in line and " PROC" in line:
            inside = True
        elif inside:
            if " ENDP" in line:
                break
            body.append(line)
    text = "\n".join(body)
    rejected = len(re.findall(r"\b(?:call|jmp)\s+[^\n;]*" + re.escape(callee),
                              text))
    expanded = n_sites - rejected
    if expanded == n_sites:
        lo, hi = None, SMALL_FREE          # never rejected: cb <= 40 (or
        #                                    budget never binding - use more
        #                                    sites)
    elif expanded == 0:
        lo, hi = CANDIDACY_CB, None        # candidacy cliff (or cb > budget)
    else:
        lo = BUDGET_FLOOR // (expanded + 1) + 1
        hi = BUDGET_FLOOR // expanded
    return expanded, rejected, lo, hi


# ---------------------------------------------------------------------------
# --selftest: the validation record.  Every case is a MEASURED pinned-compiler
# behaviour (docs/vc6/inliner.md section 5); the model must reproduce each
# across the full measured cb bracket, not at one lucky value.
# ---------------------------------------------------------------------------

def _selftest() -> int:
    failures = []

    def check(label, ok, detail=""):
        print(f"[selftest] {'PASS' if ok else 'FAIL'}  {label}"
              + (f"  {detail}" if detail else ""))
        if not ok:
            failures.append(label)

    # 1. a06 fill_plain: 9 sites, small caller -> 6 expand + 3 reject
    #    (2 call + 1 tail-jmp), for every cb in the measured [143,166].
    ok = all(_counts(predict(120, [Site(Callee("fill", cb))] * 9), "fill")
             == (6, 3) for cb in range(143, 167))
    check("a06 fill_plain 6+3 across cb=[143,166]", ok)

    # 2. a06 fill_storedec: the dead pre-loop store crosses the FRONT-END
    #    body-save gate -> not a candidate -> 0 expand + 9 reject.
    rep = predict(120, [Site(Callee("filsd", 170, candidate=False))] * 9)
    check("a06 fill_storedec candidacy drop 0+9",
          _counts(rep, "filsd") == (0, 9))

    # 3. E2: cb <= 40 is budget-exempt -> 60/60 expand.
    rep = predict(60, [Site(Callee("tiny", 20))] * 60)
    check("E2 small-free 60/60", _counts(rep, "tiny") == (60, 0))

    # 4. E5 nested (the do_general_melee shadow): ff -> 6x gg -> 3x hh.
    #    Budget 1000 divides by remaining sites at the nested level: exactly
    #    ONE hh per gg copy (6 expand + 12 reject), all six gg expand.
    #    Must hold across cb(hh)=[143,166] and cb(gg)<=40.
    ok = True
    for cbh in range(143, 167):
        gg = Callee("gg", 30, sites=[Site(Callee("hh", cbh))] * 3)
        rep = predict(60, [Site(gg)] * 6)
        ok &= _counts(rep, "gg") == (6, 0) and _counts(rep, "hh") == (6, 12)
    check("E5 nested 6/0 gg + 6/12 hh across cb(hh)=[143,166]", ok)

    # 5. E3 caller-size coupling: padding the CALLER (cb 930) lifts the
    #    budget to 2*cb=1860 >= 9*cb(fill) -> all 9 expand.
    rep = predict(930, [Site(Callee("fill", 150))] * 9)
    check("E3 pad flip 9/0 at caller_cb=930", _counts(rep, "fill") == (9, 0))

    # 6. E3 wrap: caller_cb > 32767 wraps the signed 16-bit field ->
    #    budget floor 1000 -> back to the pad=0 answer (6 expand).
    rep = predict(40000, [Site(Callee("fill", 150))] * 9)
    check("E3 int16 wrap reverts to floor budget 6/3",
          _counts(rep, "fill") == (6, 3))

    # 7. E6 STL flip shape (measured: 12 strings, 24 _Tidy sites; pad=0 ->
    #    23 _Tidy calls, pad=300 -> 0): 12 ctor trees charge the budget up
    #    front, the divided budget starves every nested _Tidy EXCEPT the
    #    last site (nrem=1 lifts the division) - 1 expand + 23 call; the
    #    padded caller (budget 2*cb) expands all 24.
    ctor = lambda: Callee("ctor", 60, sites=[Site(Callee("_Tidy", 85))])
    dtor = lambda: Callee("dtor", 20, sites=[Site(Callee("_Tidy", 85))])
    shape = lambda: [Site(ctor()) for _ in range(12)] + \
                    [Site(dtor()) for _ in range(12)]
    small = predict(80, shape())
    big = predict(3500, shape())
    s_ex, s_ca = _counts(small, "_Tidy")
    b_ex, b_ca = _counts(big, "_Tidy")
    check("E6 STL flip: small caller starves _Tidy, padded caller 24/0",
          s_ca >= 20 and (b_ex, b_ca) == (24, 0),
          f"small={s_ex}/{s_ca} big={b_ex}/{b_ca}")

    # 8. do_general_melee (A9): candidate sites in tuple order are
    #    kill, inflict_damage, kill, inflict_damage (get_final_melee_value is
    #    not a candidate).  Measured cb: get_total [46,47], kill [77,83].
    #    OUR side: get_total is a CALL in kill copy 2 <=> the budget left at
    #    that site is < 172 (charge 80, then floor(budget/2) < 46) while
    #    kill#2 itself is still accepted (budget >= 80): the inflict_damage#1
    #    subtree spends in (708, 787] of the 1000 floor (the intersection
    #    across the measured cb brackets).  That spend window
    #    is DERIVED from the observed decision (the model run backwards);
    #    within it the model reproduces expand@copy1 + call@copy2 for every
    #    measured cb combination, and a larger caller estimate (retail's
    #    side of the knife-edge) expands both.
    ok = True
    flip_ok = True
    for cbgt in (46, 47):
        for cbk in range(77, 84):
            for id_spend in range(710, 787, 25):
                kill = lambda: Callee("kill", cbk,
                                      sites=[Site(Callee("get_total", cbgt))])
                idmg = Callee("inflict_damage", id_spend, candidate=True)
                sites = [Site(kill()), Site(idmg), Site(kill()), Site(idmg)]
                rep = predict(235, sites)
                gt_ex, gt_ca = _counts(rep, "get_total")
                ok &= (gt_ex, gt_ca) == (1, 1)
                # retail's side: a bigger caller estimate lifts the budget
                # and the SAME tree expands get_total in both copies.
                rep2 = predict(1400, sites)
                flip_ok &= _counts(rep2, "get_total") == (2, 0)
    check("A9 do_general_melee: get_total expand@kill#1, call@kill#2 (ours)",
          ok)
    check("A9 do_general_melee: larger caller_cb expands both (retail shape)",
          flip_ok)

    if failures:
        print(f"[selftest] {len(failures)} FAILURE(S): " + ", ".join(failures))
    else:
        print("[selftest] ALL PASS")
    return 1 if failures else 0


def _load_spec(path: Path):
    spec = json.loads(path.read_text())

    def mk_site(s):
        c = s.get("callee", s)
        callee = Callee(c.get("name", "?"), c["cb"],
                        sites=[mk_site(x) for x in c.get("sites", [])],
                        forceinline=c.get("forceinline", False),
                        marked=c.get("marked", False),
                        candidate=c.get("candidate", True))
        return Site(callee, depth_allow=s.get("depth_allow", DEPTH_DEFAULT))

    return spec["caller_cb"], [mk_site(s) for s in spec["sites"]]


def run_model(args) -> int:
    if getattr(args, "selftest", False):
        return _selftest()
    if getattr(args, "measure_cb", None):
        src = Path(args.measure_cb).resolve()
        if not src.is_file():
            _common.die(f"harness TU missing: {src}")
        if not (args.fn and getattr(args, "caller", None)
                and getattr(args, "sites", None)):
            _common.die("--measure-cb needs --fn CALLEE --caller CALLER "
                        "--sites N")
        ex, rej, lo, hi = measure_cb(src, args.fn, args.caller, args.sites)
        if lo is None:
            verdict = f"cb <= {SMALL_FREE} (never rejected; budget-exempt)"
        elif hi is None:
            verdict = ("NOT an inline candidate (front-end body-save gate "
                       f"or cb >= {CANDIDACY_CB})")
        else:
            verdict = f"cb in [{lo},{hi}]"
        print(f"[measure-cb] {args.fn}: {ex} expanded, {rej} rejected "
              f"(call+jmp) of {args.sites} -> {verdict}")
        return 0
    if getattr(args, "gap", None):
        caller_cb, sites = _load_spec(Path(args.gap))
        g = budget_gap(caller_cb, sites)
        if getattr(args, "json", False):
            print(json.dumps({k: v for k, v in g.items() if k != "report"},
                             indent=2))
            return 0
        floor = "  (AT THE 1000 FLOOR - caller is small/starved)" \
            if g["at_floor"] else ""
        print(f"[budget-gap] budget={g['budget']}{floor}  spent={g['spent']}")
        if not g["starved_sites"]:
            print("  no budget-starved sites - inline structure is not "
                  "budget-limited here.")
        for s in g["starved_sites"]:
            print(f"  {s['callee'][:56]} cb={s['cb']}: budget {s['budget_at_site']}"
                  f" at site, short {s['deficit_cb']} -> grow the CALLER by "
                  f"~{s['approx_caller_statements']} statement(s) "
                  f"(+{s['grow_caller_cb_by']} caller_cb)")
        return 0
    if getattr(args, "spec", None):
        caller_cb, sites = _load_spec(Path(args.spec))
        rep = predict(caller_cb, sites)
        if getattr(args, "json", False):
            print(json.dumps(rep, indent=2))
        else:
            _print_report(rep)
        return 0
    _common.die("--predict needs --selftest, --spec/--gap FILE or "
                "--measure-cb TU")


if __name__ == "__main__":
    import argparse
    import sys
    ap = argparse.ArgumentParser(prog="python3 -m homm3.vc6.inline_model")
    ap.add_argument("src", nargs="?", help="diagnoser: TU to compile; "
                    "model: unused")
    ap.add_argument("--fn", help="diagnoser: caller; measure-cb: callee")
    ap.add_argument("--against")
    ap.add_argument("--against-src", dest="against_src")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--predict", action="store_true",
                    help="phase-3 model modes (see below) instead of the "
                    "diagnoser")
    ap.add_argument("--selftest", action="store_true",
                    help="replay the validated oracle cases through predict()")
    ap.add_argument("--spec", help="JSON caller/sites spec to predict")
    ap.add_argument("--gap", help="JSON caller/sites spec: report the "
                    "budget deficit per starved site as CALLER statements")
    ap.add_argument("--measure-cb", dest="measure_cb", metavar="TU",
                    help="titrate a callee's cb: compile TU, count rejected "
                    "sites in --caller")
    ap.add_argument("--caller", help="measure-cb: harness caller function")
    ap.add_argument("--sites", type=int, help="measure-cb: site count in the "
                    "harness caller")
    _args = ap.parse_args()
    if (_args.predict or _args.selftest or _args.spec or _args.gap
            or _args.measure_cb):
        sys.exit(run_model(_args))
    if not (_args.src and _args.fn):
        ap.error("diagnoser mode needs src and --fn")
    sys.exit(run_predict(_args))
