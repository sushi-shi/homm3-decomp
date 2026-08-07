#!/usr/bin/env python3
"""homm3.analysis.h2_twins - propose renamed homm2->homm3 function twins.

The exact-name join (homm3.analysis.homm2_overlap, functions lane) only
sees functions HoMM3 kept the name of. A renamed survivor -
advManager::SystemOptions -> DoSystemOptions, combatManager::
GetNextArmy -> NextArmy, advManager::ProcessMapChange ->
ProcessMapChangeNew - is invisible to it, and its exact VC6 template
goes unused. This lane scores every unpaired homm2 function against
the Dreamcast functions of ITS OWN class and proposes the likely twin.

The score is three auditable terms, S = 0.6*name + 0.2*arity +
0.2*callee, each emitted raw in the row:
  name    Jaccard over CamelCase token sets from the case-preserved
          method components (the lowercased join key destroys the token
          boundaries; substring matching is refused - it proposed
          army::DispelGood ~ army::Is).
  arity   delta = dc_params - (h2_arity + 1); DC CodeView `params`
          counts `this` (calibrated on the exact-name pairs: delta=0
          for ~3/4 of them). NEVER a veto - HoMM3 extends signatures
          (the exact-name pair armyGroup::GetMorale sits at delta=+3).
  callee  Jaccard of normalized callee-name sets restricted to the
          shared cross-game vocabulary; the DC xref graph carries only
          ~3k direct-call edges, so this corroborates and never gates.

Soundness rules:
  - candidates come only from the same (case-folded) class; free
    functions are refused BY CONSTRUCTION - which is what keeps the
    documented bzip compress/uncompress vs zlib collision out;
  - every unpaired homm2 function emits exactly one row (the exact-name
    lane silently drops its non-joiners; this file IS that residue);
  - one DC row proposed by two homm2 methods demotes both to `refused
    collision` unless one leads by the margin (a naming map is an
    injection - the same law dc_callgraph enforces);
  - positive and negative controls are asserted on EVERY run and the
    module refuses to write evidence when one fails.

Grades: twin-strong (top-1, margin >= MARGIN or sole candidate, name >=
NAME_FLOOR, plus a corroborator: delta=0 or a shared callee),
twin-candidate (top-1 and eligible, under-margined or uncorroborated),
refused (weak-name / tie / no-shared-class / free-function / collision
/ no-candidates).

ANALYSIS OUTPUT, not retail evidence: a twin-* row is a naming/template
proposal, external-candidate grade, promoted only in supervised review.

  python3 -m homm3.analysis.h2_twins              write evidence/homm2-overlap/twins.csv
  python3 -m homm3.analysis.h2_twins --calibrate  report calibration, write nothing
"""
from __future__ import annotations

import argparse
import csv
import re
import statistics
import sys
from collections import defaultdict, namedtuple

from homm3.core import common
from homm3.analysis import homm2_overlap as h2

OUT = common.EVIDENCE_DIR / "homm2-overlap/twins.csv"
XREF = common.EVIDENCE_DIR / "dc-xref-graph.tsv"

WEIGHTS = (0.6, 0.2, 0.2)  # name, arity, callee
MARGIN = 0.15              # twin-strong needs this lead over the runner-up
EPS = 1e-9                 # float tolerance on every margin comparison
NAME_FLOOR = 0.5           # twin-strong needs this much name agreement
ELIGIBLE = 0.4             # below this the name signal is refused outright
STOP = {"get", "set", "do", "is", "my", "the"}

_CAMEL = re.compile(r"[A-Z]+(?=[A-Z][a-z])|[A-Z]?[a-z]+|[A-Z]+|\d+")

Cand = namedtuple("Cand", "offset norm case_name method tokens params "
                          "module callees")

POSITIVE_CONTROLS = [
    ("advmanager::systemoptions", "advmanager::dosystemoptions"),
    ("combatmanager::getnextarmy", "combatmanager::nextarmy"),
    ("advmanager::processmapchange", "advmanager::processmapchangenew"),
]
NEGATIVE_FREE = ("compress", "uncompress")  # the documented zlib collision


def tokens_of(case_name: str) -> frozenset:
    return frozenset(t.lower()
                     for t in _CAMEL.findall(case_name.replace("~", "")))


def strip_templates(name: str) -> str:
    out, depth = [], 0
    for ch in name:
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth = max(0, depth - 1)
        elif depth == 0:
            out.append(ch)
    return "".join(out)


