"""Enumerate the assemblies one unchanged function body compiles into across
real compilation contexts (docs/vc6/context-variants.md).

The unit's front end runs once (/d1il). Every variant replays the same IL
through the trace shim with one context channel set to a value that some
real context produces. No C2 decision is overridden: the shim only changes
the inputs C2 decides from.

Channels (each measured on the CUR != MAX ground-truth set):

* ``phase``: C2's per-function phase flag at 0x9f120 is not reset before
  the next function's early passes read it (0x5b11). The previous
  function leaves 0 or 1; both occur in real units.
* ``root-cost``: the function's own IL cost record (sym+0x6d). A changed
  referenced declaration (a bool return, a member type) changes it while
  the body's text stays identical. Its /Ob2 root budget is 2*cost, clamped.
* ``callee-cost``: each inline candidate's cost record. Editing a callee's
  source changes its cost and so every caller's expand/keep test.

Pure symbol-handle offsets (typedef padding before the function or at the
top of the unit) changed no byte on the measured cases. They are not swept.
"""
from __future__ import annotations

import collections
import contextlib
import hashlib
import json
import re
import shutil
import struct
import sys
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

from homm3.core import cc_wrap
from homm3.core.project import Project
from homm3.sema import _asm
from homm3.vc6 import _common, _selection, _unit, il as _ilmod, inline_force
from homm3.vc6.shim import build

VARIANT_ROOT = _common.REPO / "build/vc6/variants"
PHASE_RVA = 0x9f120
FREE_COST = 40
# Measured dependency edits moved a cost by 2..6 IL units (getMapItem 39/41,
# getHero 41/45, getCurrHero 46/50, the root of victorylossconditions
# 913/910). A window keeps the sweep to costs a nearby source spelling can
# produce instead of arbitrary records.
COST_WINDOW = 12
ROOT_WINDOW = 24
# A callee whose definition is not visible in the unit (defined in another
# source file) is never an inline candidate. The cost record emulates that
# with a cost no budget admits. Compiler-library templates stay visible.
HIDDEN_COST = 32000


def hideable(name: str) -> bool:
    return "@std@@" not in name


# --------------------------------------------------------------------------
# COFF: one function's bytes with relocation fields masked
# --------------------------------------------------------------------------

def function_bytes(obj: Path, symbol: str) -> bytes | None:
    """The function's code with relocated dwords zeroed and trailing
    alignment padding removed; None when the object does not emit it."""
    b = obj.read_bytes()
    nsec, _, symptr, nsym = struct.unpack_from("<HIII", b, 2)
    optsz = struct.unpack_from("<H", b, 16)[0]
    strtab = symptr + nsym * 18
    sections = []
    for s in range(nsec):
        h = 20 + optsz + s * 40
        size, ptr, rel, _, nrel = struct.unpack_from("<IIIIH", b, h + 16)
        data = bytearray(b[ptr:ptr + size]) if ptr else bytearray(size)
        for r in range(nrel):
            va = struct.unpack_from("<I", b, rel + r * 10)[0]
            data[va:va + 4] = b"\0\0\0\0"
        sections.append(data)
    starts = collections.defaultdict(list)
    target = None
    i = 0
    while i < nsym:
        e = b[symptr + i * 18:symptr + i * 18 + 18]
        if e[:4] == b"\0\0\0\0":
            off = strtab + struct.unpack_from("<I", e, 4)[0]
            name = b[off:b.index(b"\0", off)].decode("latin1")
        else:
            name = e[:8].rstrip(b"\0").decode("latin1")
        value, sec, typ, cls, naux = struct.unpack_from("<IhHBB", e, 8)
        if sec > 0 and typ == 0x20 and cls in (2, 3):
            starts[sec - 1].append(value)
            if name == symbol:
                target = (sec - 1, value)
        i += 1 + naux
    if target is None:
        return None
    sec, value = target
    later = [v for v in starts[sec] if v > value]
    end = min(later) if later else len(sections[sec])
    return bytes(sections[sec][value:end]).rstrip(b"\x90\xcc")


