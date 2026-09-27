"""Compare the case-to-arm mapping of VC6 jump tables with retail.

This is a lead for near matches: aggregate instruction and relocation views
can agree while cases reach different arms. Only self-contained, relocatable
tables with resolvable bounds are reported; unsupported forms stay unknown.
"""
from __future__ import annotations

import csv
import struct
from dataclasses import dataclass
from pathlib import Path

from homm3.build.canonicalize_data_symbols import CoffObject, DIR32, FUNCTION_TYPE
from homm3.match import status
from homm3.sema import _asm
from homm3.sema.context import get_context


@dataclass(frozen=True)
class Case:
    value: int
    arm: int
    features: frozenset[str]


@dataclass(frozen=True)
class Switch:
    table: int
    index: int | None
    cases: tuple[Case, ...]


def _function(path: Path, name: str, ordinal: int = 0):
    """Return body and its COFF relocations, all in function-local offsets."""
    if not path.is_file():
        raise ValueError(f"missing comparison object {path}; run homm3 build")
    from homm3.build.normalized_freshness import freshness_problems
    problems = freshness_problems(path)
    if problems:
        raise ValueError(f"stale comparison object {path}: {problems[0]}")
    obj = CoffObject(path.read_bytes())
    symbols = sorted((s for s in obj.symbols.values()
                      if s.name == name and s.section > 0
                      and s.typ == FUNCTION_TYPE), key=lambda s: s.index)
    if ordinal >= len(symbols):
        raise ValueError(f"{name} not emitted by {path.name}")
    symbol = symbols[ordinal]
    section = obj.sections[symbol.section - 1]
    successors = [s.value for s in obj.symbols.values()
                  if s.section == symbol.section and s.value > symbol.value
                  and s.typ == FUNCTION_TYPE]
    end = min(successors, default=section.raw_size)
    body = obj.section_bytes(section)[symbol.value:end]
    refs = {}
    for rel in obj.relocations:
        if rel.section != symbol.section or not symbol.value <= rel.site < end:
            continue
        target = obj.symbols[rel.symbol_index]
        local = rel.site - symbol.value
        resolved = None
        if rel.typ == DIR32 and local + 4 <= len(body) \
                and target.section == symbol.section:
            addend = struct.unpack_from("<i", body, local)[0]
            resolved = target.value + addend - symbol.value
        refs[local] = (target.name, resolved)
    return body, refs


def _table_offset(ins, refs) -> int | None:
    if not ins.disp_size:
        return None
    site = ins.address + ins.disp_offset
    return refs.get(site, (None, None))[1]


def _bound(insns, pos: int, register: int) -> tuple[int, int] | None:
    """Find the guarded 0..N index immediately before one table jump."""
    from capstone.x86_const import X86_OP_IMM, X86_OP_REG
    for i in range(pos - 1, max(-1, pos - 15), -1):
        ins = insns[i]
        if ins.mnemonic == "cmp" and len(ins.operands) == 2 \
                and ins.operands[0].type == X86_OP_REG \
                and ins.operands[0].reg == register:
            guards = [item.mnemonic for item in insns[i + 1:pos]
                      if item.mnemonic.startswith("j")]
            if len(guards) != 1 or not guards[0].startswith("ja"):
                continue
            rhs = ins.operands[1]
            if rhs.type == X86_OP_IMM:
                return (rhs.imm, i) if 0 <= rhs.imm <= 4096 else None
            if rhs.type == X86_OP_REG:
                for prior in reversed(insns[max(0, i - 5):i]):
                    if prior.mnemonic == "mov" and len(prior.operands) == 2 \
                            and prior.operands[0].reg == rhs.reg \
                            and prior.operands[1].type == X86_OP_IMM:
                        return prior.operands[1].imm, i
        if ins.mnemonic in ("ret", "jmp"):
            break
    return None


