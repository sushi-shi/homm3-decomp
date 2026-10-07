"""Force C2's per-site inline decisions over one frozen IL capture.

`homm3 vc6 force` and `homm3 vc6 reach` never change source. The unit's
front end runs once (`/d1il`); every back-end run replays fresh copies of the
same four IL streams (`/d2il`), so declarations, TU state and handle numbering
are fixed. The inline-trace shim then overrides chosen budget comparisons:
`E` takes C2's admitted path (rva 0x19faf), `K` its rejection path (0x19a94).
An empty rule list must reproduce the plain replay byte for byte; `force`
refuses to report anything else (the inertness gate).

`reach` reads retail's observable call stream for the function, forces the
missing/extra expansions site by site, and iterates until the instruction
stream equals retail's or no rule changes anything. A function that reaches
retail this way is an *inline-state* wall: its remaining source target is the
budget/cost delta that produces those decisions without forcing. The rules
and per-site budgets are written under build/vc6/force/<unit>/<tag>/.
See docs/vc6/decision-forcing.md.
"""
from __future__ import annotations

import contextlib
import difflib
import hashlib
import json
import re
from dataclasses import dataclass, field
from pathlib import Path
import sys

from homm3.core import cc_wrap
from homm3.sema import _asm
from homm3.vc6 import _common, _selection, _unit, inline_model, inline_trace
from homm3.vc6.shim import build

FORCE_ROOT = _common.REPO / "build/vc6/force"


@dataclass
class Rule:
    owner: str
    callee: str
    occurrence: int
    action: str  # "E" expand, "K" keep a call

    def line(self) -> str:
        if self.action not in ("E", "K"):
            raise ValueError(f"bad forcing action {self.action!r}")
        fields = []
        for text, limit in ((self.owner, 95), (self.callee, 159)):
            if "\t" in text or "\n" in text:
                raise ValueError(f"unusable rule field {text!r}")
            # The shim matches substrings; a prefix of a long mangled name
            # keeps its identity while fitting the shim's fixed buffers.
            fields.append(text.encode("latin1", "replace")[:limit].decode("latin1") or "*")
        return f"{fields[0]}\t{fields[1]}\t{self.occurrence}\t{self.action}"


def parse_sites(text: str, symbol: str) -> list[dict]:
    """Site records of the selected root, keeping owner and any forced action."""
    names = {}
    for line in text.splitlines():
        if line.startswith("sym "):
            _, address, name = line.split(" ", 2)
            names[address] = name
    roots = {a for a, n in names.items() if n == symbol}
    sites = []
    for line in text.splitlines():
        if not line.startswith("site "):
            continue
        fields = dict(word.split("=", 1) for word in line.split()[1:])
        if fields["root"] not in roots:
            continue
        sites.append(dict(owner=names.get(fields["owner"], fields["owner"]),
                          callee=names.get(fields["callee"], fields["callee"]),
                          cb=int(fields["cb"]), budget=int(fields["budget"]),
                          depth=int(fields["depth"]), remain=int(fields["remain"]),
                          force=fields.get("force", "")))
    occurrence: dict[tuple[str, str], int] = {}
    for site in sites:
        key = (site["owner"], site["callee"])
        occurrence[key] = occurrence.get(key, 0) + 1
        site["occurrence"] = occurrence[key]
        site["admitted"] = (site["force"] == "E" or
                            (site["force"] != "K" and
                             (site["cb"] <= 40 or site["budget"] >= site["cb"])))
    return sites


REGISTER_NAMES = {1: "eax", 2: "ecx", 3: "edx", 4: "ebx", 6: "ebp", 7: "esi", 8: "edi"}


def parse_colors(text: str, symbol: str) -> list[dict]:
    """Global coloring decisions (0x24748) of the selected root, in order."""
    names = {}
    for line in text.splitlines():
        if line.startswith("sym "):
            _, address, name = line.split(" ", 2)
            names[address] = name
    roots = {a for a, n in names.items() if n == symbol}
    rows = []
    for line in text.splitlines():
        if not line.startswith("color "):
            continue
        fields = dict(word.split("=", 1) for word in line.split()[1:])
        if fields["root"] not in roots:
            continue
        mask = int(fields["eligible"], 16)
        rows.append(dict(k=int(fields["k"]), chosen=int(fields["chosen"]),
                         eligible=[r for r in range(1, 9) if mask >> r & 1],
                         priority=int(fields["priority"]),
                         forced=int(fields.get("forced", 0))))
    return rows


