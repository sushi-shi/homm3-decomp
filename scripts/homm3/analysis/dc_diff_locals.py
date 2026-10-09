"""Our source's locals, temporaries and calls against Dreamcast's inventory.

`homm3 dreamcast diff-locals` lists, for every Dreamcast-paired retail
function, the locals/temporaries and calls our authored C++ has that the
Dreamcast CodeView inventory lacks, and the reverse. Functions are ranked by
their current ledger score (non-exact first, closest to exact first).

A Dreamcast local inventory is a lower bound: register-only values can be
missing, and the source is an older revision. An extra local is a lead to
test under VC6, not a defect by itself. `--probe` performs that test: each
removable extra local (one initialised declaration, never reassigned or
address-taken) is substituted into its uses in a disposable copy of the TU,
compiled with the unit's exact build profile, normalised and scored exactly as
`homm3 build --fast` scores it, then discarded. The authored tree, objects,
report and ledger are never written.
"""
from __future__ import annotations

from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Iterable, TextIO

from homm3.analysis import dc_callees, dreamcast
from homm3.analysis.source_facts import name_key, semantic_name_key

SCHEMA = "homm3.dreamcast-diff-locals.v1"
CAUTION = (
    "Dreamcast records a lower-bound local inventory of an older SH4 build. "
    "Differences are leads for VC6 experiments, never source verdicts; keep a "
    "change only when retail bytes and the remaining evidence agree."
)
PROBE_ROOT = Path("build/dreamcast/diff-locals-probe")


# --- selection and scores -----------------------------------------------------

def _units() -> list[dict]:
    from homm3.vc6 import _unit
    return list(_unit._manifest().get("unit", []))


def unit_sources() -> dict[str, str]:
    """unit -> repository-relative source path."""
    return {row["unit"]: row["source"] for row in _units()}


def ledger_by_va(baseline=None) -> dict[int, list[dict[str, Any]]]:
    from homm3.core import common
    from homm3.match import status
    rows = status.load_baseline() if baseline is None else baseline
    out: dict[int, list[dict[str, Any]]] = defaultdict(list)
    for (unit, symbol), row in rows.items():
        if row.rva is not None:
            out[common.IMAGE_BASE + row.rva].append(
                {"unit": unit, "symbol": symbol, "cur": row.cur, "max": row.max, "hist": row.hist})
    return out


def score_for(va: int, path: str, ledger: dict[int, list[dict]], sources: dict[str, str]) \
        -> dict[str, Any] | None:
    rows = ledger.get(va, ())
    owners = [row for row in rows if sources.get(row["unit"]) == path]
    chosen = owners or list(rows)
    return max(chosen, key=lambda row: row["cur"] if row["cur"] is not None else -1) \
        if chosen else None


def rank_key(item: dict[str, Any]) -> tuple:
    score = item.get("score")
    value = None if score is None else (score["cur"] if score["cur"] is not None else score["max"])
    if value is None:
        return (2, 0.0, item["va"])
    return (1 if value >= 100 else 0, -value, item["va"])


def paired(corpus, *, selectors=(), units=(), modules=(), all_functions=False) \
        -> list[tuple[dict[str, str], Any]]:
    """(Dreamcast roster row, source claim) for every selected paired function."""
    if sum((bool(selectors), bool(units), bool(modules), bool(all_functions))) != 1:
        raise dreamcast.DreamcastError(
            "supply SELECTOR(s), --unit U, --module M, or --all (exactly one)")
    pairs: list[tuple[dict[str, str], Any]] = []
    if selectors:
        for selector in selectors:
            for row in dreamcast._matches(corpus, selector):
                claims = corpus.claims_by_key.get(corpus.key(row), ())
                if not claims:
                    dreamcast._note(f"{selector}: {row['name']} has no retail source claim; skipped")
                pairs.extend((row, claim) for claim in claims)
    else:
        sources = unit_sources()
        if units:
            unknown = sorted(set(units) - set(sources))
            if unknown:
                raise dreamcast.NoMatch("unknown unit(s): " + ", ".join(unknown))
            wanted_paths = {sources[unit] for unit in units}
        if modules:
            wanted_modules = {name.lower().removesuffix(".obj") + ".obj" for name in modules}
        for key, claims in corpus.claims_by_key.items():
            row = corpus.by_key.get(key)
            if row is None:
                continue
            for claim in claims:
                if units and claim.path not in wanted_paths:
                    continue
                if modules and row["module"].lower() not in wanted_modules:
                    continue
                pairs.append((row, claim))
    unique = {(corpus.key(row), claim.va): (row, claim) for row, claim in pairs}
    if not unique:
        raise dreamcast.NoMatch("no Dreamcast-paired retail functions in the selection")
    return list(unique.values())