def jaccard(a, b) -> float:
    if not a or not b:
        return 0.0
    inter = len(a & b)
    return inter / len(a | b) if inter else 0.0


def read_dc_rows():
    """[(offset, stripped case name, module, params|None)] for every DC
    function row."""
    rows = []
    with (common.EVIDENCE_DIR / "dreamcast/functions.csv").open() as fh:
        lines = (ln for ln in fh if not ln.startswith("#"))
        for r in csv.DictReader(lines):
            name = h2._STRIP_STACK.sub("", r["name"])
            params = r.get("params") or ""
            rows.append((int(r["offset"], 16), name, r["module"],
                         int(params) if params.isdigit() else None))
    return rows


def read_dc_calls():
    """src_dc_offset -> {normalized callee name} for direct calls only
    (same bsr_calls>0 rule as dc_callgraph; pool_refs are
    address-takings, not call sites)."""
    calls = defaultdict(set)
    with XREF.open() as fh:
        rows = csv.DictReader((l for l in fh if not l.startswith("#")),
                              delimiter="\t")
        for row in rows:
            try:
                if int(row.get("bsr_calls") or 0) <= 0:
                    continue
                src = int(row["src_offset"], 16)
            except (ValueError, KeyError, TypeError):
                continue
            dst = h2._STRIP_STACK.sub("", row.get("dst_name") or "").lower()
            if dst and "$" not in dst:
                calls[src].add(dst)
    return calls


def build_pools(dc_rows, dc_calls, vocab):
    """cls_key(lower) -> [Cand], name-cased, callees pre-restricted to
    the shared vocabulary. Compiler-generated and operator rows never
    enter a pool: their homm2 counterparts never enter the universe."""
    pools = defaultdict(list)
    for offset, name, module, params in dc_rows:
        if "$" in name or "`" in name:
            continue
        stripped = strip_templates(name)
        if "::" not in stripped:
            continue
        cls, method = stripped.rsplit("::", 1)
        if not method or method.startswith("operator"):
            continue
        callees = frozenset(n for n in dc_calls.get(offset, ())
                            if n in vocab)
        pools[cls.lower()].append(Cand(
            offset, stripped.lower(), stripped, method,
            tokens_of(method), params, module, callees))
    return pools


def score(h2_tokens, h2_arities, h2_callees, cand):
    """(S, name_j, delta, shared_callees) - the three terms, raw."""
    name_j = jaccard(h2_tokens, cand.tokens)
    delta = None
    arity_term = 0.0
    if h2_arities and cand.params is not None:
        delta = min((cand.params - (a + 1) for a in h2_arities), key=abs)
        arity_term = 1.0 if delta == 0 else 0.5 if abs(delta) == 1 else 0.0
    callee_term = jaccard(h2_callees, cand.callees)
    shared = len(h2_callees & cand.callees)
    w_name, w_arity, w_callee = WEIGHTS
    return (w_name * name_j + w_arity * arity_term + w_callee * callee_term,
            name_j, delta, shared)


def rank(pool, h2_tokens, h2_arities, h2_callees):
    """Candidates scored and sorted best-first (offset breaks ties so
    the order is deterministic)."""
    scored = [(score(h2_tokens, h2_arities, h2_callees, c), c) for c in pool]
    scored.sort(key=lambda sc: (-sc[0][0], sc[1].offset))
    return scored


def margin_of(ranked):
    """Lead of the best row over the best DIFFERENTLY-NAMED candidate -
    same-name overloads are the same naming proposal, not competition."""
    (s0, _n, _d, _sh), best = ranked[0]
    for (s, _n2, _d2, _sh2), cand in ranked[1:]:
        if cand.norm != best.norm:
            return s0 - s
    return s0


# --- the production pass ----------------------------------------------------------


def h2_universe(buka_by, pol_by, dc_norms):
    """[(norm, mangled, branch, rva, unit)] for every unpaired homm2
    function - one output row each, no silent drops."""
    rows = []
    for norm in sorted(set(buka_by) | set(pol_by)):
        if norm in dc_norms:
            continue  # exact-paired: functions.csv owns it
        if norm in buka_by:
            rva, mangled, unit, _size = buka_by[norm]
            branch = "both" if norm in pol_by else "buka"
        else:
            rva, mangled, unit, _size = pol_by[norm]
            branch = "pol"
        rows.append((norm, mangled, branch, rva, unit))
    return rows


