"""C2/C1XX state that unrelated edits disturb: read it, set it, and compile
one translation unit 1-to-M (docs/vc6/unstable-state.md).

An edit that touches neither a function nor its callees can still change
that function's bytes, because C2 carries two inputs across the unit:

* ``phase``: the flag at C2 0x9f120 that the previous function's driver
  (0x13615) leaves behind. 0x5739 reads it at 0x5b11 before the next
  function's own driver sets it. The first function sees the initial 0.
* ``decl-offset``: the symbol-handle numbering. Every declaration before a
  point takes handles (a typedef 1, a struct 9; handle-order.md). C2's
  handle decoder 0x1c92e returns those numbers, and codegen depends on them
  with period 64.

The unit's front end runs once. Each state is a replay of the same IL
through the trace shim, which sets only those inputs: the leftover flag at
the driver's return (0x683cc), and every decoded handle plus k with the
`gl` high-water raised by k. C2 then decides everything itself.
"""
from __future__ import annotations

import collections
import contextlib
import hashlib
import json
import random
import re
import shutil
import struct
import sys
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path

from homm3.core import cc_wrap
from homm3.core.project import Project
from homm3.vc6 import _common, _il, _selection, _unit, il as _ilmod, inline_force
from homm3.vc6.shim import build

STATE_ROOT = _common.REPO / "build/vc6/unstable-state"
DECL_PERIOD = 64
PHASES = (0, 1)


# --------------------------------------------------------------------------
# state and replay
# --------------------------------------------------------------------------

@dataclass(frozen=True)
class State:
    """phase None leaves the captured leftover; offset is extra handles
    before every symbol of the unit (a global declaration offset);
    callee_order j treats, for every function, the first j of its inline
    callees (in compile order) as compiled before it (None: as captured)."""
    phase: int | None = None
    offset: int = 0
    callee_order: int | None = None

    def label(self) -> str:
        if self.phase is None and not self.offset and self.callee_order is None:
            return "captured"
        parts = []
        if self.phase is not None:
            parts.append(f"phase={self.phase}")
        if self.offset or self.callee_order is None:
            parts.append(f"decl-offset={self.offset}")
        if self.callee_order is not None:
            parts.append(f"callees-compiled-first={self.callee_order}")
        return " ".join(parts)

    def as_dict(self) -> dict:
        out = {"phase": self.phase, "decl_offset": self.offset}
        if self.callee_order is not None:
            out["callee_order"] = self.callee_order
        return out


def state_env(state: State, log: Path | None = None, spec: Path | None = None) -> dict:
    env = {"MSVC_DIR": str(build.OVERLAY_MSVC)}
    if spec is not None:
        env["HOMM3_VC6_COMPILED_SPEC"] = cc_wrap.winepath_w(spec)
    if state.phase is not None:
        env["HOMM3_VC6_PHASE"] = str(state.phase)
    if state.offset:
        env["HOMM3_VC6_HANDLE_SHIFT"] = f"1:{state.offset}"
    if log is not None:
        env["HOMM3_VC6_SHIM_LOG"] = cc_wrap.winepath_w(log)
        env["HOMM3_VC6_STATE_LOG"] = "1"
    return env


def shifted_streams(streams: dict, offset: int) -> dict:
    """The gl high-water sizes C2's handle tables; raise it with the shift."""
    if not offset:
        return streams
    gl = bytearray(streams["gl"])
    at = len(_il.GL_MAGIC)
    if bytes(gl[:at]) != _il.GL_MAGIC:
        raise RuntimeError("gl stream has no high-water header")
    struct.pack_into("<I", gl, at, struct.unpack_from("<I", gl, at)[0] + offset)
    return dict(streams, gl=bytes(gl))


@dataclass
class Unit:
    name: str
    source: Path
    flags: list
    streams: dict
    workdir: Path

    @classmethod
    def open(cls, unit: str, streams: dict | None = None) -> "Unit":
        source, flags = _unit.source_for_unit(unit), _unit.flags_for_unit(unit)
        if source is None or flags is None:
            _common.die(f"unknown unit/profile {unit!r}")
        workdir = (STATE_ROOT / unit).resolve()
        workdir.mkdir(parents=True, exist_ok=True)
        if streams is None:
            streams = inline_force.capture_unit(unit)
        return cls(unit, Path(source).resolve(), list(flags), streams, workdir)

    def replay(self, state: State, slot: str, log: bool = False,
               compiled: dict[str, list[str]] | None = None) -> tuple[Path, str]:
        work = self.workdir / slot
        work.mkdir(parents=True, exist_ok=True)
        out = work / "compiled.obj"
        out.unlink(missing_ok=True)
        log_path = work / "state.log" if log else None
        if log_path:
            log_path.write_text("")
        spec = None
        if compiled is not None:
            spec = work / "compiled.tsv"
            spec.write_text("".join("\t".join([root, *callees]) + "\n"
                                    for root, callees in compiled.items()), encoding="latin1")
        process = build._traceReplay(out, self.source, self.flags,
                                     shifted_streams(self.streams, state.offset),
                                     state_env(state, log_path, spec))
        if process.returncode or not out.is_file():
            raise RuntimeError(f"replay {state.label()} failed:\n{build._tail(process)}")
        return out, log_path.read_text(encoding="latin1") if log_path else ""

    def plain(self) -> Path:
        work = self.workdir / "plain"
        work.mkdir(parents=True, exist_ok=True)
        out = work / "compiled.obj"
        out.unlink(missing_ok=True)
        process = build._traceReplay(out, self.source, self.flags, self.streams, None)
        if process.returncode or not out.is_file():
            raise RuntimeError("plain replay failed:\n" + build._tail(process))
        return out