def _bias(insns, pos: int, register: int) -> int:
    from capstone.x86_const import X86_OP_IMM, X86_OP_REG
    for ins in reversed(insns[max(0, pos - 5):pos]):
        if not ins.operands or ins.operands[0].type != X86_OP_REG \
                or ins.operands[0].reg != register:
            continue
        if ins.mnemonic in ("sub", "add") and len(ins.operands) == 2 \
                and ins.operands[1].type == X86_OP_IMM:
            value = ins.operands[1].imm
            if 0 < value < 4096:
                return value if ins.mnemonic == "sub" else -value
        if ins.mnemonic == "dec":
            return 1
        if ins.mnemonic == "inc":
            return -1
    return 0


def _features(insns, byaddr, start: int, refs: dict, depth: int) -> frozenset[str]:
    from capstone.x86_const import X86_OP_IMM, X86_OP_MEM
    keys = set()
    seen = set()
    at = start
    for _ in range(depth):
        ins = byaddr.get(at)
        if ins is None or at in seen:
            break
        seen.add(at)
        if ins.mnemonic == "jmp" and ins.operands \
                and ins.operands[0].type == X86_OP_IMM:
            at = ins.operands[0].imm
            continue
        if ins.mnemonic.startswith("j") or ins.mnemonic.startswith("ret"):
            keys.add("<ret>" if ins.mnemonic.startswith("ret") else "<branch>")
            break
        if ins.mnemonic == "call":
            ref = next((name for site, (name, _) in refs.items()
                        if ins.address <= site < ins.address + ins.size), None)
            keys.add("call:" + (ref or "indirect"))
        else:
            for op in ins.operands:
                if op.type == X86_OP_IMM and not ins.mnemonic.startswith("j"):
                    keys.add(f"imm:{op.imm & 0xffffffff:x}")
                elif op.type == X86_OP_MEM and op.mem.disp \
                        and ins.reg_name(op.mem.base) not in ("esp", "ebp"):
                    keys.add(f"disp:{op.mem.disp & 0xffffffff:x}")
        at += ins.size
    return frozenset(keys)


def switches(body: bytes, refs: dict, depth: int = 64) -> list[Switch]:
    """Decode direct and byte-indexed self-relocated switch tables."""
    import capstone
    from capstone.x86_const import X86_OP_MEM
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    insns = list(md.disasm(body, 0))
    byaddr = {ins.address: ins for ins in insns}
    out = []
    for pos, ins in enumerate(insns):
        if ins.mnemonic != "jmp" or not ins.operands \
                or ins.operands[0].type != X86_OP_MEM \
                or ins.operands[0].mem.scale != 4:
            continue
        table = _table_offset(ins, refs)
        if table is None or not ins.address < table < len(body):
            continue
        index = None
        source_reg = ins.operands[0].mem.index
        for prior in reversed(insns[max(0, pos - 15):pos]):
            byte_mem = next((op for op in prior.operands
                             if op.type == X86_OP_MEM and op.size == 1), None)
            if byte_mem is not None:
                candidate = _table_offset(prior, refs)
                if candidate is not None and prior.address < candidate < len(body):
                    index = candidate
                    source_reg = byte_mem.mem.index or byte_mem.mem.base
                    break
        bound_info = _bound(insns, pos, source_reg)
        if bound_info is None:
            continue  # without a guarded bound, table extent is only a guess
        bound, cmp_pos = bound_info
        bias = _bias(insns, cmp_pos, source_reg)
        code_end = min(table, index) if index is not None else table
        code = {addr: row for addr, row in byaddr.items() if addr < code_end}
        table_cases = []
        for value in range(bound + 1):
            slot = body[index + value] if index is not None \
                and index + value < len(body) else value
            site = table + 4 * slot
            if site + 4 > len(body) or site not in refs or refs[site][1] is None:
                table_cases = []
                break
            arm = refs[site][1]
            if arm not in code:
                table_cases = []
                break
            table_cases.append(Case(value + bias, arm,
                                    _features(insns, code, arm, refs, depth)))
        if table_cases:
            out.append(Switch(table, index, tuple(table_cases)))
    return out


def _groups(cases: dict[int, Case]) -> dict[int, frozenset[int]]:
    by_arm = {}
    for value, case in cases.items():
        by_arm.setdefault(case.arm, set()).add(value)
    return {value: frozenset(by_arm[case.arm]) for value, case in cases.items()}