# --- inventories ----------------------------------------------------------------

def _local_names(row: dict[str, Any]) -> list[str]:
    return list(row.get("aliases") or []) + [row["name"]]


def diff_locals(dc_locals: Iterable[dict[str, Any]], ours: Iterable[dict[str, Any]]) \
        -> tuple[list[dict], list[dict], list[tuple[dict, dict]]]:
    """(ours only, Dreamcast only, matched pairs).

    Exact case/underscore-folded names (or owning `Before normalization`
    aliases) pair first; the documented Hungarian/scope normalization pairs
    the rest. Each local pairs at most once, so shadowed copies stay visible.
    """
    left = [row for row in dc_locals]
    right = [row for row in ours]
    matched: list[tuple[dict, dict]] = []
    for key_of in (name_key, semantic_name_key):
        for dc in list(left):
            key = key_of(dc["name"])
            for cpp in right:
                if any(key_of(name) == key for name in _local_names(cpp)):
                    matched.append((dc, cpp))
                    left.remove(dc)
                    right.remove(cpp)
                    break
    return right, left, matched


def dc_inventory(dossier, clue_rows: Iterable[dict[str, Any]]) -> dict[str, Any]:
    """Every named call in the procedure extent plus statement temporaries.

    SH line attribution bleeds across expanded helpers (a caller's own call
    can sit on a header row), so calls are not filtered by source file. A call
    on an inline-residue row records the candidate helper definition(s)."""
    residue = {(clue["address"], dreamcast._source_key(clue["source"]), clue["line"]):
               clue["definitions"] for clue in clue_rows}
    calls, temporaries = [], Counter()
    for statement in dossier.shape.statements:
        where = (statement.address, dreamcast._source_key(statement.source_file),
                 statement.source_line)
        constructed, destroyed = Counter(), Counter()
        for call in statement.calls:
            identity = dc_callees.callee(call.name)
            if identity is None:
                continue
            if identity.kind == "constructor":
                constructed[identity.key] += 1
            elif identity.kind == "destructor":
                destroyed[identity.key[1:]] += 1
            item = {"name": call.name, "key": identity.key, "kind": identity.kind,
                    "display": identity.display, "line": statement.source_line,
                    "file": dreamcast._basename(statement.source_file)}
            if where in residue:
                item["inline_row"] = list(residue[where])
            calls.append(item)
        # A class constructed and destroyed on one statement row is a
        # statement temporary (a by-value argument/result with a destructor).
        for key in constructed:
            if destroyed[key]:
                temporaries[key] += min(constructed[key], destroyed[key])
    return {"calls": calls, "temporaries": temporaries}


def our_calls(candidate: dict[str, Any]) -> list[dict[str, Any]]:
    out = []
    for call in candidate.get("calls", []):
        if call.get("copy"):
            continue
        identity = dc_callees.callee(call["name"])
        if identity is None or identity.kind == "destructor":
            continue
        out.append({"name": call["name"], "key": identity.key, "kind": identity.kind,
                    "display": identity.display, "line": call["line"]})
    return out