# --------------------------------------------------------------------------
# objects: every function's relocation-masked bytes
# --------------------------------------------------------------------------

def object_functions(obj: Path) -> dict[str, bytes]:
    """{symbol: code with relocated dwords zeroed, trailing int3/nop padding
    removed} for every external or static function symbol."""
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
    by_section = collections.defaultdict(list)
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
            by_section[sec - 1].append((value, name))
        i += 1 + naux
    out = {}
    for sec, rows in by_section.items():
        rows.sort()
        for k, (value, name) in enumerate(rows):
            end = rows[k + 1][0] if k + 1 < len(rows) else len(sections[sec])
            out[name] = bytes(sections[sec][value:end]).rstrip(b"\x90\xcc")
    return out


def digest(code: bytes) -> str:
    return hashlib.sha1(code).hexdigest()[:16]


# --------------------------------------------------------------------------
# read: what state does the captured context give each function?
# --------------------------------------------------------------------------

def parse_drivers(log: str) -> list[dict]:
    rows = []
    for line in log.splitlines():
        if not line.startswith("driver "):
            continue
        head, name = line.split(" name=", 1)
        fields = dict(word.split("=", 1) for word in head.split()[1:])
        rows.append({"name": name, "left": int(fields["left"]), "base": int(fields["base"], 16)})
    return rows


def read_state(unit: Unit) -> list[dict]:
    """Emission order; each function's received phase (what the previous
    function left; 0 for the first) and its handle base modulo 64."""
    _, log = unit.replay(State(), "read", log=True)
    return received_states(parse_drivers(log))


def received_states(rows: list[dict]) -> list[dict]:
    received = 0
    out = []
    for index, row in enumerate(rows):
        out.append({"index": index, "name": row["name"], "phase": received,
                    "handle_base": row["base"], "handle_residue": row["base"] % DECL_PERIOD})
        received = row["left"]
    return out


# --------------------------------------------------------------------------
# compile 1-to-M
# --------------------------------------------------------------------------

def callee_prefixes(order: list[str], callees: dict[str, list[str]]) -> dict[str, list[str]]:
    """{function: its inline callees with an emitted body, in compile order}."""
    index = {name: i for i, name in enumerate(order)}
    return {root: sorted((c for c in found if c in index and c != root), key=index.__getitem__)
            for root, found in callees.items()}


def compiled_round(prefixes: dict[str, list[str]], j: int) -> dict[str, list[str]]:
    return {root: ranked[:j] for root, ranked in prefixes.items() if ranked}


def states(phases=PHASES, offsets=range(DECL_PERIOD)) -> list[State]:
    return [State(p, k) for p in phases for k in offsets]


def inline_callees(unit: Unit) -> dict[str, list[str]]:
    """{root: inline candidates at any depth} from one traced replay; the
    fuzz verifier must not move these when it checks the root."""
    work = unit.workdir / "callees"
    work.mkdir(parents=True, exist_ok=True)
    out, log = work / "compiled.obj", work / "trace.log"
    out.unlink(missing_ok=True)
    log.write_text("")
    build._traceReplay(out, unit.source, unit.flags, unit.streams, {
        "MSVC_DIR": str(build.OVERLAY_MSVC), "HOMM3_VC6_INLINE_TRACE": "?",
        "HOMM3_VC6_SHIM_LOG": cc_wrap.winepath_w(log)})
    names, callees = {}, collections.defaultdict(set)
    for line in log.read_text(encoding="latin1").splitlines():
        if line.startswith("sym "):
            _, address, name = line.split(" ", 2)
            names[address] = name
        elif line.startswith("site ") or line.startswith("candidate "):
            fields = dict(word.split("=", 1) for word in line.split()[1:])
            root, callee = names.get(fields["root"]), names.get(fields["callee"])
            if root and callee:
                callees[root].add(callee)
    return {root: sorted(found) for root, found in callees.items()}