def propose(universe, pools, consumed, arities, edges, vocab):
    """One result dict per universe row."""
    results = []
    for norm, mangled, branch, rva, unit in universe:
        h2_arities = arities.get(norm, [])
        base = {
            "h2_name": norm, "h2_branch": branch, "rva": rva, "unit": unit,
            "h2_method": h2.h2_method_case(mangled) or "",
            "h2_arity": "/".join(str(a) for a in h2_arities),
            "dc_class": "", "cand": None, "score": None, "name_j": None,
            "delta": None, "shared": 0, "h2_callees": 0, "dc_callees": 0,
            "margin": None, "grade": "refused", "detail": "",
        }
        if "::" not in norm:
            base["detail"] = "free-function"
            results.append(base)
            continue
        cls_key = norm.rsplit("::", 1)[0]
        pool = [c for c in pools.get(cls_key, ())
                if c.norm not in consumed]
        if cls_key not in pools:
            base["detail"] = "no-shared-class"
            results.append(base)
            continue
        base["dc_class"] = cls_key
        if not pool:
            base["detail"] = "no-candidates (all consumed by exact pairs)"
            results.append(base)
            continue
        h2_tokens = tokens_of(base["h2_method"])
        h2_callees = frozenset(n for n in edges[branch].get(rva, ())
                               if n in vocab)
        ranked = rank(pool, h2_tokens, h2_arities, h2_callees)
        (s, name_j, delta, shared), best = ranked[0]
        margin = margin_of(ranked)
        base.update(score=s, name_j=name_j, delta=delta, shared=shared,
                    h2_callees=len(h2_callees), dc_callees=len(best.callees),
                    margin=margin)
        meaty = (h2_tokens & best.tokens) - STOP
        if name_j < ELIGIBLE or not meaty:
            base["detail"] = (f"weak-name (best {best.case_name} "
                              f"S={s:.2f} name={name_j:.2f})")
            results.append(base)
            continue
        if len(ranked) > 1 and margin < 1e-9:
            base["detail"] = (f"tie (best {best.case_name} vs "
                              f"{ranked[1][1].case_name} S={s:.2f})")
            results.append(base)
            continue
        base["cand"] = best
        corroborated = delta == 0 or shared >= 1
        if (margin >= MARGIN - EPS or len(ranked) == 1) \
                and name_j >= NAME_FLOOR and corroborated:
            base["grade"] = "twin-strong"
        else:
            base["grade"] = "twin-candidate"
        notes = [f"margin {margin:.2f}"]
        if not h2_arities:
            notes.append("no-arity")
        if not h2_callees or not best.callees:
            notes.append("no-callee-signal")
        overloads = sum(1 for _sc, c in ranked if c.norm == best.norm)
        if overloads > 1:
            notes.append(f"overloads={overloads}")
        base["detail"] = ", ".join(notes)
        results.append(base)
    return enforce_injective(results)


def enforce_injective(results):
    """Two homm2 methods proposing the same DC row cannot both be right.
    The leader keeps the proposal only with a MARGIN lead; otherwise
    every claimant is demoted."""
    by_offset = defaultdict(list)
    for row in results:
        if row["cand"] is not None:
            by_offset[row["cand"].offset].append(row)
    for _offset, owners in by_offset.items():
        if len(owners) < 2:
            continue
        owners.sort(key=lambda r: -r["score"])
        lead = owners[0]["score"] - owners[1]["score"]
        losers = owners[1:] if lead >= MARGIN - EPS else owners
        for row in losers:
            row["detail"] = (f"collision ({len(owners)} claimants for "
                             f"{row['cand'].case_name}, lead {lead:.2f}); "
                             + row["detail"])
            row["grade"] = "refused"
            row["cand"] = None
    return results


# --- calibration ------------------------------------------------------------------