def diff_calls(dc: list[dict], ours: list[dict], traces: dict[str, list[dict]]) \
        -> tuple[list[dict], list[dict]]:
    """Per-callee count differences; destructors are not authored calls."""
    dc = [call for call in dc if call["kind"] != "destructor"]
    left, right = defaultdict(list), defaultdict(list)
    for call in dc:
        left[call["key"]].append(call)
    for call in ours:
        right[call["key"]].append(call)
    ours_only, dc_only = [], []
    for key in sorted(set(left) | set(right)):
        a, b = left.get(key, []), right.get(key, [])
        if len(a) == len(b):
            continue
        sample = (b or a)[0]
        item = {"key": key, "callee": sample["name"], "kind": sample["kind"],
                "ours": len(b), "dreamcast": len(a),
                "our_lines": sorted({call["line"] for call in b}),
                "dc_lines": sorted({f"{call['file']}:{call['line']}" for call in a}),
                "dc_inline_rows": sorted({name for call in a
                                          for name in call.get("inline_row", ())})}
        if len(b) > len(a):
            item["dc_inline"] = traces.get(key, [])
            ours_only.append(item)
        else:
            dc_only.append(item)
    absent_first = lambda item: (min(item["ours"], item["dreamcast"]) > 0, item["key"])
    return sorted(ours_only, key=absent_first), sorted(dc_only, key=absent_first)


def diff_temporaries(dc: Counter, candidate: dict[str, Any]) -> tuple[list[dict], list[dict]]:
    """Destructor-carrying temporaries only: Dreamcast leaves no trace of
    trivially destructible ones."""
    ours = Counter()
    lines = defaultdict(list)
    for temporary in candidate.get("temporaries", []):
        if temporary.get("destructor"):
            key = dc_callees.type_key(temporary["type"])
            ours[key] += 1
            lines[key].append(temporary["line"])
    ours_only = [{"type": key, "ours": ours[key], "dreamcast": dc.get(key, 0),
                  "our_lines": sorted(lines[key])}
                 for key in sorted(ours) if ours[key] > dc.get(key, 0)]
    dc_only = [{"type": key, "ours": ours.get(key, 0), "dreamcast": dc[key]}
               for key in sorted(dc) if dc[key] > ours.get(key, 0)]
    return ours_only, dc_only


def removable(local: dict[str, Any]) -> str | None:
    """None when --probe can substitute the local; otherwise the reason."""
    if local.get("storage_class") in ("static", "extern"):
        return "static storage"
    if not local.get("declaration"):
        return "not a sole declaration statement"
    if not local.get("initializer"):
        return "no initializer"
    if "[" in local.get("type", ""):
        return "array"
    uses = local.get("uses", [])
    if not uses:
        return "unused"
    if any(use["modifies"] for use in uses):
        return "reassigned, incremented or address-taken"
    if local["declaration"]["macro"] or local["initializer"]["macro"] \
            or any(use["macro"] for use in uses):
        return "macro expansion"
    return None


def _gap_line(gap: str) -> str:
    """One line per gap: the summary plus the first compiler error, if any."""
    from homm3.core import common
    lines = gap.replace(str(common.HOMM3_DIR) + "/", "").splitlines()
    error = next((line.strip() for line in lines[1:] if "error:" in line), None)
    return lines[0].rstrip(":") + (f" ({error})" if error else "")


def _one_line(text: str, limit: int = 60) -> str:
    text = " ".join(text.split())
    return text if len(text) <= limit else text[:limit - 3] + "..."