def score_object(unit: str, obj: Path) -> dict[str, float]:
    """{function: objdiff fuzzy_match_percent} for a candidate object of
    `unit` against its delinked retail target, through the same paired
    normalization the full build applies (the number the ledger banks)."""
    import os
    import subprocess
    import tempfile
    from homm3.build import normalize_objs as normalize
    from homm3.vc6 import tu_state_sweep as sweep

    target = sweep._first_pass(unit, (normalize.OBJDIFF / "target" / f"{unit}.c.obj").read_bytes())
    base, paired, _ = normalize.canonicalize_pair(
        sweep._first_pass(unit, Path(obj).read_bytes()), target, unit,
        normalize._retail_symbol_rvas(), image_base=normalize.retail_image_base())
    with tempfile.TemporaryDirectory(dir=STATE_ROOT) as directory:
        directory = Path(directory)
        (directory / "base.obj").write_bytes(base)
        (directory / "target.obj").write_bytes(paired)
        (directory / "objdiff.json").write_text(json.dumps({
            "build_base": False, "build_target": False,
            "options": {"functionRelocDiffs": "all"},
            "units": [{"name": unit, "base_path": str(directory / "base.obj"),
                       "target_path": str(directory / "target.obj"),
                       "scratch": {"platform": "win32", "compiler": "msvc6.0"}}]}))
        process = subprocess.run(
            ["objdiff-cli", "-C", str(directory), "-L", "error", "report", "generate",
             "-o", "report.json"], capture_output=True, text=True,
            env=dict(os.environ, RAYON_NUM_THREADS="1"))
        if process.returncode:
            raise RuntimeError((process.stdout + process.stderr).strip())
        report = json.loads((directory / "report.json").read_text())
    return {fn["name"]: float(fn.get("fuzzy_match_percent") or 0.0)
            for fn in report["units"][0].get("functions", [])}


def scan_states(rounds: int, offsets: bool = True) -> list[State]:
    """One axis at a time from the captured state: the phase value, each
    declaration offset (unless the unit is known offset-inert), each
    callee-prefix round."""
    return ([State(0), State(1)]
            + ([State(None, k) for k in range(1, DECL_PERIOD)] if offsets else [])
            + [State(None, 0, j) for j in range(rounds)])


def compile_m(unit: Unit, *, phases=PHASES, offsets=range(DECL_PERIOD), jobs: int = 6,
              axes: str = "product", score: bool = False, sweep_offsets: bool = True,
              focus: set[str] | None = None) -> dict:
    """Every function of the unit with each distinct assembly it takes
    across the swept states, labelled with those states. axes="product"
    sweeps phase x offset; axes="scan" sweeps each axis alone and adds the
    product only when some function responds to both phase and offset.
    With score, each assembly carries its objdiff fuzzy score. In scan mode,
    sweep_offsets=False skips the offset axis (for units a handle census
    found inert) and focus limits the callee-prefix rounds to the longest
    prefix among those functions."""
    plain_obj = unit.plain()
    plain = object_functions(plain_obj)
    plain_scores = score_object(unit.name, plain_obj) if score else {}
    captured = object_functions(unit.replay(State(), "captured")[0])
    if captured != plain:
        raise RuntimeError("shim with no state set is not inert for this unit")
    callee_map = inline_callees(unit)
    order = [row["name"] for row in read_state(unit)]
    prefixes = callee_prefixes(order, callee_map)
    rounds = max((len(v) for k, v in prefixes.items() if focus is None or k in focus),
                 default=0) + 1
    if axes == "scan":
        todo = scan_states(rounds, sweep_offsets)
    else:
        todo = states(phases, offsets) + [State(None, 0, j) for j in range(rounds)]

    def one(item):
        index, state = item
        compiled = (compiled_round(prefixes, state.callee_order)
                    if state.callee_order is not None else None)
        obj, _ = unit.replay(state, f"slot{index}-{state.phase}-{state.offset}-{state.callee_order}",
                             compiled=compiled)
        funcs = object_functions(obj)
        scores = score_object(unit.name, obj) if score else {}
        shutil.rmtree(obj.parent, ignore_errors=True)
        return state, funcs, scores

    table: dict[str, dict[bytes, list]] = collections.defaultdict(dict)
    fuzzy: dict[tuple[str, bytes], float] = {}
    for name, code in captured.items():
        table[name].setdefault(code, []).append(State().as_dict())
        if name in plain_scores:
            fuzzy[name, code] = plain_scores[name]

    def absorb(results):
        for state, funcs, scores in results:
            for name, code in funcs.items():
                table[name].setdefault(code, []).append(state.as_dict())
                if name in scores:
                    fuzzy.setdefault((name, code), scores[name])

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        absorb(pool.map(one, list(enumerate(todo))))
        if axes == "scan":
            def responds(name, test):
                return any(test(s) for code, labels in table[name].items()
                           if code != captured.get(name) for s in labels)
            both = [name for name in table
                    if responds(name, lambda s: s["phase"] is not None)
                    and responds(name, lambda s: s["decl_offset"])]
            if both:
                extra = [State(p, k) for p in phases for k in range(1, DECL_PERIOD)]
                absorb(pool.map(one, list(enumerate(extra, len(todo)))))
    functions = {}
    for name in sorted(table):
        variants = []
        for code, labels in table[name].items():
            variant = {"sha": digest(code), "bytes": code.hex(), "states": labels,
                       "captured": State().as_dict() in labels}
            if (name, code) in fuzzy:
                variant["score"] = fuzzy[name, code]
            variants.append(variant)
        variants.sort(key=lambda v: (not v["captured"], -len(v["states"]), v["sha"]))
        functions[name] = {"m": len(variants), "variants": variants,
                           "inline_callees": callee_map.get(name, []),
                           "compiled_callees_in_order": prefixes.get(name, [])}
        if name in plain_scores:
            functions[name]["current_score"] = plain_scores[name]
    return {"unit": unit.name, "period": DECL_PERIOD, "phases": list(phases),
            "offsets": list(offsets), "axes": axes, "callee_order_rounds": rounds,
            "functions": functions}


