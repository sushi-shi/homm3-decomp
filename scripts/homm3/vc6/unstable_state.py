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
    before every symbol of the unit (a global declaration offset)."""
    phase: int | None = None
    offset: int = 0

    def label(self) -> str:
        if self.phase is None and not self.offset:
            return "captured"
        parts = []
        if self.phase is not None:
            parts.append(f"phase={self.phase}")
        parts.append(f"decl-offset={self.offset}")
        return " ".join(parts)

    def as_dict(self) -> dict:
        return {"phase": self.phase, "decl_offset": self.offset}


def state_env(state: State, log: Path | None = None) -> dict:
    env = {"MSVC_DIR": str(build.OVERLAY_MSVC)}
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

    def replay(self, state: State, slot: str, log: bool = False) -> tuple[Path, str]:
        work = self.workdir / slot
        work.mkdir(parents=True, exist_ok=True)
        out = work / "compiled.obj"
        out.unlink(missing_ok=True)
        log_path = work / "state.log" if log else None
        if log_path:
            log_path.write_text("")
        process = build._traceReplay(out, self.source, self.flags,
                                     shifted_streams(self.streams, state.offset),
                                     state_env(state, log_path))
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

def states(phases=PHASES, offsets=range(DECL_PERIOD)) -> list[State]:
    return [State(p, k) for p in phases for k in offsets]


def compile_m(unit: Unit, *, phases=PHASES, offsets=range(DECL_PERIOD), jobs: int = 6) -> dict:
    """Every function of the unit with each distinct assembly it takes
    across the swept states, labelled with those states."""
    plain = object_functions(unit.plain())
    captured = object_functions(unit.replay(State(), "captured")[0])
    if captured != plain:
        raise RuntimeError("shim with no state set is not inert for this unit")
    todo = states(phases, offsets)

    def one(item):
        index, state = item
        obj, _ = unit.replay(state, f"slot{index % max(jobs, 1)}-{state.phase}-{state.offset}")
        funcs = object_functions(obj)
        shutil.rmtree(obj.parent, ignore_errors=True)
        return state, funcs

    table: dict[str, dict[bytes, list]] = collections.defaultdict(dict)
    for name, code in captured.items():
        table[name].setdefault(code, []).append(State().as_dict())
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for state, funcs in pool.map(one, list(enumerate(todo))):
            for name, code in funcs.items():
                table[name].setdefault(code, []).append(state.as_dict())
    functions = {}
    for name in sorted(table):
        variants = []
        for code, labels in table[name].items():
            variants.append({"sha": digest(code), "bytes": code.hex(), "states": labels,
                             "captured": State().as_dict() in labels})
        variants.sort(key=lambda v: (not v["captured"], -len(v["states"]), v["sha"]))
        functions[name] = {"m": len(variants), "variants": variants}
    return {"unit": unit.name, "period": DECL_PERIOD, "phases": list(phases),
            "offsets": list(offsets), "functions": functions}


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


def unrelated_edit(text: str, rng: random.Random, serial: int) -> tuple[str, list[str]]:
    """Insert 1..4 unrelated declarations/definitions at top-level boundaries
    (the top of the unit or right before an annotated definition)."""
    anchors = [0] + [m.start() for m in re.finditer(r"^VA\(0x", text, re.M)]
    edits = []
    inserts = []
    for j in range(rng.randint(1, 4)):
        at = rng.choice(anchors)
        snippet = rng.choice(UNRELATED_SNIPPETS).format(n=serial * 10 + j)
        inserts.append((at, snippet))
        edits.append(f"@{at}:{snippet.strip().splitlines()[0][:48]}")
    for at, snippet in sorted(inserts, key=lambda item: -item[0]):
        text = text[:at] + snippet + text[at:]
    return text, edits


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
    text = unit.source.read_text(encoding="latin1")
    rng = random.Random(seed)
    cases = [unrelated_edit(text, rng, i) for i in range(edits)]

    def one(item):
        index, (edited, desc) = item
        work = unit.workdir / "fuzz" / f"e{index:04d}"
        if work.exists():
            shutil.rmtree(work)
        work.mkdir(parents=True)
        copy = work / unit.source.name
        copy.write_text(edited, encoding="latin1")
        _ilmod._wine_cl([*unit.flags, f"/Fo{copy.stem}.obj", copy.name], work, _include(unit))
        obj = work / f"{copy.stem}.obj"
        funcs = object_functions(obj) if obj.is_file() else None
        shutil.rmtree(work, ignore_errors=True)
        return index, desc, funcs

    escapes, hits, compiled, checked = [], collections.defaultdict(set), 0, 0
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for index, desc, funcs in pool.map(one, list(enumerate(cases))):
            if funcs is None:
                continue
            compiled += 1
            for name, code in funcs.items():
                if name not in predicted or (only and name not in only):
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
                labels = [State(s["phase"], s["decl_offset"]).label() for s in v["states"][:4]]
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
    print(f"[fuzz-verify] {unit_name}: {report['compiled']}/{report['edits']} edits compiled, "
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