def instruction_stream(text: str) -> list[str]:
    """Masked instructions: absolute operands masked, displacements masked."""
    return [row for row in _asm.norm(text) if not row.startswith("reloc ")]


def strict_stream(text: str) -> list[str]:
    """Instructions keeping stack displacements, immediates and registers;
    only absolute addresses and branch/call targets are masked."""
    rows = []
    for _offset, insn in _asm.code_insns(text):
        row = _asm.mask_insn(insn)
        # Switch-table addends are section-relative relocations: the
        # delinked target and a fresh object place the table differently.
        row = re.sub(r"^(jmp dword ptr \[4\*\w+)(?: \+ 0x[0-9a-f]+)?\]$", r"\1 + <table>]", row)
        rows.append(row)
    while rows and rows[-1] == "nop":
        rows.pop()
    return rows


def similarity(a: list[str], b: list[str]) -> float:
    return difflib.SequenceMatcher(None, a, b, autojunk=False).ratio()


@dataclass
class Session:
    """One frozen front-end capture of a unit plus its plain-replay object."""
    unit: str
    symbol: str
    workdir: Path
    source: Path
    flags: list[str]
    streams: dict = field(repr=False, default_factory=dict)
    last_colors: list = field(repr=False, default_factory=list)
    reference: bytes = field(repr=False, default=b"")
    runs: int = 0

    @classmethod
    def open(cls, unit: str, symbol: str, streams: dict | None = None) -> "Session":
        """Per-function workdir; *streams* may be a unit capture shared by
        several sessions. The reference object is always replayed in this
        session's own workdir: /Z7 records the object path."""
        source, flags = _unit.source_for_unit(unit), _unit.flags_for_unit(unit)
        if source is None or flags is None:
            _common.die(f"unknown unit/profile {unit!r}")
        tag = hashlib.sha256(symbol.encode()).hexdigest()[:12]
        workdir = (FORCE_ROOT / unit / tag).resolve()
        workdir.mkdir(parents=True, exist_ok=True)
        session = cls(unit, symbol, workdir, Path(source).resolve(), list(flags))
        session.streams = streams if streams is not None else capture_unit(unit)
        session.reference = session._replay(None)
        return session

    def _replay(self, rules: list[Rule] | None, registers: dict[int, int] | None = None) -> bytes:
        # /Z7 records the object path: every replay writes the same file.
        compiled = self.workdir / "compiled.obj"
        compiled.unlink(missing_ok=True)
        extra = None
        if rules is not None:
            spec = self.workdir / "rules.tsv"
            spec.write_text("".join(rule.line() + "\n" for rule in rules))
            log = self.workdir / "sites.log"
            log.write_text("")
            extra = {"MSVC_DIR": str(build.OVERLAY_MSVC),
                     "HOMM3_VC6_INLINE_TRACE": self.symbol,
                     "HOMM3_VC6_INLINE_FORCE": cc_wrap.winepath_w(spec),
                     "HOMM3_VC6_SHIM_LOG": cc_wrap.winepath_w(log)}
            if registers:
                extra["HOMM3_VC6_REG_FORCE"] = ",".join(
                    f"{k}:{reg}" for k, reg in sorted(registers.items()))
        process = build._traceReplay(compiled, self.source, self.flags, self.streams, extra)
        if process.returncode or not compiled.is_file():
            raise RuntimeError("C2 replay failed:\n" + build._tail(process))
        self.runs += 1
        return compiled.read_bytes()

    def force(self, rules: list[Rule], registers: dict[int, int] | None = None
              ) -> tuple[bytes, list[dict]]:
        data = self._replay(rules, registers)
        log = (self.workdir / "sites.log").read_text(encoding="latin1")
        self.last_colors = parse_colors(log, self.symbol)
        return data, parse_sites(log, self.symbol)

    def text(self, data: bytes) -> str:
        path = self.workdir / "probe.obj"
        path.write_bytes(data)
        return _asm.objdump(path, self.symbol, 0)


