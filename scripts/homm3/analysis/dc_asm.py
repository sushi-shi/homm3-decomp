"""SH4 assembly and CFG views for :mod:`homm3.analysis.dreamcast`.

CodeView line-program entries are rendered as debugger *breakpoints*, matching
Vostok's ``gen_sources`` terminology. ``S_BLOCK32`` records are lexical C++
scopes. Basic blocks are inferred independently from SH4 control flow; the
three kinds of boundary are deliberately never conflated.
"""
from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any, Callable, Iterable, TextIO

from homm3.analysis import dc_lines


class AsmError(ValueError):
    pass


@dataclass(frozen=True)
class Instruction:
    address: int
    size: int
    data: bytes
    mnemonic: str
    operands: str = ""

    @property
    def conditional(self) -> bool:
        return self.mnemonic in {"bt", "bf", "bt/s", "bf/s"}

    @property
    def direct_jump(self) -> bool:
        return self.mnemonic == "bra"

    @property
    def indirect_jump(self) -> bool:
        return self.mnemonic in {"braf", "jmp"}

    @property
    def returns(self) -> bool:
        return self.mnemonic in {"rts", "rte"}

    @property
    def terminator(self) -> bool:
        return self.conditional or self.direct_jump or self.indirect_jump \
            or self.returns

    @property
    def delayed(self) -> bool:
        return self.mnemonic in {
            "bt/s", "bf/s", "bra", "braf", "bsr", "bsrf", "jmp", "jsr",
            "rts", "rte",
        }

    @property
    def target(self) -> int | None:
        if not (self.conditional or self.direct_jump or self.mnemonic == "bsr"):
            return None
        try:
            return int(self.operands.strip(), 0)
        except ValueError:
            return None


@dataclass
class BasicBlock:
    start: int
    instructions: list[Instruction] = field(default_factory=list)
    successors: list[int] = field(default_factory=list)
    predecessors: list[int] = field(default_factory=list)

    @property
    def end(self) -> int:
        if not self.instructions:
            return self.start
        last = self.instructions[-1]
        return last.address + last.size


@dataclass(frozen=True)
class Breakpoint:
    index: int
    address: int
    line: int
    source: str
    scope_depth: int


def _decode_capstone(data: bytes) -> Callable[[int], Instruction]:
    try:
        import capstone
    except ImportError as exc:
        raise AsmError("capstone is not importable; use the analysis dev shell") from exc
    # SH4 does not imply the floating-point extension in Capstone's mode
    # flags.  The Dreamcast CPU uses it heavily, so omitting SHFPU turns valid
    # fmov/fadd/flds/lds encodings into apparent data words.
    md = capstone.Cs(capstone.CS_ARCH_SH,
                     capstone.CS_MODE_SH4 | capstone.CS_MODE_SHFPU
                     | capstone.CS_MODE_LITTLE_ENDIAN)

    def decode(address: int) -> Instruction:
        raw = data[dc_lines.TEXT_RAW + address:dc_lines.TEXT_RAW + address + 8]
        rows = list(md.disasm(raw, address, count=1))
        if not rows or rows[0].address != address:
            if len(raw) < 2:
                raise AsmError(f"truncated SH4 instruction at dc {address:#x}")
            # Capstone 5 omits a handful of valid SH4 system/FPU transfers
            # (0x405a is one in this image). Keep traversal exact for every
            # control-transfer encoding and render other unsupported words as
            # data-like instructions instead of losing the rest of the body.
            word = int.from_bytes(raw[:2], "little")
            mnemonic, operands = ".word", f"0x{word:04x}"
            high = word & 0xFF00
            if high in {0x8900, 0x8B00, 0x8D00, 0x8F00}:
                names = {0x8900: "bt", 0x8B00: "bf",
                         0x8D00: "bt/s", 0x8F00: "bf/s"}
                disp = word & 0xFF
                if disp & 0x80:
                    disp -= 0x100
                mnemonic, operands = names[high], hex(address + 4 + disp * 2)
            elif (word & 0xF000) in {0xA000, 0xB000}:
                disp = word & 0xFFF
                if disp & 0x800:
                    disp -= 0x1000
                mnemonic = "bra" if (word & 0xF000) == 0xA000 else "bsr"
                operands = hex(address + 4 + disp * 2)
            elif word == 0x000B:
                mnemonic, operands = "rts", ""
            elif word == 0x002B:
                mnemonic, operands = "rte", ""
            elif (word & 0xF0FF) == 0x400B:
                mnemonic, operands = "jsr", f"@r{(word >> 8) & 0xF}"
            elif (word & 0xF0FF) == 0x402B:
                mnemonic, operands = "jmp", f"@r{(word >> 8) & 0xF}"
            elif (word & 0xF0FF) == 0x0023:
                mnemonic, operands = "braf", f"r{(word >> 8) & 0xF}"
            elif (word & 0xF0FF) == 0x0003:
                mnemonic, operands = "bsrf", f"r{(word >> 8) & 0xF}"
            return Instruction(address, 2, raw[:2], mnemonic, operands)
        ins = rows[0]
        return Instruction(ins.address, ins.size, bytes(ins.bytes),
                           ins.mnemonic, ins.op_str)

    return decode