def write_prediction(result: dict) -> Path:
    path = STATE_ROOT / result["unit"] / "compile-m.json"
    path.write_text(json.dumps(result, indent=1, sort_keys=True) + "\n")
    return path


# --------------------------------------------------------------------------
# fuzz verification
# --------------------------------------------------------------------------

UNRELATED_SNIPPETS = (
    "typedef int h3fz_t{n};\n",
    "struct h3fz_s{n} {{ int a; short b; void m(); }};\n",
    "enum h3fz_e{n} {{ H3FZ_A{n}, H3FZ_B{n}, H3FZ_C{n} }};\n",
    "extern int h3fz_g{n};\n",
    "int h3fz_d{n}(int, int);\n",
    "static int h3fz_f{n}(int a) {{ return a * {n} + 1; }}\n",
    "int h3fz_f{n}(int a) {{ return a * {n} + 1; }}\n",
    "int h3fz_l{n}(const int* p, int c) {{ int s = 0; for (int i = 0; i < c; ++i) s += p[i] ^ {n}; return s; }}\n",
    "struct h3fz_r{n} {{ h3fz_r{n}(); ~h3fz_r{n}(); }};\nvoid h3fz_x{n}() {{ h3fz_r{n} r; }}\n",
    "int h3fz_w{n}(int k) {{ switch (k) {{ case 0: return 3; case 1: return {n}; case 2: return 9; case 3: return 4; default: return 0; }} }}\n",
    "struct h3fz_v{n} {{ virtual ~h3fz_v{n}(); virtual int f(); int a; }};\nint h3fz_v{n}::f() {{ return a + {n}; }}\n",
)


def _blocks(text: str) -> list[tuple[int, int]]:
    """[start, end) of each annotated definition block (VA line to next)."""
    starts = [m.start() for m in re.finditer(r"^VA\(0x", text, re.M)]
    return [(a, starts[i + 1] if i + 1 < len(starts) else len(text)) for i, a in enumerate(starts)]


def unrelated_edit(text: str, rng: random.Random, serial: int,
                   headers: dict[str, str] | None = None) -> tuple[str, list[str], dict[str, str]]:
    """One random unrelated edit: 1..4 insertions of declarations/definitions
    at top-level boundaries, a swap of two adjacent definition blocks, or an
    inserted declaration in a shadow copy of a directly included header.
    Returns (unit text, description, {header name: shadow text})."""
    kind = rng.choice(("insert", "insert", "swap", "front", "header")) if headers else rng.choice(("insert", "insert", "swap", "front"))
    blocks = _blocks(text)
    if kind == "front" and len(blocks) >= 2:
        i = rng.randrange(1, len(blocks))
        (f0, _), (b0, b1) = blocks[0], blocks[i]
        block = text[b0:b1] if text[b0:b1].endswith("\n") else text[b0:b1] + "\n"
        rest = text[:b0] + text[b1:]
        moved = re.match(r"VA\((0x[0-9a-fA-F]+)", block)
        return (rest[:f0] + block + rest[f0:],
                [f"swap front {i} " + (moved.group(1) if moved else "")], {})
    if kind == "swap" and len(blocks) >= 2:
        i = rng.randrange(len(blocks) - 1)
        (a0, a1), (b0, b1) = blocks[i], blocks[i + 1]
        first, second = text[a0:a1], text[b0:b1]
        if not second.endswith("\n"):
            second += "\n"
        moved = [m.group(1) for m in (re.match(r"VA\((0x[0-9a-fA-F]+)", first),
                                      re.match(r"VA\((0x[0-9a-fA-F]+)", second)) if m]
        return (text[:a0] + second + first + text[b1:],
                [f"swap blocks {i}/{i + 1} " + " ".join(moved)], {})
    if kind == "header" and headers:
        name = rng.choice(sorted(headers))
        body = headers[name]
        cut = body.rfind("#endif")
        cut = cut if cut >= 0 else len(body)
        snippet = rng.choice(UNRELATED_SNIPPETS[:5]).format(n=serial * 10 + 9)
        return text, [f"header {name}: {snippet.strip()[:40]}"], {name: body[:cut] + snippet + body[cut:]}
    anchors = [0] + [a for a, _ in blocks]
    edits, inserts = [], []
    for j in range(rng.randint(1, 4)):
        at = rng.choice(anchors)
        snippet = rng.choice(UNRELATED_SNIPPETS).format(n=serial * 10 + j)
        inserts.append((at, snippet))
        edits.append(f"@{at}:{snippet.strip().splitlines()[0][:48]}")
    for at, snippet in sorted(inserts, key=lambda item: -item[0]):
        text = text[:at] + snippet + text[at:]
    return text, edits, {}