def register_reach(selector: str, *, rules: list[Rule] | None = None,
                   passes: int = 2, streams: dict | None = None,
                   install_shim: bool = True, max_trials: int = 0) -> dict:
    """Greedy search over C2's own legal global register choices.

    Every alternative is a register C2 itself found eligible for that live-range
    group; no instruction is invented. Decisions are taken in C2's priority
    order; at each one every eligible register is tried with the earlier
    choices fixed, and the most retail-like object is kept."""
    selected = _selection.retail(selector)
    retail_text, retail_label = _selection.reference_text(selector)
    retail_strict = strict_stream(retail_text)
    session = Session.open(selected.unit, selected.name, streams)
    if rules is None:
        rules = _saved_inline_rules(session.workdir)
    trials = []
    with (forcing_shim() if install_shim else contextlib.nullcontext()):
        gate = gate_inert(session)
        data, _ = session.force(rules)
        base_score = similarity(strict_stream(session.text(data)), retail_strict)
        colors = session.last_colors
        fixed: dict[int, int] = {}
        best_score, best_data = base_score, data
        for _ in range(passes):
            improved = False
            for decision in colors:
                k = decision["k"]
                current = fixed.get(k, decision["chosen"])
                for reg in decision["eligible"]:
                    if reg == current or not decision["chosen"]:
                        continue
                    if max_trials and len(trials) >= max_trials:
                        break
                    trial = {**fixed, k: reg}
                    data, _ = session.force(rules, trial)
                    score = similarity(strict_stream(session.text(data)), retail_strict)
                    trials.append(dict(k=k, reg=reg, score=round(score, 6)))
                    if score > best_score + 1e-12:
                        best_score, best_data, fixed = score, data, trial
                        current, improved = reg, True
                if best_score == 1.0:
                    break
            if not improved or best_score == 1.0:
                break
    strict = strict_stream(session.text(best_data)) == retail_strict
    report = dict(selector=selector, unit=selected.unit, symbol=selected.name,
                  retail=retail_label, inert_gate=gate, inline_rules=[r.line() for r in rules],
                  base_similarity=round(base_score, 6), best_similarity=round(best_score, 6),
                  strict=strict, decisions=colors,
                  forced={str(k): REGISTER_NAMES.get(v, v) for k, v in sorted(fixed.items())},
                  trials=len(trials), replays=session.runs, workdir=str(session.workdir))
    report["verdict"] = ("regalloc-state: retail reached by forcing global register choices"
                         if strict and fixed else
                         "regalloc-state partial" if best_score > base_score else
                         "not global-register state")
    (session.workdir / "register-reach.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def _parse_rule_line(line: str) -> Rule:
    owner, callee, occurrence, action = line.split("\t")
    return Rule("" if owner == "*" else owner, "" if callee == "*" else callee,
                int(occurrence), action)


def _saved_inline_rules(workdir: Path) -> list[Rule]:
    """The rules a previous `reach` of the same source hash settled on."""
    path = workdir / "reach.json"
    if not path.exists():
        return []
    best = json.loads(path.read_text()).get("best") or {}
    return [_parse_rule_line(line) for line in best.get("rules", [])]


def register_reach_many(selectors: list[str], *, jobs: int = 4, passes: int = 2,
                        max_trials: int = 0, output: Path | None = None) -> list[dict]:
    from concurrent.futures import ThreadPoolExecutor

    by_unit: dict[str, list[str]] = {}
    for selector in selectors:
        try:
            by_unit.setdefault(_selection.retail(selector).unit, []).append(selector)
        except SystemExit:
            continue
    results = []
    output = output or (FORCE_ROOT / "reg-reach-all.jsonl")
    with forcing_shim(), output.open("w") as sink:
        for unit, members in sorted(by_unit.items()):
            streams = capture_unit(unit)

            def one(selector, streams=streams, unit=unit):
                try:
                    return register_reach(selector, passes=passes, streams=streams,
                                          install_shim=False, max_trials=max_trials)
                except (Exception, SystemExit) as error:
                    return dict(selector=selector, unit=unit,
                                verdict=f"error: {str(error).splitlines()[0][:160] if str(error) else type(error).__name__}")

            with ThreadPoolExecutor(max_workers=jobs) as pool:
                for report in pool.map(one, members):
                    slim = {k: v for k, v in report.items() if k != "decisions"}
                    results.append(slim)
                    sink.write(json.dumps(slim) + "\n")
                    sink.flush()
                    print(f"[reg-reach] {slim.get('selector')} {unit}: {slim.get('verdict')} "
                          f"{slim.get('base_similarity')} -> {slim.get('best_similarity')}",
                          file=sys.stderr)
    return results


def run_register_reach_all(args) -> int:
    selectors = [line.split("\t")[0] for line in Path(args.list).read_text().splitlines()
                 if line.startswith("0x")]
    results = register_reach_many(selectors, jobs=args.jobs, passes=args.passes,
                                  max_trials=args.max_trials)
    lines = ["| VA | unit | base | best | forced | verdict |", "| --- | --- | ---: | ---: | --- | --- |"]
    for row in results:
        lines.append(f"| {row.get('selector')} | {row.get('unit')} | {row.get('base_similarity', 0):.4f} "
                     f"| {row.get('best_similarity', 0):.4f} | {row.get('forced', '')} | {row.get('verdict')} |")
    summary = FORCE_ROOT / "reg-reach-all.md"
    summary.write_text("# Register-forcing reachability\n\n" + "\n".join(lines) + "\n")
    print(summary.read_text())
    return 0


def run_register_reach(args) -> int:
    rules = None  # reuse a saved `reach` result for this source hash
    if args.with_inline:
        inline = reach(args.target, iterations=args.iterations)
        rules = [_parse_rule_line(line) for line in (inline.get("best") or {}).get("rules", [])]
    report = register_reach(args.target, rules=rules, passes=args.passes)
    print(f"[reg-reach] {report['symbol']} ({report['unit']}) vs {report['retail']}")
    print(f"[gate]  empty rules reproduce the plain replay")
    for row in report["decisions"]:
        names = ",".join(REGISTER_NAMES.get(r, str(r)) for r in row["eligible"])
        print(f"   k={row['k']:>3} prio={row['priority']:>6} chose "
              f"{REGISTER_NAMES.get(row['chosen'], row['chosen'])} of {{{names}}}")
    print(f"[force] {report['forced'] or 'none'}  inline rules: {len(report['inline_rules'])}")
    print(f"[score] {report['base_similarity']:.4f} -> {report['best_similarity']:.4f}"
          f"{' STRICT' if report['strict'] else ''} ({report['trials']} trials)")
    print(f"[verdict] {report['verdict']}")
    return 0


def capture_unit(unit: str) -> dict:
    """One /d1il front-end capture of *unit* with its exact profile."""
    source, flags = _unit.source_for_unit(unit), _unit.flags_for_unit(unit)
    if source is None or flags is None:
        _common.die(f"unknown unit/profile {unit!r}")
    build._ensure_wine_env()
    with contextlib.redirect_stdout(sys.stderr):
        build.ensure_overlay()
    output = (FORCE_ROOT / unit / "capture").resolve()
    output.mkdir(parents=True, exist_ok=True)
    return build._traceCapture(Path(source).resolve(), list(flags), output)


@contextlib.contextmanager
def forcing_shim():
    """Install the trace/force shim for the block; always restore the clean one."""
    build._ensure_wine_env()
    with contextlib.redirect_stdout(sys.stderr):
        build.ensure_overlay()
    try:
        with contextlib.redirect_stdout(sys.stderr):
            build.compile_shim(inlineTrace=True)
        yield
    finally:
        with contextlib.redirect_stdout(sys.stderr):
            build.compile_shim()


def gate_inert(session: Session) -> dict:
    """Empty rule list: identical object outside the COFF timestamp."""
    data, sites = session.force([])
    differences = build._masked_diff(session.reference, data)
    if differences:
        raise RuntimeError(f"forcing shim is not inert: {len(differences)} byte "
                           f"differences outside the timestamp")
    # A function without inline candidates still passes: it simply has no
    # forcible decision, and is classified as such.
    return dict(object_bytes=len(data), sites=len(sites))


def derive_rules(sites: list[dict], ours_text: str, retail_text: str,
                 rules: list[Rule]) -> list[Rule]:
    """Add rules that move our per-callee call counts toward retail's."""
    divergence = inline_model.ordered_divergence(ours_text, retail_text)
    fixed = {(r.owner, r.callee, r.occurrence) for r in rules}
    added = []
    # `under`: we call more than retail -> expand kept sites (earliest first).
    for symbol, delta, _ in divergence["under"]:
        kept = [s for s in sites if s["callee"] == symbol and not s["admitted"]
                and (s["owner"], s["callee"], s["occurrence"]) not in fixed]
        for site in kept[:delta]:
            added.append(Rule(site["owner"], site["callee"], site["occurrence"], "E"))
    # `over`: retail calls more -> keep admitted sites (latest first: budgets
    # are spent in tuple order, so the last sites are the ones that lose).
    for symbol, _, delta in divergence["over"]:
        admitted = [s for s in sites if s["callee"] == symbol and s["admitted"]
                    and (s["owner"], s["callee"], s["occurrence"]) not in fixed]
        for site in admitted[::-1][:delta]:
            added.append(Rule(site["owner"], site["callee"], site["occurrence"], "K"))
    return added


def reach(selector: str, *, iterations: int = 6, streams: dict | None = None,
          install_shim: bool = True) -> dict:
    selected = _selection.retail(selector)
    unit, symbol = selected.unit, selected.name
    retail_text, retail_label = _selection.reference_text(selector)
    retail_stream = instruction_stream(retail_text)
    retail_strict = strict_stream(retail_text)
    session = Session.open(unit, symbol, streams)
    history = []
    with (forcing_shim() if install_shim else contextlib.nullcontext()):
        gate = gate_inert(session)
        base_text = session.text(session.reference)
        base_stream = instruction_stream(base_text)
        rules: list[Rule] = []
        data, sites = session.force(rules)
        best = None
        for step in range(iterations + 1):
            text = session.text(data)
            stream = instruction_stream(text)
            score = similarity(strict_stream(text), retail_strict)
            row = dict(step=step, rules=[r.line() for r in rules],
                       similarity=round(score, 6), exact=stream == retail_stream,
                       strict=strict_stream(text) == retail_strict,
                       instructions=len(stream))
            history.append(row)
            rank = (row["strict"], row["exact"], score)
            if best is None or rank > (best["strict"], best["exact"], best["similarity"]):
                best = dict(row, sites=sites)
            if row["strict"]:
                break
            added = derive_rules(sites, text, retail_text, rules)
            if not added:
                break
            rules = rules + added
            data, sites = session.force(rules)
    report = dict(selector=selector, unit=unit, symbol=symbol, retail=retail_label,
                  inert_gate=gate,
                  base=dict(similarity=round(similarity(strict_stream(base_text), retail_strict), 6),
                            exact=base_stream == retail_stream,
                            strict=strict_stream(base_text) == retail_strict,
                            instructions=len(base_stream)),
                  retail_instructions=len(retail_stream),
                  history=history, best=best, replays=session.runs,
                  workdir=str(session.workdir))
    report["verdict"] = classify(report)
    report["deficits"] = deficits(best["sites"]) if best else []
    (session.workdir / "reach.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def reach_many(selectors: list[str], *, jobs: int = 4, iterations: int = 6,
               output: Path | None = None) -> list[dict]:
    """Classify many functions: one front-end capture per unit, one shim install."""
    from concurrent.futures import ThreadPoolExecutor

    by_unit: dict[str, list[str]] = {}
    for selector in selectors:
        try:
            unit = _selection.retail(selector).unit
        except SystemExit:
            continue
        by_unit.setdefault(unit, []).append(selector)
    results = []
    output = output or (FORCE_ROOT / "reach-all.jsonl")
    output.parent.mkdir(parents=True, exist_ok=True)
    with forcing_shim(), output.open("w") as sink:
        for unit, members in sorted(by_unit.items()):
            try:
                streams = capture_unit(unit)
            except SystemExit as error:
                for selector in members:
                    row = dict(selector=selector, unit=unit, verdict=f"error: capture {error}")
                    results.append(row); sink.write(json.dumps(row) + "\n")
                continue

            def one(selector, streams=streams, unit=unit):
                try:
                    return reach(selector, iterations=iterations, streams=streams,
                                 install_shim=False)
                except (Exception, SystemExit) as error:  # keep the batch going
                    return dict(selector=selector, unit=unit,
                                verdict=f"error: {str(error).splitlines()[0][:160] if str(error) else type(error).__name__}")

            with ThreadPoolExecutor(max_workers=jobs) as pool:
                for report in pool.map(one, members):
                    slim = {k: v for k, v in report.items() if k not in ("best", "history")}
                    if report.get("best"):
                        slim["best"] = {k: v for k, v in report["best"].items() if k != "sites"}
                    results.append(slim)
                    sink.write(json.dumps(slim) + "\n")
                    sink.flush()
                    print(f"[reach] {slim.get('selector')} {slim.get('unit')}: {slim.get('verdict')}",
                          file=sys.stderr)
    return results


def deficits(sites: list[dict]) -> list[dict]:
    """Budget arithmetic each forced site needs from source (no forcing).

    E at a charged site (cb > 40) needs budget >= cb: deficit cb - budget.
    K needs budget < cb (cb > 40); a free callee (cb <= 40) cannot be kept
    by budget at all, only by a different body cost. The depth-1 budget is
    clamp(2*caller_cb, 1000, 35000) less earlier charges; a depth-n budget is
    the parent remainder divided by the sites still ahead (inliner.md)."""
    rows = []
    for site in sites:
        if not site.get("force"):
            continue
        row = dict(depth=site["depth"], callee=site["callee"], owner=site["owner"],
                   occurrence=site["occurrence"], cb=site["cb"],
                   budget=site["budget"], remain=site["remain"], action=site["force"])
        if site["force"] == "E":
            row["need"] = (f"budget +{site['cb'] - site['budget']}"
                           if site["cb"] > 40 and site["budget"] < site["cb"]
                           else "already admissible by budget (other gate)")
        else:
            row["need"] = ("callee cost above 40 (free callee)" if site["cb"] <= 40
                           else f"budget -{site['budget'] - site['cb'] + 1}"
                           if site["budget"] >= site["cb"] else "already refused by budget")
        rows.append(row)
    return rows


def classify(report: dict) -> str:
    base, best = report["base"], report["best"]
    if base["strict"]:
        return "exact"
    if best and best["strict"] and best["rules"]:
        return "inline-state: retail reached by forcing"
    if best and best["exact"] and best["rules"]:
        return "inline-state: retail instruction shape reached (stack/immediate residue)"
    if best and best["rules"] and best["similarity"] > base["similarity"] + 1e-9:
        return "inline-state partial: forcing moves toward retail"
    if not report["inert_gate"]["sites"]:
        return "not inline: no inline candidate sites"
    if not best or not best["rules"]:
        return "not inline: call decisions already agree"
    return "not inline: forcing does not move toward retail"


def _print_reach(report: dict) -> None:
    print(f"[reach] {report['symbol']} ({report['unit']})  vs {report['retail']}")
    print(f"[gate]  empty rules reproduce the plain replay "
          f"({report['inert_gate']['object_bytes']} B, "
          f"{report['inert_gate']['sites']} budget comparisons)")
    print(f"[base]  instruction similarity {report['base']['similarity']:.4f}"
          f"{' EXACT' if report['base']['exact'] else ''}")
    for row in report["history"]:
        print(f"  step {row['step']}: {len(row['rules'])} rule(s) -> "
              f"{row['similarity']:.4f}{' SHAPE' if row['exact'] else ''}"
              f"{' STRICT' if row['strict'] else ''}")
    best = report["best"]
    if best and best["rules"]:
        print("[rules] (owner \\t callee \\t occurrence \\t E|K)")
        for line in best["rules"]:
            print("   " + line)
        print("[sites] forced or budget-relevant comparisons at the best step:")
        for site in best["sites"]:
            if site["force"] or site["cb"] > 40:
                print(f"   d{site['depth']} budget={site['budget']:>6} cb={site['cb']:>4} "
                      f"rem={site['remain']:>3} {'E' if site['admitted'] else 'K'}"
                      f"{('*' + site['force']) if site['force'] else '  '} "
                      f"{site['callee'][:60]}  <- {site['owner'][:40]}")
    for row in report.get("deficits", []):
        print(f"[target] d{row['depth']} {row['action']} {row['callee'][:50]} #{row['occurrence']}"
              f" cb={row['cb']} budget={row['budget']} rem={row['remain']}: {row['need']}")
    print(f"[verdict] {report['verdict']}  ({report['replays']} C2 replays; "
          f"{report['workdir']}/reach.json)")


def run_reach(args) -> int:
    report = reach(args.target, iterations=args.iterations)
    if args.json:
        print(json.dumps({k: v for k, v in report.items() if k != "best"}, indent=2))
    else:
        _print_reach(report)
    return 0


def run_force(args) -> int:
    selected = _selection.retail(args.target)
    rules = []
    for text in args.rule or []:
        parts = text.split(",")
        if len(parts) != 4:
            _common.die(f"--rule wants OWNER,CALLEE,OCCURRENCE,E|K, got {text!r}")
        rules.append(Rule(parts[0].strip(), parts[1].strip(), int(parts[2]), parts[3].strip()))
    retail_text, label = _selection.reference_text(args.target)
    session = Session.open(selected.unit, selected.name)
    with forcing_shim():
        gate = gate_inert(session)
        data, sites = session.force(rules)
    text = session.text(data)
    stream, retail_stream = instruction_stream(text), instruction_stream(retail_text)
    base_stream = instruction_stream(session.text(session.reference))
    print(f"[force] {selected.name}: inert gate ok ({gate['sites']} comparisons)")
    for site in sites:
        print(f"   d{site['depth']} budget={site['budget']:>6} cb={site['cb']:>4} "
              f"rem={site['remain']:>3} {'E' if site['admitted'] else 'K'}"
              f"{('*' + site['force']) if site['force'] else '  '} {site['callee'][:60]}"
              f" #{site['occurrence']} <- {site['owner'][:40]}")
    print(f"[score] base {similarity(base_stream, retail_stream):.4f} -> forced "
          f"{similarity(stream, retail_stream):.4f}"
          f"{' EXACT' if stream == retail_stream else ''}")
    (session.workdir / "forced.obj").write_bytes(data)
    return 0


def _summary_markdown(results: list[dict]) -> str:
    from collections import Counter

    counts = Counter(row.get("verdict", "?").split(":")[0] if row.get("verdict", "").startswith("error")
                     else row.get("verdict", "?") for row in results)
    lines = ["# Inline-forcing reachability", "",
             "| verdict | functions |", "| --- | ---: |"]
    lines += [f"| {verdict} | {n} |" for verdict, n in counts.most_common()]
    lines += ["", "| VA | unit | base sim | best sim | rules | verdict |",
              "| --- | --- | ---: | ---: | ---: | --- |"]
    for row in results:
        base = row.get("base", {})
        best = row.get("best") or {}
        lines.append(f"| {row.get('selector')} | {row.get('unit')} | "
                     f"{base.get('similarity', 0):.4f} | {best.get('similarity', 0):.4f} | "
                     f"{len(best.get('rules', []))} | {row.get('verdict')} |")
    return "\n".join(lines) + "\n"


def run_reach_all(args) -> int:
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
    results = reach_many(selectors, jobs=args.jobs, iterations=args.iterations)
    summary = FORCE_ROOT / "reach-all.md"
    summary.write_text(_summary_markdown(results))
    print(summary.read_text())
    return 0