def analyse(corpus, row, claim, *, dump, data, types, parser, ledger, sources) -> dict[str, Any]:
    from homm3.core import common
    item: dict[str, Any] = {
        "va": claim.va, "name": row["name"], "module": row["module"],
        "dc_offset": dreamcast._integer(row["offset"]), "source": claim.path,
        "score": score_for(claim.va, claim.path, ledger, sources), "gaps": []}
    dossier = dreamcast.build_dossier(corpus, row, dump=dump, data=data, type_table=types)
    clue_rows = dreamcast._inline_clue_rows(corpus, row, dump.source_lines.get(row["module"], ()))
    groups = dreamcast._inline_clue_groups(clue_rows)
    traces = dc_callees.inline_traces(groups)
    dc = dc_inventory(dossier, clue_rows)
    # `this`/hidden return storage records belong to expanded helpers.
    dc_locals = [{"name": local.name, "type": local.type_name, "storage": local.storage}
                 for local in dossier.shape.locals
                 if local.name not in ("this", "__$ReturnUdt")]
    mangled = corpus._retail_name(claim.va - common.IMAGE_BASE)
    if not mangled:
        item["gaps"].append("retail/source symbol binding unavailable; run homm3 build")
        return item
    item["symbol"] = mangled
    try:
        candidate = parser(common.HOMM3_DIR / claim.path, mangled)
    except (ValueError, OSError, subprocess.SubprocessError, json.JSONDecodeError) as exc:
        item["gaps"].append(str(exc).splitlines()[0])
        return item
    item["gaps"].extend(_gap_line(gap) for gap in candidate.get("gaps", []))
    ours_only, dc_only, matched = diff_locals(dc_locals, candidate.get("locals", []))
    item["locals"] = {
        "dreamcast": len(dc_locals), "ours": len(candidate.get("locals", [])),
        "matched": len(matched),
        "ours_only": [{"name": local["name"], "type": local["type"], "line": local["line"],
                       "uses": len(local.get("uses", [])),
                       "probe": removable(local) or "candidate"} for local in ours_only],
        "dreamcast_only": [{"name": local["name"], "type": local["type"],
                            "storage": local["storage"]} for local in dc_only],
    }
    temporaries = diff_temporaries(dc["temporaries"], candidate)
    item["temporaries"] = {"ours_only": temporaries[0], "dreamcast_only": temporaries[1]}
    calls = diff_calls(dc["calls"], our_calls(candidate), traces)
    item["calls"] = {"ours_only": calls[0], "dreamcast_only": calls[1]}
    item["_candidate"] = candidate  # probe input; dropped from output
    return item


# --- probe ------------------------------------------------------------------------

def substitute(source: bytes, local: dict[str, Any]) -> bytes:
    """Replace each use with the parenthesized initializer, drop the declaration."""
    init = local["initializer"]
    expression = b"(" + source[init["begin"]:init["end"]] + b")"
    declaration = local["declaration"]
    start, end = declaration["begin"], declaration["end"]
    line_start = source.rfind(b"\n", 0, start) + 1
    line_end = source.find(b"\n", end)
    line_end = len(source) if line_end < 0 else line_end
    if not source[line_start:start].strip() and not source[end:line_end].strip():
        start, end = line_start, min(len(source), line_end + 1)
    edits = [(use["begin"], use["end"], expression) for use in local["uses"]]
    edits.append((start, end, b""))
    for begin, finish, text in sorted(edits, reverse=True):
        source = source[:begin] + text + source[finish:]
    return source