def direct_headers(unit: "Unit") -> dict[str, str]:
    """Project headers the unit includes by quoted name, resolved like VC6."""
    text = unit.source.read_text(encoding="latin1")
    roots = [unit.source.parent, *[p for p in Project(_common.REPO).includes if p.is_dir()]]
    out = {}
    for name in re.findall(r'^#include\s+"([^"]+)"', text, re.M):
        for root in roots:
            path = root / name
            if path.is_file():
                out[name] = path.read_text(encoding="latin1")
                break
    return out


def _include(unit: Unit) -> str:
    roots = [cc_wrap.msvc_dir() / "include",
             *[p for p in Project(_common.REPO).includes if p.is_dir()], unit.source.parent]
    return ";".join(cc_wrap.winepath_w(p) for p in roots)


def fuzz_verify(unit: Unit, prediction: dict, *, edits: int, seed: int = 1, jobs: int = 6,
                only: set[str] | None = None) -> dict:
    """Compile N randomly edited copies with the plain compiler; every
    original function must land inside its predicted set."""
    predicted = {name: {v["sha"]: v for v in data["variants"]}
                 for name, data in prediction["functions"].items()}
    callees = {name: set(data.get("inline_callees", ()))
               for name, data in prediction["functions"].items()}
    va_names: dict[str, str] = {}

    def moved_symbols(desc: list[str]) -> set[str]:
        out = set()
        for item in desc:
            if not item.startswith("swap"):
                continue
            for va in item.split()[3:]:
                if va not in va_names:
                    try:
                        va_names[va] = _selection.retail(va).name
                    except SystemExit:
                        va_names[va] = ""
                out.add(va_names[va])
        return out - {""}
    text = unit.source.read_text(encoding="latin1")
    rng = random.Random(seed)
    headers = direct_headers(unit)
    cases = [unrelated_edit(text, rng, i, headers) for i in range(edits)]

    def one(item):
        index, (edited, desc, shadows) = item
        work = unit.workdir / "fuzz" / f"e{index:04d}"
        if work.exists():
            shutil.rmtree(work)
        work.mkdir(parents=True)
        for name, body in shadows.items():
            (work / name).parent.mkdir(parents=True, exist_ok=True)
            (work / name).write_text(body, encoding="latin1")
        copy = work / unit.source.name
        copy.write_text(edited, encoding="latin1")
        _ilmod._wine_cl([*unit.flags, f"/Fo{copy.stem}.obj", copy.name], work, _include(unit))
        obj = work / f"{copy.stem}.obj"
        funcs = object_functions(obj) if obj.is_file() else None
        shutil.rmtree(work, ignore_errors=True)
        return index, desc, funcs

    escapes, hits, compiled, checked, skipped = [], collections.defaultdict(set), 0, 0, 0
    kinds = collections.Counter()
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for index, desc, funcs in pool.map(one, list(enumerate(cases))):
            if funcs is None:
                continue
            compiled += 1
            kinds[desc[0].split()[0] if desc and not desc[0].startswith("@") else "insert"] += 1
            moved = moved_symbols(desc)
            for name, code in funcs.items():
                if name not in predicted or (only and name not in only):
                    continue
                if moved & callees.get(name, set()):
                    skipped += 1  # the edit moved one of this function's inline callees
                    continue
                checked += 1
                sha = digest(code)
                if sha in predicted[name]:
                    hits[name].add(sha)
                else:
                    escapes.append({"edit": index, "function": name, "sha": sha, "changes": desc})
    coverage = {name: {"hit": len(hits.get(name, ())), "predicted": len(variants)}
                for name, variants in predicted.items() if not only or name in only}
    return {"unit": unit.name, "edits": edits, "compiled": compiled, "checked": checked,
            "skipped_callee_moved": skipped,
            "edit_kinds": dict(kinds),
            "escapes": escapes, "escape_rate": (len(escapes) / checked) if checked else 0.0,
            "coverage": coverage}


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def _unit_and_filter(target: str, function: str | None) -> tuple[str, str | None]:
    if function:
        return target, _selection.retail(function).name
    if target.lower().startswith("0x") or "::" in target or "@" in target:
        selected = _selection.retail(target)
        return selected.unit, selected.name
    return target, None