# --------------------------------------------------------------------------
# replay with context channels
# --------------------------------------------------------------------------

@dataclass
class Context:
    """One point in channel space; empty means the captured context."""
    phase: int | None = None
    root_cost: int | None = None
    callee_cost: dict = field(default_factory=dict)

    def label(self) -> str:
        parts = []
        if self.phase is not None:
            parts.append(f"phase={self.phase}")
        if self.root_cost is not None:
            parts.append(f"root-cost={self.root_cost}")
        parts += [f"hidden[{name}]" if cost == HIDDEN_COST else f"cost[{name}]={cost}"
                  for name, cost in sorted(self.callee_cost.items())]
        return ", ".join(parts) or "captured context"

    def key(self) -> str:
        return json.dumps([self.phase, self.root_cost, sorted(self.callee_cost.items())])


@dataclass
class Replayer:
    unit: str
    symbol: str
    workdir: Path
    source: Path
    flags: list[str]
    streams: dict = field(repr=False)
    runs: int = 0

    def run(self, ctx: Context, slot: int = 0) -> tuple[bytes | None, str, str]:
        """(masked function bytes, objdump text, trace log) under *ctx*."""
        work = self.workdir / f"slot{slot}"
        work.mkdir(parents=True, exist_ok=True)
        out = work / "compiled.obj"
        out.unlink(missing_ok=True)
        log = work / "trace.log"
        log.write_text("")
        env = {"MSVC_DIR": str(build.OVERLAY_MSVC),
               "HOMM3_VC6_INLINE_TRACE": self.symbol,
               "HOMM3_VC6_SHIM_LOG": cc_wrap.winepath_w(log)}
        if ctx.phase is not None:
            patch = work / "carry.bin"
            patch.write_bytes(struct.pack("<II", PHASE_RVA, ctx.phase))
            env["HOMM3_VC6_STATE_PATCH"] = cc_wrap.winepath_w(patch)
        costs = dict(ctx.callee_cost)
        if ctx.root_cost is not None:
            costs["*root*"] = ctx.root_cost
        if costs:
            env["HOMM3_VC6_CB_SET"] = ";".join(f"{k}={v}" for k, v in costs.items())
        process = build._traceReplay(out, self.source, self.flags, self.streams, env)
        self.runs += 1
        if process.returncode or not out.is_file():
            return None, "", log.read_text(encoding="latin1")
        code = function_bytes(out, self.symbol)
        text = _asm.objdump(out, self.symbol, 0) if code is not None else ""
        return code, text, log.read_text(encoding="latin1")


def parse_main(log: str, symbol: str) -> dict:
    roots = inline_force._root_addresses(log, symbol)
    for line in log.splitlines():
        if line.startswith("main ") and line.split()[1] in roots:
            fields = dict(word.split("=", 1) for word in line.split()[2:])
            return {"cost": int(fields["cb"]), "phase": int(fields.get("phase", -1))}
    return {}


def callee_points(sites: list[dict], limit: int) -> list[tuple[str, int]]:
    """Cost values at which some recorded site would decide the other way,
    nearest margin first: the free threshold and each site's budget edge."""
    points = {}
    for site in sites:
        name, cost, budget = site["callee"], site["cb"], site["budget"]
        candidates = {FREE_COST, FREE_COST + 1, budget, budget + 1}
        for value in candidates:
            if value <= 0 or value == cost or abs(value - cost) > COST_WINDOW:
                continue
            # the decision at this site changes only if the value crosses it
            crosses = ((value <= FREE_COST) != (cost <= FREE_COST)
                       or (value <= budget) != (cost <= budget))
            if crosses:
                margin = abs(value - cost)
                key = (name, value)
                points[key] = min(points.get(key, margin), margin)
    return [k for k, _ in sorted(points.items(), key=lambda kv: (kv[1], kv[0]))][:limit]