class UnitProbe:
    """Compile and score disposable copies of one unit's TU."""

    def __init__(self, unit: str, root: Path):
        from homm3.build import normalize_objs as normalize
        from homm3.match import status
        from homm3.vc6 import _unit, tu_state_sweep as scoring
        self.unit = unit
        self.source = _unit.source_for_unit(unit)
        self.flags = _unit.flags_for_unit(unit)
        if self.source is None or not self.flags:
            raise dreamcast.DreamcastError(f"{unit}: no source or compiler profile")
        target = normalize.OBJDIFF / "target" / f"{unit}.c.obj"
        if not target.is_file():
            raise dreamcast.DreamcastError(f"{unit}: no retail target; run homm3 build --fast {unit}")
        # The report is optional: it only cross-checks the unchanged copy.
        self.report = (status.fn_fuzzy(status.load_report())
                       if status.REPORT.is_file() else {})
        scored = tuple(sorted({key for key in self.report if key[0] == unit}
                              | {key for key in status.load_baseline() if key[0] == unit}))
        if not scored:
            raise dreamcast.DreamcastError(f"{unit}: no scored functions in the ledger or report")
        self.original = self.source.read_bytes()
        self.plan = scoring.UnitPlan(
            unit, self.source, "", hashlib.sha256(self.original).hexdigest(), (), scored, scored,
            scoring._first_pass(unit, target.read_bytes()), "diff-locals", root, ())
        self.scoring = scoring
        self.root = root
        self.control: dict[str, float] | None = None

    def score(self, text: bytes, tag: str) -> dict[str, float]:
        from homm3.core import common
        self.root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix=f"{self.unit}-{tag}-", dir=self.root) as raw:
            directory = Path(raw)
            copy = directory / self.source.name
            copy.write_bytes(text)
            obj = directory / "candidate.obj"
            proc = subprocess.run(
                [sys.executable, "-m", "homm3.core.cc_wrap", "--out", str(obj),
                 "--src", str(copy), "--", *self.flags],
                capture_output=True, text=True, cwd=common.HOMM3_DIR)
            if proc.returncode or not obj.is_file():
                tail = "\n".join((proc.stdout + proc.stderr).strip().splitlines()[-4:])
                raise RuntimeError(tail or "compile failed")
            base, target = self.scoring._normalized_pair(self.plan, obj.read_bytes())
            return self.scoring._report_scores(self.plan, base, target, directory)

    def baseline(self) -> dict[str, float]:
        if self.control is None:
            self.control = self.score(self.original, "control")
        return self.control

    def drift(self) -> list[str]:
        """Functions whose unchanged-copy score differs from the last report."""
        control = self.baseline()
        return sorted(symbol for symbol, value in control.items()
                      if abs(value - self.report.get((self.unit, symbol), value)) > 1e-4)


def probe_function(item: dict[str, Any], probe: UnitProbe, jobs: int) -> list[dict[str, Any]]:
    candidate = item.get("_candidate") or {}
    names = {row["name"] for row in item["locals"]["ours_only"] if row["probe"] == "candidate"}
    locals_ = [local for local in candidate.get("locals", []) if local["name"] in names
               and removable(local) is None]
    control = probe.baseline()
    symbol = item.get("symbol")

    def one(local):
        result = {"local": local["name"], "type": local["type"], "uses": len(local["uses"]),
                  "initializer": probe.original[local["initializer"]["begin"]:
                                                local["initializer"]["end"]].decode("latin-1")}
        try:
            scores = probe.score(substitute(probe.original, local), local["name"])
        except (RuntimeError, ValueError) as exc:
            result["error"] = str(exc)
            return result
        before, after = control.get(symbol), scores.get(symbol)
        result.update(before=before, after=after,
                      delta=None if before is None or after is None else round(after - before, 4),
                      collateral=[{"symbol": name, "before": control[name], "after": value,
                                   "delta": round(value - control[name], 4)}
                                  for name, value in sorted(scores.items())
                                  if name != symbol and name in control
                                  and abs(value - control[name]) > 1e-4])
        return result

    with ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        return list(pool.map(one, locals_))


# --- rendering ------------------------------------------------------------------

def _score_text(score: dict[str, Any] | None) -> str:
    if score is None:
        return "unscored"
    cur = "-" if score["cur"] is None else f"{score['cur']:.2f}"
    return f"CUR {cur} MAX {score['max']:.2f}"