@contextlib.contextmanager
def _shim():
    with inline_force.forcing_shim():
        yield


def run_state(args) -> int:
    unit_name, symbol = _unit_and_filter(args.target, None)
    with _shim():
        rows = read_state(Unit.open(unit_name))
    for row in rows:
        if symbol and row["name"] != symbol:
            continue
        print(f"{row['index']:4} phase={row['phase']} handle_base={row['handle_base']:#x} "
              f"(mod {DECL_PERIOD} = {row['handle_residue']:2})  {row['name']}")
    return 0


def run_compile_m(args) -> int:
    unit_name, symbol = _unit_and_filter(args.target, args.function)
    offsets = range(args.offsets)
    with _shim():
        unit = Unit.open(unit_name)
        result = compile_m(unit, offsets=offsets, jobs=args.jobs)
    path = write_prediction(result)
    against = None
    if args.against:
        against = object_functions(Path(args.against).resolve())
    for name, data in result["functions"].items():
        if symbol and name != symbol:
            continue
        line = f"M={data['m']:<3} {name}"
        if against is not None and name in against:
            line += "  [against: IN SET]" if digest(against[name]) in {
                v["sha"] for v in data["variants"]} else "  [against: absent]"
        print(line)
        if symbol:
            for v in data["variants"]:
                labels = [State(s["phase"], s["decl_offset"], s.get("callee_order")).label()
                          for s in v["states"][:4]]
                print(f"   {v['sha']}  x{len(v['states']):<3} {'; '.join(labels)}")
    print(f"[compile-m] {path}")
    return 0


def run_fuzz(args) -> int:
    unit_name, symbol = _unit_and_filter(args.target, args.function)
    with _shim():
        unit = Unit.open(unit_name)
        path = STATE_ROOT / unit_name / "compile-m.json"
        if args.reuse and path.exists():
            prediction = json.loads(path.read_text())
        else:
            prediction = compile_m(unit, jobs=args.jobs)
            write_prediction(prediction)
    report = fuzz_verify(unit, prediction, edits=args.edits, seed=args.seed, jobs=args.jobs,
                         only={symbol} if symbol else None)
    out = STATE_ROOT / unit_name / "fuzz-verify.json"
    out.write_text(json.dumps(report, indent=1) + "\n")
    hit = sum(1 for c in report["coverage"].values() if c["hit"])
    print(f"[fuzz-verify] {unit_name}: {report['compiled']}/{report['edits']} edits compiled "
          f"{report['edit_kinds']}, "
          f"{report['checked']} function checks, {len(report['escapes'])} escapes "
          f"(rate {report['escape_rate']:.4f})")
    multi = [n for n, c in report["coverage"].items() if c["predicted"] > 1]
    print(f"[coverage] {hit}/{len(report['coverage'])} functions hit; of the "
          f"{len(multi)} with M>1, predicted variants hit: "
          f"{sum(report['coverage'][n]['hit'] for n in multi)}/"
          f"{sum(report['coverage'][n]['predicted'] for n in multi)}")
    for escape in report["escapes"][:10]:
        print(f"   escape: {escape['function']} after {escape['changes']}")
    print(f"[fuzz-verify] {out}")
    return 1 if report["escapes"] else 0


def retail_membership(prediction: dict, target: Path, symbols: list[str]) -> list[dict]:
    """For each symbol: is the delinked retail copy one of its M assemblies?"""
    retail = object_functions(target)
    rows = []
    for name in symbols:
        data = prediction["functions"].get(name)
        if data is None or name not in retail:
            rows.append({"function": name, "verdict": "not compared"})
            continue
        sha = digest(retail[name])
        hit = next((v for v in data["variants"] if v["sha"] == sha), None)
        if hit is None:
            verdict = "retail not among the M assemblies"
        elif hit["captured"]:
            verdict = "retail in the captured state"
        else:
            verdict = "retail reachable by unrelated-edit state"
        rows.append({"function": name, "m": data["m"], "verdict": verdict,
                     "retail_states": hit["states"][:8] if hit else []})
    return rows