def calibrate(pools, dc_norms, buka_by, pol_by, arities, edges, vocab):
    """Leave-one-out and rename-stress top-1 over the exact-name pairs.

    LOO scores the true name against its full class pool (truth present,
    name identity intact) - an upper bound that validates margins and
    distractor separation. The stress pass deletes each single token
    from the homm2 token set in turn (the observed rename pattern: all
    three known twins are one-token edits) and takes the WORST rank;
    1 - stress_top1 is the published false-positive estimate, and
    strong_fp counts stress winners that would still grade twin-strong.
    """
    loo_n = loo_k = stress_n = stress_k = strong_fp = 0
    margins = []
    for norm in sorted(dc_norms):
        if "::" not in norm:
            continue
        src = buka_by.get(norm) or pol_by.get(norm)
        if src is None:
            continue
        rva, mangled, _unit, _size = src
        method = h2.h2_method_case(mangled)
        if not method:
            continue
        cls_key = norm.rsplit("::", 1)[0]
        pool = pools.get(cls_key, ())
        if len(pool) < 2 or not any(c.norm == norm for c in pool):
            continue
        branch = "buka" if norm in buka_by else "pol"
        h2_arities = arities.get(norm, [])
        h2_callees = frozenset(n for n in edges[branch].get(rva, ())
                               if n in vocab)
        h2_tokens = tokens_of(method)
        ranked = rank(pool, h2_tokens, h2_arities, h2_callees)
        loo_n += 1
        if ranked[0][1].norm == norm:
            loo_k += 1
            margins.append(margin_of(ranked))
        if len(h2_tokens) < 2:
            continue
        stress_n += 1
        worst_ok = True
        for tok in sorted(h2_tokens):
            variant = h2_tokens - {tok}
            v_ranked = rank(pool, variant, h2_arities, h2_callees)
            (s, name_j, delta, shared), best = v_ranked[0]
            if best.norm == norm:
                continue
            worst_ok = False
            meaty = (variant & best.tokens) - STOP
            if (name_j >= NAME_FLOOR and meaty
                    and margin_of(v_ranked) >= MARGIN - EPS
                    and (delta == 0 or shared >= 1)):
                strong_fp += 1
        if worst_ok:
            stress_k += 1
    return {
        "loo_n": loo_n, "loo_k": loo_k,
        "stress_n": stress_n, "stress_k": stress_k,
        "strong_fp": strong_fp,
        "margin_med": statistics.median(margins) if margins else 0.0,
    }


def check_controls(results) -> list:
    """Positive and negative controls; returns failure strings."""
    failures = []
    by_name = {r["h2_name"]: r for r in results}
    for norm, expected in POSITIVE_CONTROLS:
        row = by_name.get(norm)
        if row is None:
            failures.append(f"positive control {norm}: not in the universe")
        elif not row["grade"].startswith("twin"):
            failures.append(f"positive control {norm}: graded "
                            f"{row['grade']} ({row['detail']})")
        elif row["cand"].norm != expected:
            failures.append(f"positive control {norm}: proposed "
                            f"{row['cand'].norm}, expected {expected}")
    for norm in NEGATIVE_FREE:
        row = by_name.get(norm)
        if row is not None and row["grade"].startswith("twin"):
            failures.append(f"negative control {norm}: must be refused "
                            f"free-function, got {row['grade']}")
    dispel = by_name.get("army::dispelgood")
    if dispel is not None and dispel["cand"] is not None \
            and dispel["cand"].norm == "army::is":
        failures.append("negative control army::dispelgood: proposed "
                        "army::is (the substring-nonsense case)")
    # homm2 carries BOTH SpellMessage and ShowSpellMessage; the DC
    # ShowSpellMessage row belongs to its exact-name pair, and the
    # consumed-exclusion must keep SpellMessage's hands off it.
    spell = by_name.get("combatmanager::spellmessage")
    if spell is not None and spell["cand"] is not None \
            and spell["cand"].norm == "combatmanager::showspellmessage":
        failures.append("negative control combatmanager::spellmessage: "
                        "proposed the consumed exact-pair row "
                        "ShowSpellMessage")
    return failures


# --- output -----------------------------------------------------------------------


_GRADE_ORDER = {"twin-strong": 0, "twin-candidate": 1, "refused": 2}


