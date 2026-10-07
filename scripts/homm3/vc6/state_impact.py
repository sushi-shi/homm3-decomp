"""homm3.vc6.state_impact - predict which functions a source edit moves,
through which compiler state (docs/vc6/state-impact.md).

The prediction is analytic: it applies the emission-order and handle rules
to the unit's current object, a C1XX front-end capture of the edited text,
and the unit's state->bytes table from `homm3 vc6 compile-m`. It does not
compile the edit. `--verify` then compiles the edited text once and
compares every function with the prediction.

    homm3 vc6 impact UNIT                 # compile-order groups
    homm3 vc6 impact UNIT EDIT [--verify]

EDIT is one of
    swap-include:A.h:B.h        swap two #include lines
    insert:LINE:TEXT            insert TEXT before 1-based LINE (\\n allowed)
    move:VA:before:VA           move a VA-annotated definition block
    remove:VA                   delete a VA-annotated definition block

The three states (phase-flag.md, handle-period.md, unstable-state.md):
* phase: only the first body C2 compiles receives 0;
* callee order: an inline callee counts as compiled when its body was
  emitted earlier in the compile order;
* handle offset: a function's assembly depends on the handles (mod 64) of
  the data symbols it addresses.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

from homm3.vc6 import _common, _unit

IMPACT = _common.EVIDENCE / "impact"
STATE_ROOT = _common.EVIDENCE / "unstable-state"

# ---------------------------------------------------------------------------
# object facts: compile order (= COFF section order) and references
# ---------------------------------------------------------------------------

_SYM = re.compile(r"\(sec\s+(\d+)\)\(fl 0x00\)\(ty\s+(\w+)\)\(scl\s+(\d+)\).* (\S+)$")


def object_symbols(obj: Path) -> tuple[list[str], set[str]]:
    """(function bodies in compile order, defined data symbols)."""
    out = subprocess.run(["llvm-objdump", "-t", str(obj)], capture_output=True,
                         text=True, check=True).stdout
    funcs, data = [], set()
    for line in out.splitlines():
        m = _SYM.search(line)
        if not m or int(m.group(1)) == 0:
            continue
        if m.group(2) == "20":
            funcs.append((int(m.group(1)), m.group(4)))
        else:
            data.add(m.group(4))
    return [n for _, n in sorted(funcs)], data


def object_references(obj: Path) -> dict[str, list[str]]:
    """{function: relocation targets in its code, in order, with repeats}.
    A target that appears as an immediate operand (an address constant:
    `push offset x`, `mov reg, offset x`) is also listed as `imm:x`."""
    out = subprocess.run(["llvm-objdump", "-dr", "--x86-asm-syntax=intel", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    refs: dict[str, list[str]] = {}
    current, last = None, ""
    for line in out.splitlines():
        head = re.match(r"^[0-9a-f]+ <(.+)>:$", line)
        if head:
            current = head.group(1)
            refs.setdefault(current, [])
            continue
        reloc = re.search(r"IMAGE_REL_I386_(DIR32|REL32)\s+(\S+)", line)
        if reloc and current:
            refs[current].append(reloc.group(2))
            if reloc.group(1) == "DIR32" and _immediate_operand(last):
                refs[current].append("imm:" + reloc.group(2))
        elif re.match(r"^\s+[0-9a-f]+:", line):
            last = line
    return refs


def _immediate_operand(listing_line: str) -> bool:
    """Is the relocated field the instruction's immediate (its last operand
    a number: `push 0x0`, `mov r, 0x0`, `mov dword ptr [ebp - 4], 0x0`)
    rather than a memory displacement?"""
    text = listing_line.split("\t")[-1].strip()
    last = text.rsplit(",", 1)[-1].strip() if "," in text else text.split(None, 1)[-1]
    return bool(re.fullmatch(r"-?0x[0-9a-f]+|\d+", last))


@dataclass
class Body:
    name: str
    kind: str               # own | initializer | inline-copy | deferred
    users: list[str] = field(default_factory=list)  # inline-copy: possible first users


def classify(order: list[str], refs: dict[str, list[str]], claimed: set[str]) -> list[Body]:
    """Emission rule (state-impact.md section 3), read off the object:
    * the unit's own definitions (non-inline, VA-annotated): source order;
    * `_$E` namespace-scope initializers before the last own body: at the
      object's definition point;
    * any other body before the last own body: an inline copy (explicit,
      in-class or virtual inline, implicit special member, or a template
      member needed by one), emitted right after its first user;
    * everything after the last own body: deferred to the end of the TU
      (template instantiations needed by own functions, library statics)."""
    bodies: list[Body] = []
    last_own = max((i for i, n in enumerate(order) if n in claimed), default=-1)
    for i, name in enumerate(order):
        users = [b for b in order[:i] if name in refs.get(b, ())]
        if name in claimed:
            bodies.append(Body(name, "own"))
        elif i > last_own:
            bodies.append(Body(name, "deferred"))
        elif name.startswith("_$E"):
            bodies.append(Body(name, "initializer"))
        else:
            # an inline copy, implicit member or unannotated helper: it
            # travels with the emission event just before it
            bodies.append(Body(name, "inline-copy", users or ([order[i - 1]] if i else [])))
    return bodies


def reorder(bodies: list[Body], refs: dict[str, list[str]], own_order: list[str],
            moved: str | None = None, candidates: dict[str, set[str]] | None = None) -> list[str]:
    """Predicted compile order after the own definitions take *own_order*.

    Each own body leads a group: the initializers before it and the inline
    copies emitted after it (its emission event). A group moves with its
    own body (whatever emission event placed a copy there travels with
    it). Deferred bodies stay at the end."""
    groups: dict[str, list[str]] = {}
    lead = None
    head: list[str] = []
    pending: list[str] = []
    for b in bodies:
        if b.kind == "own":
            if lead is None:
                head += pending      # initializers above every definition stay first
            else:
                groups[lead] += pending
            lead = b.name
            groups[lead] = [lead]
            pending = []
        elif b.kind == "initializer" and lead is None:
            pending.append(b.name)
        elif b.kind in ("inline-copy", "initializer"):
            (groups[lead] if lead else head).append(b.name)
    deferred = [b.name for b in bodies if b.kind == "deferred"]
    copies = {b.name: b for b in bodies if b.kind == "inline-copy"}
    placed: list[str] = list(head)
    emitted = set(head)
    lead_of = {m: lead for lead, members in groups.items() for m in members}
    for name in own_order:
        for member in groups.get(name, [name]):
            if member in emitted:
                continue
            placed.append(member)
            emitted.add(member)
        if name == moved and candidates:
            # the moved function becomes the first user of inline bodies it
            # can need out of line (its inline candidates) whose emission
            # event now comes later: they are emitted right after it
            later = own_order[own_order.index(name) + 1:]
            for c in candidates.get(name, ()):
                if c in copies and c not in emitted and lead_of.get(c) in later:
                    placed.append(c)
                    emitted.add(c)
    placed += [n for n in pending + deferred if n not in emitted]
    placed += [c for c in copies if c not in emitted]
    return placed


# ---------------------------------------------------------------------------
# handles of addressed data symbols (front-end capture)
# ---------------------------------------------------------------------------


def handles_by_name(text: str, unit: str, tag: str) -> dict[str, int]:
    """IL handle of every named gl record, including `$`-statics."""
    from homm3.vc6 import _il
    from homm3.vc6.shim import build
    work = (IMPACT / unit / tag).resolve()
    work.mkdir(parents=True, exist_ok=True)
    src = work / Path(_unit.source_for_unit(unit)).name
    src.write_text(text, encoding="latin1")
    build._ensure_wine_env()
    gl = build._traceCapture(src, list(_unit.flags_for_unit(unit)), work)["gl"]
    hw = _il.gl_highwater(gl)
    out: dict[str, int] = {}
    for m in re.finditer(rb"[$?][\x21-\x7e]{2,}\x00", gl):
        name = m.group(0)[:-1].decode("latin1")
        if name in out:
            continue
        h = decode_record_handle(gl, m.start(), hw)
        if h is not None:
            out[name] = h
    return out


def decode_record_handle(gl: bytes, o: int, hw: int) -> int | None:
    """Handle of the named gl record whose name starts at *o*:
    `<u16> 00 name` (handle < 0x8000), `<u32> 00 name` with bit 15 set
    (31-bit form: low 15 bits, then the upper word), or `<u16> name` for
    `$` file statics."""
    if o >= 5 and gl[o - 1] == 0:
        word = struct.unpack_from("<I", gl, o - 5)[0]
        if word & 0x8000:
            h = (word & 0x7FFF) | ((word >> 16) << 15)
            if 0x8000 <= h <= hw:
                return h
    if o >= 3 and gl[o - 1] == 0:
        lo = struct.unpack_from("<H", gl, o - 3)[0]
        if lo and not lo & 0x8000 and lo <= hw:
            return lo
    if o >= 2:
        lo = struct.unpack_from("<H", gl, o - 2)[0]
        if 0 < lo <= hw:
            return lo
    return None


def obj_to_il_name(name: str) -> str:
    """Object names of file statics are `_x`; their IL records are `$x`."""
    return "$" + name[1:] if name.startswith("_") and not name.startswith("__") else name


def address_constants(fn: str, refs: dict[str, list[str]]) -> list[str]:
    """Data symbols whose address the function uses as an immediate: the
    operands C2's constant-pseudo table hashes by handle (handle-period.md)."""
    out = []
    for t in refs.get(fn, ()):
        if t.startswith("imm:") and t[4:] not in out and not t[4:].startswith("$"):
            out.append(t[4:])
    return out