def run_walls(args) -> int:
    by_unit = collections.defaultdict(list)
    for line in Path(args.list).read_text().splitlines():
        fields = line.split("\t")
        if not fields or not fields[0].startswith("0x"):
            continue
        if len(fields) > 1 and (fields[1].startswith("rmg") or fields[1] == "zlib"):
            continue
        try:
            selected = _selection.retail(fields[0])
        except SystemExit:
            continue
        by_unit[selected.unit].append((fields[0], selected.name))
    out = STATE_ROOT / "walls.jsonl"
    rows = []
    with _shim(), out.open("w") as sink:
        for unit_name in sorted(by_unit)[:args.limit or None]:
            path = STATE_ROOT / unit_name / "compile-m.json"
            try:
                if args.reuse and path.exists():
                    prediction = json.loads(path.read_text())
                else:
                    prediction = compile_m(Unit.open(unit_name), jobs=args.jobs)
                    write_prediction(prediction)
                target = _common.REPO / "build/objdiff/target" / f"{unit_name}.c.obj"
                found = retail_membership(prediction, target, [name for _, name in by_unit[unit_name]])
            except (Exception, SystemExit) as error:
                found = [{"function": name, "verdict": f"error: {str(error).splitlines()[0][:120]}"}
                         for _, name in by_unit[unit_name]]
            for (selector, _), row in zip(by_unit[unit_name], found):
                row.update(selector=selector, unit=unit_name)
                rows.append(row)
                sink.write(json.dumps(row) + "\n")
            sink.flush()
            print(f"[walls] {unit_name}: " + ", ".join(r["verdict"].split(":")[0] for r in found),
                  file=sys.stderr)
    counts = collections.Counter(r["verdict"].split(":")[0] for r in rows)
    lines = ["# Stable walls under unrelated-edit state", "", "| verdict | walls |", "| --- | ---: |"]
    lines += [f"| {k} | {v} |" for k, v in counts.most_common()]
    lines += ["", "| VA | unit | M | verdict | retail states |", "| --- | --- | ---: | --- | --- |"]
    for r in rows:
        states = "; ".join(State(s["phase"], s["decl_offset"], s.get("callee_order")).label()
                           for s in r.get("retail_states", [])[:3])
        lines.append(f"| {r['selector']} | {r['unit']} | {r.get('m', '')} | {r['verdict']} | {states} |")
    (STATE_ROOT / "walls.md").write_text("\n".join(lines) + "\n")
    print("\n".join(lines[:8]))
    return 0


# --------------------------------------------------------------------------
# phase census: which functions the phase flag moves, and what retail needs
# --------------------------------------------------------------------------

def retail_order(unit_name: str) -> list[str]:
    """The unit's retail functions in address order."""
    funcs = _selection.get_context().symbols.funcs
    return [v[0] for rva, v in sorted(funcs.items()) if v[1] == unit_name]


def phase_census_unit(unit: Unit, target: Path | None) -> dict:
    order = read_state(unit)
    p0 = object_functions(unit.replay(State(0), "phase0")[0])
    p1 = object_functions(unit.replay(State(1), "phase1")[0])
    retail = object_functions(target) if target and target.is_file() else {}
    retail_first = next((n for n in retail_order(unit.name) if n in retail), None)
    sensitive = []
    for name in sorted(p0):
        if name not in p1 or p0[name] == p1[name]:
            continue
        r = retail.get(name)
        needs = None
        if r is not None:
            d = digest(r)
            needs = 0 if d == digest(p0[name]) else 1 if d == digest(p1[name]) else "neither"
        ours = 0 if order and order[0]["name"] == name else 1
        sensitive.append({"function": name, "retail_needs": needs, "ours": ours,
                          "size": len(p0[name])})
    return {"unit": unit.name, "functions": len(p0),
            "first_compiled": order[0]["name"] if order else None,
            "compile_order_head": [r["name"] for r in order[:4]],
            "retail_first_by_address": retail_first,
            "sensitive": sensitive}


def run_phase_census(args) -> int:
    from homm3 import manifest
    units = args.units or sorted(u for u in manifest.by_unit()
                                 if not u.startswith("rmg") and u != "zlib")
    out = STATE_ROOT / "phase-census.jsonl"
    out.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    with _shim(), out.open("w") as sink:
        def one(name):
            try:
                unit = Unit.open(name)
                target = _common.REPO / "build/objdiff/target" / f"{name}.c.obj"
                return phase_census_unit(unit, target)
            except (Exception, SystemExit) as error:
                return {"unit": name, "error": str(error).splitlines()[0][:160] if str(error) else type(error).__name__}

        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            for row in pool.map(one, units):
                rows.append(row)
                sink.write(json.dumps(row) + "\n")
                sink.flush()
                print(f"[phase] {row['unit']}: "
                      + (row.get("error") or f"{len(row['sensitive'])}/{row['functions']} sensitive"),
                      file=sys.stderr)
    ok = [r for r in rows if "error" not in r]
    total = sum(r["functions"] for r in ok)
    sens = [dict(s, unit=r["unit"]) for r in ok for s in r["sensitive"]]
    lines = ["# Phase-flag census", "",
             f"{len(ok)} units, {total} functions, {len(sens)} phase-sensitive "
             f"({100 * len(sens) / max(total, 1):.2f}%).", "",
             "| unit | function | retail needs | ours | first compiled (ours) | retail first by address |",
             "| --- | --- | --- | --- | --- | --- |"]
    by_unit = {r["unit"]: r for r in ok}
    for s in sens:
        r = by_unit[s["unit"]]
        lines.append(f"| {s['unit']} | {s['function']} | {s['retail_needs']} | {s['ours']} | "
                     f"{r['first_compiled']} | {r['retail_first_by_address']} |")
    (STATE_ROOT / "phase-census.md").write_text("\n".join(lines) + "\n")
    print("\n".join(lines[:3]))
    return 0