@dataclass
class Variant:
    code: bytes
    text: str
    contexts: list[str]


def enumerate_variants(replayer: Replayer, *, max_replays: int = 80, jobs: int = 4,
                       root_span: int = ROOT_WINDOW) -> dict:
    variants: dict[bytes, Variant] = {}
    seen: dict[str, bytes | None] = {}
    log_by_context: dict[str, str] = {}

    def record(ctx: Context, result) -> bytes | None:
        code, text, log = result
        seen[ctx.key()] = code
        log_by_context[ctx.key()] = log
        if code is not None:
            variants.setdefault(code, Variant(code, text, [])).contexts.append(ctx.label())
        return code

    def batch(contexts: list[Context]) -> list[bytes | None]:
        todo = [c for c in contexts if c.key() not in seen][:max(0, max_replays - replayer.runs)]
        with ThreadPoolExecutor(max_workers=jobs) as pool:
            results = list(pool.map(lambda ic: replayer.run(ic[1], ic[0]), enumerate(todo)))
        for ctx, result in zip(todo, results):
            record(ctx, result)
        return [seen.get(c.key()) for c in contexts]

    base = Context()
    record(base, replayer.run(base))
    log = log_by_context[base.key()]
    main = parse_main(log, replayer.symbol)
    sites = inline_force.parse_sites(log, replayer.symbol)
    if not main:
        return dict(error="the function never reached C2's inliner (no main record)",
                    variants=variants, main=main, sites=0)
    phases = [0, 1] if main.get("phase") in (0, 1) else [None]
    other_phase = [Context(phase=1 - main["phase"])] if main.get("phase") in (0, 1) else []
    batch(other_phase)

    # root cost: a grid, refined wherever neighbouring outputs differ
    cost = main["cost"]
    lo, hi = max(1, cost - root_span), cost + root_span
    grid = sorted(set(range(lo, hi + 1, 4)) | {cost})
    batch([Context(root_cost=v) for v in grid if v != cost])
    def root_out(v):
        return seen.get((Context() if v == cost else Context(root_cost=v)).key())
    pending = [(a, b) for a, b in zip(grid, grid[1:]) if root_out(a) != root_out(b)]
    while pending and replayer.runs < max_replays:
        mids = [(a + b) // 2 for a, b in pending if b - a > 1]
        if not mids:
            break
        batch([Context(root_cost=m) for m in mids if m != cost])
        nxt = []
        for a, b in pending:
            if b - a <= 1:
                continue
            m = (a + b) // 2
            if root_out(a) != root_out(m):
                nxt.append((a, m))
            if root_out(m) != root_out(b):
                nxt.append((m, b))
        pending = nxt

    # callee costs: values that flip some recorded site, nearest first
    points = callee_points(sites, limit=max(0, (max_replays - replayer.runs) // 2))
    batch([Context(callee_cost={name: value}) for name, value in points])

    # visibility: each project callee the captured context expands, with its
    # definition outside the unit
    admitted = sorted({site["callee"] for site in sites if site["admitted"] and hideable(site["callee"])})
    batch([Context(callee_cost={name: HIDDEN_COST}) for name in admitted])

    # pairs: each callee cost that changed the output, with each root-cost
    # boundary value that did (a dependency edit usually moves both)
    base_code = seen.get(base.key())
    callee_moves = [(n, v) for n, v in points
                    if seen.get(Context(callee_cost={n: v}).key()) not in (None, base_code)]
    root_moves = sorted({v for v in range(lo, hi + 1)
                         if Context(root_cost=v).key() in seen and root_out(v) not in (None, base_code)})
    batch([Context(root_cost=r, callee_cost={n: v}) for n, v in callee_moves for r in root_moves])

    # the other phase combined with every distinct single-channel result
    if other_phase and seen.get(other_phase[0].key()) != base_code:
        flipped = 1 - main["phase"]
        batch([Context(phase=flipped, root_cost=v) for v in root_moves]
              + [Context(phase=flipped, callee_cost={n: v}) for n, v in callee_moves])

    return dict(variants=variants, main=main, sites=len(sites), replays=replayer.runs)


# --------------------------------------------------------------------------
# declaration-offset channel: k more symbols declared before the definition
# --------------------------------------------------------------------------

DECL_PERIOD = 64


def _include_path() -> str:
    roots = [cc_wrap.msvc_dir() / "include",
             *[p for p in Project(_common.REPO).includes if p.is_dir()]]
    return ";".join(cc_wrap.winepath_w(p) for p in roots)


def padded_source(text: str, va: int, k: int, place: str) -> str | None:
    """The unit with k handle-consuming declarations (`typedef int`, cost 1
    each in handle-order.md's table) placed right before the function's
    annotation block (place "before") or at the top of the unit ("top")."""
    pad = "".join(f"typedef int h3ctx_decl{i};\n" for i in range(k))
    if place == "top":
        return pad + text
    match = re.search(rf"^VA\(0x{va:08x}\b", text, re.M | re.I)
    if match is None:
        return None
    return text[:match.start()] + pad + text[match.start():]


def compile_unit_text(replayer: "Replayer", text: str, work: Path) -> Path | None:
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    copy = work / replayer.source.name
    copy.write_text(text, encoding="latin1")
    include = _include_path() + ";" + cc_wrap.winepath_w(replayer.source.parent)
    _ilmod._wine_cl([*replayer.flags, f"/Fo{copy.stem}.obj", copy.name], work, include)
    obj = work / f"{copy.stem}.obj"
    return obj if obj.is_file() else None


def capture_unit_text(replayer: "Replayer", text: str, work: Path) -> dict | None:
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    copy = work / replayer.source.name
    copy.write_text(text, encoding="latin1")
    include = _include_path() + ";" + cc_wrap.winepath_w(replayer.source.parent)
    cap = _ilmod.capture(copy, replayer.flags, work / "il", include=include)
    return {suffix: path.read_bytes() for suffix, path in cap.items()}


def declaration_offsets(replayer: "Replayer", va: int, jobs: int) -> dict:
    """{(place, k): (code, text)} for k = 1..63 at both placements."""
    text = replayer.source.read_text(encoding="latin1")
    points = [(place, k) for place in ("before", "top") for k in range(1, DECL_PERIOD)]

    def one(point):
        place, k = point
        source = padded_source(text, va, k, place)
        if source is None:
            return point, None
        work = replayer.workdir / "decl" / f"{place}{k:02d}"
        obj = compile_unit_text(replayer, source, work)
        if obj is None:
            return point, None
        code = function_bytes(obj, replayer.symbol)
        body = _asm.objdump(obj, replayer.symbol, 0) if code is not None else ""
        shutil.rmtree(work, ignore_errors=True)
        return point, (code, body)

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        return dict(pool.map(one, points))


# --------------------------------------------------------------------------
# entry points
# --------------------------------------------------------------------------

def open_replayer(selector: str, streams: dict | None = None) -> tuple[Replayer, object]:
    selected = _selection.retail(selector)
    source, flags = _unit.source_for_unit(selected.unit), _unit.flags_for_unit(selected.unit)
    if source is None or flags is None:
        _common.die(f"unknown unit/profile {selected.unit!r}")
    tag = hashlib.sha256(selected.name.encode()).hexdigest()[:12]
    workdir = (VARIANT_ROOT / selected.unit / tag).resolve()
    workdir.mkdir(parents=True, exist_ok=True)
    if streams is None:
        streams = inline_force.capture_unit(selected.unit)
    return Replayer(selected.unit, selected.name, workdir, Path(source).resolve(),
                    list(flags), streams), selected


def classify(selector: str, *, streams: dict | None = None, max_replays: int = 80,
             jobs: int = 4, against: Path | None = None, decl: bool = True,
             decl_classes: int = 4) -> dict:
    replayer, selected = open_replayer(selector, streams)
    # Inertness: the shim with no channel set must reproduce the plain replay.
    plain = replayer.workdir / "plain" / "compiled.obj"
    plain.parent.mkdir(parents=True, exist_ok=True)
    plain.unlink(missing_ok=True)
    process = build._traceReplay(plain, replayer.source, replayer.flags, replayer.streams, None)
    if process.returncode or not plain.is_file():
        raise RuntimeError("plain replay failed:\n" + build._tail(process))
    reference = function_bytes(plain, selected.name)
    result = enumerate_variants(replayer, max_replays=max_replays, jobs=jobs)
    variants = result["variants"]
    if "error" not in result and decl:
        # Declaration offsets are swept as real front-end contexts; each new
        # class then gets its own phase/cost sweep on that context's IL.
        va = selected.rva + 0x400000
        offsets = declaration_offsets(replayer, va, jobs)
        fresh = {}
        for (place, k), value in sorted(offsets.items(), key=lambda kv: (kv[0][1], kv[0][0])):
            if value is None or value[0] is None:
                continue
            code, body = value
            label = f"decls-{place}={k}"
            if code not in variants:
                fresh.setdefault(code, (place, k))
            variants.setdefault(code, Variant(code, body, [])).contexts.append(label)
        for code, (place, k) in list(fresh.items())[:decl_classes]:
            text = padded_source(replayer.source.read_text(encoding="latin1"), va, k, place)
            streams = capture_unit_text(replayer, text, replayer.workdir / "decl" / f"cap-{place}{k}")
            sub = Replayer(replayer.unit, replayer.symbol, replayer.workdir / f"ctx-{place}{k}",
                           replayer.source, replayer.flags, streams)
            inner = enumerate_variants(sub, max_replays=max(8, max_replays // 2), jobs=jobs)
            for inner_code, variant in inner["variants"].items():
                labels = [f"decls-{place}={k}; {c}" for c in variant.contexts]
                variants.setdefault(inner_code, Variant(inner_code, variant.text, [])).contexts.extend(labels)
            replayer.runs += sub.runs
    captured = next((v.code for v in variants.values() if "captured context" in v.contexts), None)
    if captured != reference:
        raise RuntimeError("trace shim is not inert: the captured-context replay differs "
                           "from the plain replay")
    report = dict(selector=selector, unit=selected.unit, symbol=selected.name,
                  replays=replayer.runs, sites=result.get("sites"), main=result.get("main"),
                  distinct=len(variants))
    if "error" in result:
        # No inline candidate means no root/callee cost decision and no
        # inliner entry at which to observe or set the phase flag.
        report["verdict"] = "not swept: no inliner entry (no cost or phase channel)"
        return report
    retail_text, retail_label = _selection.reference_text(selector)
    retail = inline_force.strict_stream(retail_text)
    hits = [v for v in variants.values() if inline_force.strict_stream(v.text) == retail]
    best = max(variants.values(), default=None,
               key=lambda v: inline_force.similarity(inline_force.strict_stream(v.text), retail))
    base_code = next((v for v in variants.values() if "captured context" in v.contexts), None)
    report["retail"] = retail_label
    report["captured_strict"] = bool(base_code and inline_force.strict_stream(base_code.text) == retail)
    report["retail_in_set"] = bool(hits)
    report["retail_contexts"] = hits[0].contexts[:6] if hits else []
    if best is not None:
        report["best_similarity"] = round(inline_force.similarity(
            inline_force.strict_stream(best.text), retail), 6)
        report["best_contexts"] = best.contexts[:4]
    report["variants"] = [dict(contexts=v.contexts[:6], n=len(v.contexts),
                               sha=hashlib.sha1(v.code).hexdigest()[:12]) for v in variants.values()]
    if against is not None:
        target = function_bytes(against, selected.name)
        report["against"] = str(against)
        report["against_in_set"] = target is not None and target in variants
        report["against_contexts"] = variants[target].contexts[:6] if report["against_in_set"] else []
    if report["captured_strict"]:
        report["verdict"] = "exact in the captured context"
    elif hits:
        report["verdict"] = "body correct: retail is reachable in a real context"
    else:
        report["verdict"] = "not reachable by context channels: source differs"
    (replayer.workdir / "variants.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def _print(report: dict) -> None:
    print(f"[variants] {report['symbol']} ({report['unit']}): {report.get('distinct')} "
          f"distinct assemblies from {report.get('replays')} replays")
    for row in report.get("variants", []):
        print(f"   {row['sha']}  x{row['n']:<3} {'; '.join(row['contexts'][:3])}")
    if "against" in report:
        print(f"[against] {report['against']}: {'IN SET' if report['against_in_set'] else 'absent'}"
              f" {report.get('against_contexts', [])[:2]}")
    print(f"[retail]  {'IN SET via ' + '; '.join(report['retail_contexts'][:2]) if report.get('retail_in_set') else 'absent'}"
          f" (best strict similarity {report.get('best_similarity')})")
    print(f"[verdict] {report['verdict']}")


@contextlib.contextmanager
def _shim():
    with inline_force.forcing_shim():
        yield


def run(args) -> int:
    with _shim():
        report = classify(args.target, max_replays=args.max_replays, jobs=args.jobs,
                          against=Path(args.against).resolve() if args.against else None,
                          decl_classes=args.decl_classes)
    _print(report)
    return 0


def run_all(args) -> int:
    selectors = []
    for line in Path(args.list).read_text().splitlines():
        fields = line.split("\t")
        if not fields or not fields[0].startswith("0x"):
            continue
        if len(fields) > 1 and (fields[1].startswith("rmg") or fields[1] == "zlib"):
            continue
        selectors.append(fields[0])
    if args.limit:
        selectors = selectors[:args.limit]
    units = {}
    for selector in selectors:
        try:
            units[selector] = _selection.retail(selector).unit
        except SystemExit:
            continue
    out = VARIANT_ROOT / "variants-all.jsonl"
    out.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    with _shim(), out.open("w") as sink:
        streams = {u: inline_force.capture_unit(u) for u in sorted(set(units.values()))}

        def one(selector):
            try:
                return classify(selector, streams=streams[units[selector]],
                                max_replays=args.max_replays, jobs=args.inner_jobs,
                                decl_classes=args.decl_classes)
            except (Exception, SystemExit) as error:
                return dict(selector=selector, unit=units[selector],
                            verdict=f"error: {str(error).splitlines()[0][:160] if str(error) else type(error).__name__}")

        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            for report in pool.map(one, list(units)):
                rows.append(report)
                sink.write(json.dumps(report) + "\n")
                sink.flush()
                print(f"[variants] {report.get('selector')} {report.get('unit')}: "
                      f"{report.get('distinct')} -> {report.get('verdict')}", file=sys.stderr)
    counts = collections.Counter(r["verdict"].split(":")[0] if r["verdict"].startswith("error")
                                 else r["verdict"] for r in rows)
    summary = VARIANT_ROOT / "variants-all.md"
    lines = ["# Context-variant classification", "", "| verdict | functions |", "| --- | ---: |"]
    lines += [f"| {k} | {v} |" for k, v in counts.most_common()]
    lines += ["", "| VA | unit | distinct | best | verdict | retail contexts |",
              "| --- | --- | ---: | ---: | --- | --- |"]
    for r in rows:
        lines.append(f"| {r.get('selector')} | {r.get('unit')} | {r.get('distinct', '')} | "
                     f"{r.get('best_similarity', '')} | {r['verdict']} | "
                     f"{'; '.join(r.get('retail_contexts', [])[:2])} |")
    summary.write_text("\n".join(lines) + "\n")
    print(summary.read_text())
    return 0