# ---------------------------------------------------------------------------
# edits
# ---------------------------------------------------------------------------


def _blocks(text: str) -> list[tuple[int, int, int]]:
    starts = [(m.start(), int(m.group(1), 16)) for m in re.finditer(r"^VA\((0x[0-9a-fA-F]+)", text, re.M)]
    return [(a, starts[i + 1][0] if i + 1 < len(starts) else len(text), va)
            for i, (a, va) in enumerate(starts)]


def apply_edit(text: str, edit: str) -> str:
    kind, _, rest = edit.partition(":")
    if kind == "swap-include":
        a, b = rest.split(":")
        la = re.search(rf'^#include\s+["<]{re.escape(a)}[">].*$', text, re.M)
        lb = re.search(rf'^#include\s+["<]{re.escape(b)}[">].*$', text, re.M)
        if not la or not lb:
            raise SystemExit(f"include not found: {a} / {b}")
        first, second = sorted((la, lb), key=lambda m: m.start())
        return (text[:first.start()] + second.group(0) + text[first.end():second.start()]
                + first.group(0) + text[second.end():])
    if kind == "insert":
        line, _, snippet = rest.partition(":")
        lines = text.split("\n")
        n = int(line) - 1
        return "\n".join(lines[:n] + [snippet.replace("\\n", "\n")] + lines[n:])
    blocks = _blocks(text)
    if kind == "move":
        va, _, target = rest.partition(":before:")
        va, target = int(va, 16), int(target, 16)
        src = next(b for b in blocks if b[2] == va)
        dst = next(b for b in blocks if b[2] == target)
        block = text[src[0]:src[1]]
        if not block.endswith("\n"):
            block += "\n"
        rest_text = text[:src[0]] + text[src[1]:]
        at = dst[0] if dst[0] < src[0] else dst[0] - (src[1] - src[0])
        return rest_text[:at] + block + rest_text[at:]
    if kind == "remove":
        va = int(rest, 16)
        src = next(b for b in blocks if b[2] == va)
        return text[:src[0]] + text[src[1]:]
    raise SystemExit(f"unknown edit {edit!r}")