# --------------------------------------------------------------------------
# state scan: score every reachable assembly of every function
# --------------------------------------------------------------------------

def scan_units() -> list[str]:
    """Matching scope: game-profile and victor units, without rmg or zlib."""
    import tomllib
    rows = tomllib.loads((_common.REPO / "config/units.toml").read_text())["unit"]
    return sorted(row["unit"] for row in rows
                  if row["source"].startswith("src/") and not row["unit"].startswith("rmg"))


def scan_rows(prediction: dict, ledger: dict) -> list[dict]:
    """Functions that some state moves, with the best-scoring assembly."""
    rows = []
    unit = prediction["unit"]
    for name, data in prediction["functions"].items():
        if data["m"] < 2 or "current_score" not in data:
            continue
        scored = [v for v in data["variants"] if "score" in v]
        if not scored:
            continue
        best = max(scored, key=lambda v: (v["score"], v["captured"]))
        row = ledger.get((unit, name))
        cur = data["current_score"]
        rows.append({
            "unit": unit, "function": name, "m": data["m"], "current": cur,
            "ledger_cur": getattr(row, "cur", None), "ledger_max": getattr(row, "max", None),
            "ledger_hist": getattr(row, "hist", None),
            "best": best["score"], "best_sha": best["sha"], "best_states": best["states"][:6],
            "gain_over_current": round(best["score"] - cur, 5),
            "gain_over_max": (round(best["score"] - row.max, 5) if row is not None else None),
            "axes": sorted({"phase" if s["phase"] is not None else
                            "callee_order" if s.get("callee_order") is not None else "offset"
                            for v in data["variants"] if not v["captured"] for s in v["states"]}),
        })
    return rows


def run_scan(args) -> int:
    from homm3.match import status
    ledger = status.load_baseline()
    focus = collections.defaultdict(set)
    for (unit_name, name), row in ledger.items():
        if (row.cur is not None and row.cur < 100) or row.max < 100:
            focus[unit_name].add(name)
    offset_units = (json.loads(Path(args.offset_units).read_text())
                    if args.offset_units else None)
    units = args.units or [u for u in scan_units() if focus.get(u)]
    out = STATE_ROOT / "scan.jsonl"
    out.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    with _shim(), out.open("a" if args.reuse else "w") as sink:
        for unit_name in units:
            path = STATE_ROOT / unit_name / "compile-m.json"
            try:
                prediction = json.loads(path.read_text()) if args.reuse and path.exists() else None
                if prediction is None or prediction.get("axes") != "scan":
                    prediction = compile_m(
                        Unit.open(unit_name), jobs=args.jobs, axes="scan", score=True,
                        sweep_offsets=offset_units is None or unit_name in offset_units,
                        focus=focus.get(unit_name, set()))
                    write_prediction(prediction)
                found = scan_rows(prediction, ledger)
            except (Exception, SystemExit) as error:
                print(f"[scan] {unit_name}: error {str(error).splitlines()[0][:160] if str(error) else error!r}",
                      file=sys.stderr)
                continue
            for row in found:
                rows.append(row)
                sink.write(json.dumps(row) + "\n")
            sink.flush()
            better = [r for r in found if r["gain_over_current"] > 0]
            print(f"[scan] {unit_name}: {len(found)} state-sensitive, {len(better)} with a better "
                  f"assembly", file=sys.stderr)
    write_scan_report()
    return 0


def write_scan_report() -> Path:
    rows = {}
    for line in (STATE_ROOT / "scan.jsonl").read_text().splitlines():
        row = json.loads(line)
        rows[row["unit"], row["function"]] = row
    rows = sorted(rows.values(), key=lambda r: -r["gain_over_current"])
    candidates = [r for r in rows if r["current"] < 100 or
                  (r["ledger_max"] is not None and r["ledger_max"] < 100)]
    lines = ["# Unrelated-edit state scan", "",
             f"{len(rows)} state-sensitive functions; {len(candidates)} not exact "
             f"(current < 100 or ledger MAX < 100); "
             f"{sum(1 for r in candidates if r['gain_over_current'] > 0)} of them have a "
             f"better-scoring reachable assembly.", "",
             "| unit | function | current | ledger MAX | best | gain | state of best |",
             "| --- | --- | ---: | ---: | ---: | ---: | --- |"]
    for r in candidates:
        label = "; ".join(State(s["phase"], s["decl_offset"], s.get("callee_order")).label()
                          for s in r["best_states"][:3])
        lines.append(f"| {r['unit']} | {r['function']} | {r['current']:.4f} | "
                     f"{r['ledger_max'] if r['ledger_max'] is not None else ''} | {r['best']:.4f} | "
                     f"{r['gain_over_current']:+.4f} | {label} |")
    path = STATE_ROOT / "scan.md"
    path.write_text("\n".join(lines) + "\n")
    print("\n".join(lines[:3]))
    return path