def write_rows(results, cal):
    header = ["h2_name", "h2_method", "h2_branch", "h2_rva", "h2_unit",
              "h2_arity", "dc_class", "proposed_dc_name",
              "proposed_dc_offset", "proposed_dc_module", "dc_params",
              "score", "name_score", "arity_delta", "callee_shared",
              "callee_h2_count", "callee_dc_count", "margin", "grade",
              "detail"]
    extra = h2.h2_provenance() + [
        f"# calibration: loo_top1={cal['loo_k']}/{cal['loo_n']} "
        f"stress_top1={cal['stress_k']}/{cal['stress_n']} "
        f"strong_fp={cal['strong_fp']} margin_med={cal['margin_med']:.2f}"]
    with OUT.open("w", newline="") as fh:
        for line in common.provenance("homm3.analysis.h2_twins", extra):
            fh.write(line + "\n")
        w = csv.writer(fh)
        w.writerow(header)
        for r in sorted(results, key=lambda r: (_GRADE_ORDER[r["grade"]],
                                                r["h2_name"])):
            cand = r["cand"]
            w.writerow([
                r["h2_name"], r["h2_method"], r["h2_branch"],
                f"0x{r['rva']:x}", r["unit"], r["h2_arity"], r["dc_class"],
                cand.case_name if cand else "",
                f"0x{cand.offset:x}" if cand else "",
                cand.module if cand else "",
                cand.params if cand and cand.params is not None else "",
                f"{r['score']:.3f}" if r["score"] is not None else "",
                f"{r['name_j']:.3f}" if r["name_j"] is not None else "",
                r["delta"] if r["delta"] is not None else "",
                r["shared"], r["h2_callees"], r["dc_callees"],
                f"{r['margin']:.3f}" if r["margin"] is not None else "",
                r["grade"], r["detail"]])
    print(f"[h2_twins] {len(results)} rows -> "
          f"{OUT.relative_to(common.HOMM3_DIR)}")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        prog="python3 -m homm3.analysis.h2_twins")
    parser.add_argument("--calibrate", action="store_true",
                        help="report calibration and controls, write nothing")
    args = parser.parse_args(argv)

    h2.gate_tree(h2.H2_POL, h2.POL_EXE)
    h2.gate_tree(h2.H2_BUKA, h2.BUKA_EXE, extra=("docs/buka-va-queue.tsv",))

    pol_units = h2.h2_units(h2.H2_POL)
    pol_funcs, _ = h2.h2_symbols(h2.H2_POL, set(pol_units.values()))
    buka_units = h2.h2_units(h2.H2_BUKA)
    buka_funcs, _ = h2.h2_symbols(h2.H2_BUKA, set(buka_units.values()))
    buka_by = h2.branch_by_norm(buka_funcs)
    pol_by = h2.branch_by_norm(pol_funcs)
    arities = h2.buka_signatures(h2.H2_BUKA)

    dc_rows = read_dc_rows()
    dc_norms = {name.lower() for _o, name, _m, _p in dc_rows}
    h2_norms = set(buka_by) | set(pol_by)
    vocab = h2_norms & dc_norms

    edges = {
        "buka": h2.h2_call_edges(h2.H2_BUKA / h2.BUKA_EXE,
                                 h2.h2_all_funcs(h2.H2_BUKA)),
        "pol": h2.h2_call_edges(h2.H2_POL / h2.POL_EXE,
                                h2.h2_all_funcs(h2.H2_POL)),
    }
    edges["both"] = edges["buka"]

    dc_calls = read_dc_calls()
    pools = build_pools(dc_rows, dc_calls, vocab)
    consumed = dc_norms & h2_norms  # exact pairs own these DC names

    universe = h2_universe(buka_by, pol_by, dc_norms)
    scored_classes = sum(1 for n, *_ in universe
                         if "::" in n and n.rsplit("::", 1)[0] in pools)
    print(f"[h2_twins] universe: {len(universe)} unpaired "
          f"({scored_classes} with a shared class); "
          f"vocab {len(vocab)} shared names; "
          f"{sum(len(p) for p in pools.values())} DC candidates "
          f"in {len(pools)} classes")

    cal = calibrate(pools, dc_norms, buka_by, pol_by, arities, edges, vocab)
    print(f"[h2_twins] calibration: "
          f"loo_top1 {cal['loo_k']}/{cal['loo_n']}, "
          f"stress_top1 {cal['stress_k']}/{cal['stress_n']}, "
          f"strong_fp {cal['strong_fp']}, "
          f"margin_med {cal['margin_med']:.2f}")

    results = propose(universe, pools, consumed, arities, edges, vocab)
    failures = check_controls(results)
    counts = defaultdict(int)
    for row in results:
        key = row["grade"]
        if key == "refused":
            key = f"refused ({row['detail'].split(' ', 1)[0].rstrip(',')})"
        counts[key] += 1
    for key in sorted(counts):
        print(f"  {key:32s} {counts[key]}")
    if failures:
        for failure in failures:
            print(f"[h2_twins] CONTROL FAILED: {failure}", file=sys.stderr)
        print("[h2_twins] refusing to write evidence", file=sys.stderr)
        return 1
    if args.calibrate:
        print("[h2_twins] controls pass; --calibrate writes nothing")
        return 0
    write_rows(results, cal)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