def own_symbols(text: str, unit: str) -> set[str]:
    """Symbols of the unit's non-inline VA-annotated definitions."""
    syms = block_symbols(text, unit)
    out = set()
    for a, b, va in _blocks(text):
        head = text[a:b].split("{", 1)[0]
        if va in syms and not re.search(r"\binline\b", head):
            out.add(syms[va])
    return out


def block_symbols(text: str, unit: str) -> dict[int, str]:
    """VA -> compiler symbol for the unit's annotated definitions."""
    from homm3.vc6 import _selection
    out = {}
    for _, _, va in _blocks(text):
        try:
            sel = _selection.retail(hex(va))
        except Exception:
            continue
        if sel.unit == unit:
            out[va] = sel.name
    return out

# ---------------------------------------------------------------------------
# state -> bytes table
# ---------------------------------------------------------------------------


def load_table(unit: str) -> dict | None:
    path = STATE_ROOT / unit / "compile-m.json"
    return json.loads(path.read_text()) if path.is_file() else None


def variant_for(entry: dict, phase: int, offset: int, prefix: int | None) -> str | None:
    """sha of the assembly at one state, or None when the table lacks it."""
    for v in entry["variants"]:
        for s in v["states"]:
            if prefix is not None:
                if s.get("callee_order") == prefix:
                    return v["sha"]
            elif s.get("callee_order") is None and s.get("phase") == phase and s.get("decl_offset") == offset:
                return v["sha"]
    return None