def _walk_block(start: int, end: int, leaders: set[int],
                decode: Callable[[int], Instruction]) -> BasicBlock:
    block = BasicBlock(start)
    address = start
    while address < end:
        if address != start and address in leaders:
            block.successors.append(address)
            break
        ins = decode(address)
        if ins.address + ins.size > end:
            raise AsmError(f"instruction at dc {address:#x} crosses function end")
        block.instructions.append(ins)

        if ins.terminator:
            after = ins.address + ins.size
            if ins.delayed:
                if after >= end:
                    raise AsmError(f"missing SH4 delay slot at dc {ins.address:#x}")
                slot = decode(after)
                block.instructions.append(slot)
                after = slot.address + slot.size
            if ins.conditional:
                if ins.target is not None:
                    block.successors.append(ins.target)
                block.successors.append(after)
            elif ins.direct_jump:
                if ins.target is not None:
                    block.successors.append(ins.target)
            break
        address = ins.address + ins.size
    return block


def build_cfg(start: int, end: int, decode: Callable[[int], Instruction],
              extra_roots: Iterable[int] = ()) -> list[BasicBlock]:
    """Infer SH4 blocks, using uncovered line entries as indirect-arm roots.

    The entry-rooted traversal avoids treating literal pools as code. CodeView
    line entries not covered by that traversal become extra roots, recovering
    switch arms reached through indirect jumps.
    """
    roots = {start}

    def pass_(leaders: set[int]) -> tuple[dict[int, BasicBlock], set[int]]:
        blocks: dict[int, BasicBlock] = {}
        discovered = set(leaders)
        queue = sorted(leaders)
        while queue:
            block_start = queue.pop(0)
            if block_start in blocks or not (start <= block_start < end):
                continue
            block = _walk_block(block_start, end, discovered, decode)
            blocks[block_start] = block
            for successor in block.successors:
                if start <= successor < end and successor not in discovered:
                    discovered.add(successor)
                    queue.append(successor)
            queue.sort()
        return blocks, discovered

    while True:
        blocks, discovered = pass_(roots)
        if discovered == roots:
            break
        roots = discovered

    covered = {ins.address for block in blocks.values()
               for ins in block.instructions}
    roots.update(address for address in extra_roots
                 if start <= address < end and address not in covered)
    while True:
        blocks, discovered = pass_(roots)
        if discovered == roots:
            break
        roots = discovered

    for block in blocks.values():
        block.successors = sorted({successor for successor in block.successors
                                   if successor in blocks})
    for block in blocks.values():
        for successor in block.successors:
            blocks[successor].predecessors.append(block.start)
    for block in blocks.values():
        block.predecessors = sorted(set(block.predecessors))
    return [blocks[address] for address in sorted(blocks)]