def compare(base: list[Switch], target: list[Switch]) -> list[tuple[int, int | None, str]]:
    """Flag changed sharing or a case whose arm matches another case better."""
    hits = []
    if len(base) != len(target):
        hits.append((-1, None, f"switch count {len(base)} vs {len(target)}"))
    for n, (left, right) in enumerate(zip(base, target)):
        b = {c.value: c for c in left.cases}
        t = {c.value: c for c in right.cases}
        gb, gt = _groups(b), _groups(t)
        for value in sorted(b.keys() | t.keys()):
            if value not in b or value not in t:
                hits.append((n, value, "case exists on one side only"))
                continue
            if gb[value] != gt[value]:
                hits.append((n, value, "shares arm with different cases"))
                continue
            own = len(b[value].features ^ t[value].features)
            if not b[value].features or not t[value].features:
                continue
            others = [other for other in b.keys() & t.keys() if other != value
                      and (len(b[value].features ^ t[other].features) < own
                           or len(b[other].features ^ t[value].features) < own)]
            if others:
                hits.append((n, value, f"closer to case(s) {sorted(others)}"))
    return hits


def pair(name: str, unit: str, ordinal: int = 0, depth: int = 64):
    base = _asm.NORMAL_BASE / f"{unit}.obj"
    target = _asm.NORMAL_TARGET / f"{unit}.c.obj"
    return [switches(*_function(path, name, ordinal), depth=depth)
            for path in (base, target)]


def _one(ctx, selector: str, depth: int, verbose: bool, refresh: bool,
         sweep: bool = False) -> int:
    name, unit, rva, _size, ordinal = ctx.symbols.resolve_fn(selector)
    _asm.require_candidate(unit, name, rva)
    if refresh:
        _asm.refresh_unit(unit)
    base, target = pair(name, unit, ordinal, depth)
    hits = compare(base, target)
    if hits or verbose:
        print(f"0x{rva + 0x400000:08x} {name} [{unit}]: "
              f"{len(base)}/{len(target)} switches, {len(hits)} mapping lead(s)")
        if sweep and not verbose:
            print("  cases " + ", ".join(f"{n}:{value}" for n, value, _ in hits[:16])
                  + (f" ... ({len(hits) - 16} more)" if len(hits) > 16 else ""))
            return len(hits)
        for n, value, note in hits:
            where = "switch count" if n < 0 else f"switch {n}, case {value}"
            print(f"  {where}: {note}")
            if n >= 0:
                print(f"    retail table 0x{rva + 0x400000 + target[n].table:08x}")
        if verbose:
            for n, (b, t) in enumerate(zip(base, target)):
                print(f"  switch {n}: base {[(c.value, c.arm) for c in b.cases]}")
                print(f"            retail {[(c.value, c.arm) for c in t.cases]}")
    return len(hits)


def run(args) -> int:
    ctx = get_context()
    if args.all:
        report = status.load_report()
        rows = status.projected_rows(report)
        tokens = [f"0x{row.rva:x}" for row in rows.values()
                  if row.rva is not None and row.max < 100]
    elif args.tsv:
        with open(args.tsv, newline="") as stream:
            tokens = [row[0] for row in csv.reader(stream, delimiter="\t")
                      if row and row[0].startswith("0x")]
    else:
        tokens = [args.target]
    leads = skipped = hit_functions = 0
    for token in tokens:
        try:
            count = _one(ctx, token, args.depth, args.verbose,
                         refresh=not args.no_build and not (args.all or args.tsv),
                         sweep=bool(args.all or args.tsv))
            leads += count
            hit_functions += count > 0
        except (ValueError, OSError) as exc:
            skipped += 1
            if not args.all:
                print(f"[switchmap] skipped {token}: {exc}")
    if args.all or args.tsv:
        print(f"[switchmap] {len(tokens) - skipped} screened; "
              f"{hit_functions} functions with {leads} case leads; "
              f"{skipped} unavailable")
    return 0