def offset_sensitive(entry: dict, phase: int) -> bool:
    shas = {v["sha"] for v in entry["variants"] for st in v["states"]
            if st.get("callee_order") is None and st.get("phase") == phase}
    return len(shas) > 1


def phase_sensitive(entry: dict) -> bool:
    return variant_for(entry, 0, 0, None) != variant_for(entry, 1, 0, None)


def prefix_sensitive(entry: dict) -> bool:
    shas = {v["sha"] for v in entry["variants"] for st in v["states"]
            if st.get("callee_order") is not None}
    return len(shas) > 1


def captured_sha(entry: dict) -> str:
    return next(v["sha"] for v in entry["variants"] if v["captured"])

# ---------------------------------------------------------------------------
# prediction
# ---------------------------------------------------------------------------


@dataclass
class Baseline:
    unit: str
    text: str
    obj: Path
    order: list[str]
    refs: dict[str, list[str]]
    data: set[str]
    bodies: list[Body]
    claimed: set[str]
    handles: dict[str, int] = field(default_factory=dict)


def baseline(unit: str) -> Baseline:
    src = _unit.source_for_unit(unit)
    text = Path(src).read_text(encoding="latin1")
    obj, err = _unit.compile_text(text, unit, IMPACT / unit / "base", Path(src).stem, with_listing=False)
    if obj is None:
        _common.die(f"baseline compile failed: {err}")
    order, data = object_symbols(obj)
    refs = object_references(obj)
    claimed = own_symbols(text, unit)
    bodies = classify(order, refs, claimed)
    return Baseline(unit, text, obj, order, refs, data, bodies, claimed)


def predict(base: Baseline, edit: str, table: dict | None) -> dict:
    new_text = apply_edit(base.text, edit)
    funcs = set(base.order)
    # 1. compile order
    own_old = [b.name for b in base.bodies if b.kind == "own"]
    kind = edit.split(":")[0]
    own_new = list(own_old)
    moved = None
    if kind in ("move", "remove"):
        syms = block_symbols(base.text, base.unit)
        parts = edit.split(":")
        moved = syms.get(int(parts[1], 16))
        if kind == "remove":
            own_new = [n for n in own_new if n != moved]
        else:
            target = syms.get(int(parts[3], 16))
            own_new.remove(moved)
            own_new.insert(own_new.index(target), moved)
    if kind == "insert":
        line, _, snippet = edit.split(":", 1)[1].partition(":")
        if re.search(r"\)\s*(const\s*)?\{", snippet):
            # a new non-inline definition: an own body before the next
            # annotated definition below the insertion line
            at = sum(len(x) + 1 for x in base.text.split("\n")[:int(line) - 1])
            syms = block_symbols(base.text, base.unit)
            nxt = [syms[va] for a, _, va in _blocks(base.text) if a >= at and syms.get(va) in own_new]
            pos = own_new.index(nxt[0]) if nxt else len(own_new)
            own_new.insert(pos, "<inserted>")
    candidates = {k: set(v["inline_callees"]) for k, v in (table or {}).get("functions", {}).items()}
    moved_name = moved if kind == "move" else None
    new_order = (reorder(base.bodies, base.refs, own_new, moved_name, candidates)
                 if own_new != own_old else list(base.order))
    old_pos = {n: i for i, n in enumerate(base.order)}
    new_pos = {n: i for i, n in enumerate(new_order)}
    # 2. handles of addressed data symbols
    if not base.handles:
        base.handles = handles_by_name(base.text, base.unit, "base")
    new_handles = handles_by_name(new_text, base.unit, "edit")
    rows = []
    for fn in base.order:
        if fn not in new_pos:
            continue
        row = {"function": fn}
        # phase
        p0 = 0 if old_pos[fn] == 0 else 1
        p1 = 0 if new_pos[fn] == 0 else 1
        # callee prefix
        entry = (table or {}).get("functions", {}).get(fn)
        callees = entry["compiled_callees_in_order"] if entry else []
        j0 = sum(1 for c in callees if c in old_pos and old_pos[c] < old_pos[fn])
        j1 = sum(1 for c in callees if c in new_pos and new_pos[c] < new_pos[fn])
        # handle deltas
        deltas = set()
        for sym in address_constants(fn, base.refs):
            il = obj_to_il_name(sym)
            if il in base.handles and il in new_handles:
                deltas.add(new_handles[il] - base.handles[il])
            else:
                deltas.add(None)          # unresolved: cannot claim a uniform shift
        offsets = {d % 64 if d is not None else -1 for d in deltas}
        if entry and not offset_sensitive(entry, p0):
            offsets = set()          # the table shows one assembly for all 64 offsets
        if entry and p0 != p1 and not phase_sensitive(entry):
            p1 = p0
        if entry and j0 != j1 and not prefix_sensitive(entry):
            j1 = j0
        changed = []
        if p0 != p1:
            changed.append(f"phase {p0}->{p1}")
        if j0 != j1:
            changed.append(f"callees-compiled {j0}->{j1}")
        if offsets - {0}:
            changed.append("handles " + ("+%d" % next(iter(deltas)) if len(deltas) == 1
                                         else "mixed " + str(sorted(deltas, key=lambda d: (d is None, d or 0)))))
        row.update(phase=(p0, p1), prefix=(j0, j1), states=changed)
        # predicted assembly
        if entry:
            base_sha = captured_sha(entry)
            if not changed:
                row["predict"] = base_sha
            elif len(changed) == 1 and p0 != p1:
                row["predict"] = variant_for(entry, p1, 0, None)
            elif len(changed) == 1 and j0 != j1:
                row["predict"] = variant_for(entry, 0, 0, j1)
            elif len(changed) == 1 and len(offsets) == 1:
                row["predict"] = variant_for(entry, p0, next(iter(offsets)), None)
            else:
                row["predict"] = None   # combined or mixed state: not in the table
            row["captured"] = base_sha
        rows.append(row)
    return {"edit": edit, "text": new_text, "order": new_order, "rows": rows}