def _scope_depth(address: int, scopes: list[tuple[int, int]]) -> int:
    return sum(start <= address < start + size for start, size in scopes)


def build_view(row: dict[str, str], dump: list[str], data: bytes) -> dict[str, Any]:
    start, size = int(row["offset"], 0), int(row["cb"], 0)
    end = start + size
    proc = dc_lines.find_proc(dump, start)
    if proc is None:
        raise AsmError(f"no CodeView procedure at dc {start:#x}")
    _name, _cb, _locals, scopes = proc
    line_rows = dc_lines.line_table(dump, start, size)
    breakpoints = [Breakpoint(index, address, line, source,
                              _scope_depth(address, scopes))
                   for index, (address, line, source) in enumerate(line_rows)]
    blocks = build_cfg(start, end, _decode_capstone(data),
                       (bp.address for bp in breakpoints))
    return {
        "authority": "Dreamcast RoE/WinCE reference; analysis output, not retail evidence",
        "function": {
            "name": row["name"], "module": row["module"],
            "dc_offset": start, "dc_size": size, "dc_end": end,
        },
        "breakpoints": [asdict(bp) for bp in breakpoints],
        "lexical_scopes": [{"start": scope_start,
                            "end": scope_start + scope_size,
                            "size": scope_size}
                           for scope_start, scope_size in scopes],
        "blocks": [{
            "start": block.start, "end": block.end,
            "predecessors": block.predecessors,
            "successors": block.successors,
            "instructions": [{
                "address": ins.address, "size": ins.size,
                "bytes": ins.data.hex(), "mnemonic": ins.mnemonic,
                "operands": ins.operands,
            } for ins in block.instructions],
        } for block in blocks],
    }


def control_events(view: dict[str, Any], data: bytes) -> dict[int, dict[str, Any]]:
    """Return calls/conditional branches at CFG-confirmed instruction RVAs.

    Walking only instructions admitted by :func:`build_cfg` matters: literal
    pools are embedded in SH4 function extents and arbitrary pool halfwords
    can look like ``bsr``/``bt``.  A linear halfword scan would report those
    constants as source-level control flow.
    """
    sh4 = dc_lines.Sh4(data)
    events: dict[int, dict[str, Any]] = {}
    for block in view["blocks"]:
        # A switch arm recovered from a CodeView breakpoint can have no CFG
        # predecessor: its computed-jump landing pad (and therefore its
        # literal load) is not necessarily represented by a line row.  Never
        # let a register constant from an unrelated, earlier-address block
        # leak into such an arm and turn an unresolved jsr into a false named
        # source call.
        register_targets: dict[int, int] = {}
        for ins in block["instructions"]:
            address = ins["address"]
            raw = bytes.fromhex(ins["bytes"])
            word = int.from_bytes(raw, "little")
            if (word >> 12) == 0xD:
                target = sh4.pool(address)
                if target is not None:
                    register_targets[(word >> 8) & 0xF] = target

            event: dict[str, Any] = {}
            if ins["mnemonic"] in {"bt", "bf", "bt/s", "bf/s"}:
                event["conditional_branch"] = True
            if (word & 0xF0FF) == 0x400B:  # jsr @Rn
                event["call_target_va"] = register_targets.get(
                    (word >> 8) & 0xF)
            elif (word & 0xF000) == 0xB000:  # bsr disp12
                disp = word & 0xFFF
                if disp & 0x800:
                    disp -= 0x1000
                event["call_target_va"] = \
                    dc_lines.POOL_BASE + address + 4 + disp * 2
            elif (word & 0xF0FF) == 0x0003:  # bsrf Rn
                event["call_target_va"] = None
            if event:
                events[address] = event
    return events


def _basename(path: str) -> str:
    return path.rsplit("\\", 1)[-1].rsplit("/", 1)[-1]