def render(payload: dict[str, Any], out: TextIO = sys.stdout) -> None:
    print("DREAMCAST LOCAL/CALL INVENTORY DIFF — ANALYSIS OUTPUT, NOT RETAIL EVIDENCE", file=out)
    print(CAUTION, file=out)
    for item in payload["functions"]:
        print(f"\n0x{item['va']:08x} {_score_text(item['score'])}  {item['name']}"
              f"  [{item['module']} dc:0x{item['dc_offset']:x}; {item['source']}]", file=out)
        for gap in item["gaps"]:
            print(f"  UNCHECKED: {gap}", file=out)
        if "locals" not in item:
            continue
        locals_ = item["locals"]
        print(f"  locals: DC {locals_['dreamcast']}, ours {locals_['ours']}, "
              f"paired {locals_['matched']}", file=out)
        for local in locals_["ours_only"]:
            probe = "" if local["probe"] == "candidate" else f"; probe: {local['probe']}"
            print(f"    + ours  {local['type']} {local['name']} (line {local['line']}, "
                  f"{local['uses']} use(s){probe})", file=out)
        for local in locals_["dreamcast_only"]:
            print(f"    - DC    {local['type']} {local['name']} ({local['storage'] or '-'})", file=out)
        for label, key in (("ours", "ours_only"), ("DC", "dreamcast_only")):
            for temporary in item["temporaries"][key]:
                print(f"    {'+' if key == 'ours_only' else '-'} {label:<5} temporary "
                      f"{temporary['type']} x{temporary['ours'] if key == 'ours_only' else temporary['dreamcast']}"
                      f" (other side {temporary['dreamcast'] if key == 'ours_only' else temporary['ours']})",
                      file=out)
        for call in item["calls"]["ours_only"]:
            print(f"    + ours  call {call['callee']} x{call['ours']} (DC {call['dreamcast']}; "
                  f"lines {','.join(map(str, call['our_lines']))})", file=out)
            for line in dc_callees.render_traces(call.get("dc_inline", [])):
                print(f"        {line}", file=out)
        for call in item["calls"]["dreamcast_only"]:
            inside = (f"; on inline rows of {', '.join(call['dc_inline_rows'][:2])}"
                      if call["dc_inline_rows"] else "")
            print(f"    - DC    call {call['callee']} x{call['dreamcast']} (ours {call['ours']}; "
                  f"DC {','.join(call['dc_lines'][:6])}{inside})", file=out)
        for result in item.get("probe", []):
            if "error" in result:
                print(f"    probe {result['local']}: compile error: "
                      f"{result['error'].splitlines()[-1] if result['error'] else ''}", file=out)
                continue
            delta = "n/a" if result["delta"] is None else f"{result['delta']:+.2f}"
            collateral = (f"; {len(result['collateral'])} other function(s) moved"
                          if result["collateral"] else "")
            before = "-" if result["before"] is None else f"{result['before']:.2f}"
            after = "-" if result["after"] is None else f"{result['after']:.2f}"
            print(f"    probe {result['local']} -> ({_one_line(result['initializer'])}) "
                  f"x{result['uses']}: {before} -> {after} ({delta}){collateral}", file=out)
            for moved in result["collateral"][:3]:
                print(f"        also {moved['symbol']}: {moved['before']:.2f} -> "
                      f"{moved['after']:.2f}", file=out)
    summary = payload["summary"]
    print(f"\n{summary['functions']} function(s); {summary['ours_only_locals']} extra local(s), "
          f"{summary['dreamcast_only_locals']} Dreamcast-only local(s), "
          f"{summary['ours_only_calls']} extra callee(s), "
          f"{summary['dreamcast_only_calls']} Dreamcast-only callee(s)"
          + (f"; {summary['probes']} probe(s), {summary['probe_gains']} gain(s)"
             if "probes" in summary else ""), file=out)