def verify(base: Baseline, result: dict) -> list[dict]:
    from homm3.vc6.unstable_state import object_functions, digest
    obj, err = _unit.compile_text(result["text"], base.unit, IMPACT / base.unit / "verify",
                                  Path(_unit.source_for_unit(base.unit)).stem, with_listing=False)
    if obj is None:
        _common.die(f"verify compile failed: {err}")
    actual = {n: digest(b) for n, b in object_functions(obj).items()}
    out = []
    for row in result["rows"]:
        fn = row["function"]
        if "predict" not in row or fn not in actual:
            continue
        a = actual[fn]
        moved = a != row["captured"]
        if row["predict"] is None:
            verdict = "changed (predicted: state change, variant unknown)" if moved else "unchanged (state change predicted)"
        elif a == row["predict"]:
            verdict = "as predicted" + (" (changed)" if moved else "")
        else:
            verdict = "MISPREDICTED"
        out.append({"function": fn, "states": row["states"], "verdict": verdict})
    return out


def run_report(args) -> int:
    base = baseline(args.unit)
    first = base.order[0] if base.order else None
    print(f"[impact] {args.unit}: {len(base.order)} bodies; phase 0 goes to {first}")
    for i, b in enumerate(base.bodies):
        extra = f" after first user among {b.users[:2]}" if b.users else ""
        print(f"  {i:3} {b.kind:11} {b.name}{extra}")
    return 0


def run_predict(args) -> int:
    base = baseline(args.unit)
    table = load_table(args.unit)
    result = predict(base, args.edit, table)
    for row in result["rows"]:
        if row["states"]:
            print(f"  {row['function'][:70]:70} {'; '.join(row['states'])}")
    if args.verify:
        print("[impact] verify:")
        rows = verify(base, result)
        for v in rows:
            if v["states"] or v["verdict"] != "as predicted":
                print(f"  {v['function'][:70]:70} {v['verdict']}  {'; '.join(v['states'])}")
        tally: dict[str, int] = {}
        for v in rows:
            key = v["verdict"].split(" (")[0]
            if v["verdict"].endswith("(changed)"):
                key += " (changed)"
            tally[key] = tally.get(key, 0) + 1
        print("[impact] summary: " + ", ".join(f"{k}: {n}" for k, n in sorted(tally.items())))
    return 0


def run(args) -> int:
    """`homm3 vc6 impact UNIT [EDIT] [--verify]`."""
    if args.edit:
        return run_predict(args)
    return run_report(args)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="homm3.vc6.state_impact")
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("report")
    r.add_argument("unit")
    p = sub.add_parser("predict")
    p.add_argument("unit")
    p.add_argument("edit")
    p.add_argument("--verify", action="store_true")
    args = ap.parse_args(argv)
    return run_report(args) if args.cmd == "report" else run_predict(args)


if __name__ == "__main__":
    sys.exit(main())