def render(view: dict[str, Any], data: bytes, symbols: dict[int, str],
           *, blocks: bool = False, breakpoints: bool = True,
           out: TextIO) -> None:
    fn = view["function"]
    block_rows = view["blocks"]
    label_of = {block["start"]: f"B{index}" for index, block in enumerate(block_rows)}
    bp_at: dict[int, list[dict[str, Any]]] = {}
    for bp in view["breakpoints"]:
        bp_at.setdefault(bp["address"], []).append(bp)
    scopes_at: dict[int, list[dict[str, Any]]] = {}
    scope_ends: dict[int, list[dict[str, Any]]] = {}
    for scope in view["lexical_scopes"]:
        scopes_at.setdefault(scope["start"], []).append(scope)
        scope_ends.setdefault(scope["end"], []).append(scope)

    print("; DREAMCAST REFERENCE — ANALYSIS OUTPUT, NOT RETAIL EVIDENCE", file=out)
    print(f"; {fn['name']}  {fn['module']}  dc {fn['dc_offset']:#x}  "
          f"{fn['dc_size']} B SH4", file=out)
    print(f"; {len(view['breakpoints'])} CodeView breakpoint(s), "
          f"{len(view['lexical_scopes'])} lexical scope(s), "
          f"{len(block_rows)} inferred CFG block(s)", file=out)
    print("; bp = CodeView line entry; scope = S_BLOCK32; Bn = inferred CFG",
          file=out)

    sh4 = dc_lines.Sh4(data)
    for block in block_rows:
        label = label_of[block["start"]]
        if blocks:
            preds = ",".join(label_of[p] for p in block["predecessors"]) or "-"
            succs = ",".join(label_of[s] for s in block["successors"]) or "-"
            print(f"\n{label}: ; dc {block['start']:#x}..{block['end']:#x}  "
                  f"pred={preds} succ={succs}", file=out)
        else:
            print(f"\n{label}:", file=out)

        register_targets: dict[str, int] = {}
        for raw in block["instructions"]:
            address = raw["address"]
            for _scope in scope_ends.get(address, ()):
                print("; [scope close]", file=out)
            if breakpoints:
                for bp in bp_at.get(address, ()):
                    depth = f" scope-depth={bp['scope_depth']}" \
                        if bp["scope_depth"] else ""
                    print(f"; [bp {bp['index']:03}] {_basename(bp['source'])}:"
                          f"{bp['line']}{depth}", file=out)
            if address in scopes_at and (not breakpoints or address not in bp_at):
                print(f"; [scope open x{len(scopes_at[address])}; no breakpoint]",
                      file=out)

            mnemonic, operands = raw["mnemonic"], raw["operands"]
            notes: list[str] = []
            raw_bytes = bytes.fromhex(raw["bytes"])
            word = int.from_bytes(raw_bytes, "little")
            if (word >> 12) == 0xD:
                target = sh4.pool(address)
                register = operands.rsplit(",", 1)[-1].strip()
                register_targets[register] = target
                notes.append(symbols.get(target, f"= {target:#x}"))
            elif mnemonic == "jsr":
                register = operands.lstrip("@").strip()
                target = register_targets.get(register)
                if target is not None:
                    notes.append("call " + symbols.get(target, f"{target:#x}"))
            elif mnemonic == "bsr":
                target = int(operands, 0)
                name = symbols.get(dc_lines.POOL_BASE + target)
                if name:
                    notes.append("call " + name)
            elif mnemonic in {"bt", "bf", "bt/s", "bf/s", "bra"}:
                try:
                    target = int(operands, 0)
                    if target in label_of:
                        notes.append("-> " + label_of[target])
                except ValueError:
                    pass
            suffix = " ; " + "; ".join(notes) if notes else ""
            byte_text = " ".join(f"{byte:02x}" for byte in raw_bytes)
            print(f"  {address:08x}  {byte_text:<11} {mnemonic:<8} "
                  f"{operands:<24}{suffix}", file=out)

    for _scope in scope_ends.get(fn["dc_end"], ()):
        print("; [scope close]", file=out)