def run(corpus, args) -> int:
    from homm3.analysis import dc_lines, source_facts
    from homm3.core import common, inputs
    from homm3.core.nb11_types import Types
    from homm3.core.project import Project
    pairs = paired(corpus, selectors=args.selectors, units=args.unit or (),
                   modules=args.module or (), all_functions=args.all)
    ledger, sources = ledger_by_va(), unit_sources()
    items = [{"va": claim.va, "row": row, "claim": claim,
              "score": score_for(claim.va, claim.path, ledger, sources)} for row, claim in pairs]
    if args.non_exact:
        items = [item for item in items if item["score"] is None
                 or (item["score"]["cur"] if item["score"]["cur"] is not None
                     else item["score"]["max"]) < 100]
    items.sort(key=rank_key)
    if args.limit < 0:
        raise dreamcast.DreamcastError("--limit must be >= 0")
    if args.limit:
        items = items[:args.limit]
    if not items:
        raise dreamcast.NoMatch("no functions left after --non-exact/--limit")
    dump, data = dc_lines.load_symbols(), inputs.read_dreamcast_exe()
    types = Types.from_symbols(dump)
    project = Project(common.HOMM3_DIR)
    parser = source_facts.CandidateParser(project.root, batch=len(items) > 1, project=project)
    results = [analyse(corpus, item["row"], item["claim"], dump=dump, data=data, types=types,
                       parser=parser, ledger=ledger, sources=sources) for item in items]
    summary = {
        "functions": len(results),
        "ours_only_locals": sum(len(r["locals"]["ours_only"]) for r in results if "locals" in r),
        "dreamcast_only_locals": sum(len(r["locals"]["dreamcast_only"]) for r in results if "locals" in r),
        "ours_only_calls": sum(len(r["calls"]["ours_only"]) for r in results if "calls" in r),
        "dreamcast_only_calls": sum(len(r["calls"]["dreamcast_only"]) for r in results if "calls" in r),
    }
    if args.probe:
        if not 1 <= args.jobs <= 4:
            raise dreamcast.DreamcastError("--jobs must be 1..4")
        root = common.HOMM3_DIR / PROBE_ROOT
        probes: dict[str, UnitProbe] = {}
        by_path = {path: unit for unit, path in sources.items()}
        for item in results:
            unit = by_path.get(item["source"])
            if "locals" not in item:
                continue
            if unit is None:
                item["gaps"].append("probe skipped: the definition is not in a unit source file")
                continue
            if unit not in probes:
                probes[unit] = UnitProbe(unit, root)
                try:
                    drift = probes[unit].drift()
                except RuntimeError as exc:
                    raise dreamcast.DreamcastError(f"{unit}: unchanged-copy control failed: {exc}") from exc
                if drift:
                    dreamcast._note(f"{unit}: unchanged-copy control differs from the last report for "
                                    f"{len(drift)} function(s); deltas are against the control")
            item["probe"] = probe_function(item, probes[unit], args.jobs)
        shutil.rmtree(root, ignore_errors=True)
        summary["probes"] = sum(len(item.get("probe", [])) for item in results)
        summary["probe_gains"] = sum(1 for item in results for result in item.get("probe", [])
                                     if (result.get("delta") or 0) > 0)
    for item in results:
        item.pop("_candidate", None)
    payload = {"schema": SCHEMA, "authority": dreamcast.AUTHORITY, "caution": CAUTION,
               "summary": summary, "functions": results}
    if args.json:
        json.dump(payload, sys.stdout, indent=2, sort_keys=True)
        print()
    else:
        render(payload)
    return 0


def add_parser(sub) -> None:
    parser = sub.add_parser(
        "diff-locals", help="our locals/temporaries/calls against the Dreamcast inventory")
    parser.add_argument("selectors", nargs="*", metavar="SELECTOR",
                        help="retail VA/RVA, dc:OFF, module:OFF, or name; repeatable")
    scope = parser.add_mutually_exclusive_group()
    scope.add_argument("--unit", action="append", metavar="UNIT",
                       help="build unit (config/units.toml); repeatable")
    scope.add_argument("--module", action="append", metavar="MODULE",
                       help="Dreamcast module[.obj]; repeatable")
    scope.add_argument("--all", action="store_true", help="every Dreamcast-paired function")
    parser.add_argument("--non-exact", action="store_true",
                        help="skip functions whose current score is 100%%")
    parser.add_argument("--limit", type=int, default=0,
                        help="analyse only the N best-ranked functions (default 0 = all)")
    parser.add_argument("--probe", action="store_true",
                        help="score removing each extra local in a disposable TU copy")
    parser.add_argument("--jobs", type=int, default=2, help="parallel probe compiles (1..4)")
    parser.add_argument("--json", action="store_true", help="machine-readable output")
