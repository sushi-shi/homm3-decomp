"""Ghidra adapter for the shared compiler disassembly renderer."""
from __future__ import annotations

from homm3.vc6 import _toolchain
from homm3.vc6.disasm import (
    IMAGE_BASE, Instruction, Reference, load_roles, render, resolve_selector,
)


def _address(program, rva):
    return program.getAddressFactory().getDefaultAddressSpace().getAddress(IMAGE_BASE + rva)


def _row(program, binary, ins):
    rva = ins.getAddress().getOffset() - IMAGE_BASE
    raw = bytes(b & 0xff for b in ins.getBytes())
    offset = binary.rva_to_off(rva)
    if offset is None or binary.data[offset:offset + len(raw)] != raw:
        raise ValueError(f"C2 instruction bytes disagree with pinned DLL at rva 0x{rva:x}")
    references = []
    for ref in ins.getReferencesFrom():
        target = ref.getToAddress()
        if not target.isMemoryAddress():
            continue
        kind = ref.getReferenceType()
        if kind.isFallthrough():
            continue
        symbol = program.getSymbolTable().getPrimarySymbol(target)
        label = str(symbol.getName()) if symbol else ""
        if kind.isCall():
            name = "call"
        elif kind.isJump():
            name = "jump"
        elif kind.isRead() and kind.isWrite():
            name = "read/write"
        elif kind.isRead():
            name = "read"
        elif kind.isWrite():
            name = "write"
        else:
            name = "address"
        references.append(Reference(target.getOffset() - IMAGE_BASE, name, label))
    return Instruction(rva, raw, str(ins), tuple(sorted(
        set(references), key=lambda r: (r.rva, r.kind, r.name))))


def span_rows(program, binary, lo, hi):
    """Read a physical interval without disassembling embedded table bytes."""
    listing = program.getListing()
    first = listing.getInstructionAt(_address(program, lo))
    if first is None:
        containing = listing.getInstructionContaining(_address(program, lo))
        if containing is not None:
            row = _row(program, binary, containing)
            end = row.rva + len(row.raw)
            raise ValueError(
                f"rva 0x{lo:x} is not a Ghidra instruction boundary; "
                f"it splits instruction [0x{row.rva:x}, 0x{end:x}). "
                f"Start at 0x{row.rva:x} to include it or 0x{end:x} to skip it")
        raise ValueError(f"rva 0x{lo:x} is not a Ghidra instruction boundary")
    rows = []
    for ins in listing.getInstructions(_address(program, lo), True):
        rva = ins.getAddress().getOffset() - IMAGE_BASE
        if rva >= hi:
            break
        if rva + ins.getLength() > hi:
            raise ValueError(
                f"range end 0x{hi:x} splits instruction at 0x{rva:x}; "
                f"use end 0x{rva + ins.getLength():x} to include it "
                f"or 0x{rva:x} to exclude it")
        rows.append(_row(program, binary, ins))
    return rows


def render_span(program, lo, hi, *, verbose=True):
    """Also used by the existing regasg dump, without a second label path."""
    binary = _toolchain.Binary("C2.DLL")
    return render(span_rows(program, binary, lo, hi), load_roles(), verbose=verbose,
                  title=f"; C2 physical interval [0x{lo:x}, 0x{hi:x}); not a complete function CFG")


def run(program, binary, roles, args, span):
    funcs = [(fn.getEntryPoint().getOffset() - IMAGE_BASE, str(fn.getName()))
             for fn in program.getFunctionManager().getFunctions(True)]
    symbols = [(s.getAddress().getOffset() - IMAGE_BASE, str(s.getName()))
               for s in program.getSymbolTable().getAllSymbols(True)
               if s.getAddress().isMemoryAddress()]
    rva = resolve_selector(args.target, roles, symbols)
    address = _address(program, rva)
    memory = program.getMemory()
    block = memory.getBlock(address)
    if block is None:
        raise ValueError(f"rva 0x{rva:x} is outside C2.DLL")
    sha = _toolchain.PINNED['C2.DLL'][0]
    title = f"; C2.DLL 12.00.8447 sha256={sha}\n; target {args.target}: rva 0x{rva:x}, va 0x{IMAGE_BASE + rva:x}"
    if args.refs:
        rows = {}
        for ref in program.getReferenceManager().getReferencesTo(address):
            source = ref.getFromAddress()
            source_block = memory.getBlock(source)
            if source_block is None or not source_block.isExecute():
                continue
            ins = program.getListing().getInstructionAt(source)
            if ins:
                row = _row(program, binary, ins)
                rows[row.rva] = row
        title += f"\n; incoming code references: {len(rows)} site(s)"
        print(render([rows[k] for k in sorted(rows)], roles,
                     verbose=args.verbose, title=title, focus=(rva,)), end="")
        return 0
    if not block.isExecute():
        raise ValueError(f"rva 0x{rva:x} is data; use --refs for its code references")
    if span:
        lo, hi = (rva + value for value in span)
    else:
        entries = sorted(r for r, _ in funcs if r > rva)
        lo = rva
        hi = entries[0] if entries else block.getEnd().getOffset() - IMAGE_BASE + 1
    if hi > block.getEnd().getOffset() - IMAGE_BASE + 1:
        raise ValueError("disassembly range leaves the executable memory block")
    title += (f"\n; physical interval [0x{lo:x}, 0x{hi:x}); end exclusive. "
              "Ghidra instruction boundaries.\n; This interval is not a complete function CFG; cold blocks may be elsewhere.")
    print(render(span_rows(program, binary, lo, hi), roles,
                 verbose=args.verbose, title=title, focus=(rva,)), end="")
    return 0
