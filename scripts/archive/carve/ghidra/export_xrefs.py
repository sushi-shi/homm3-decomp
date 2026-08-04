# -*- coding: utf-8 -*-
"""Export per-function structure for naming: xrefs, thunks, imports, strings.

The naming pass needs evidence Ghidra alone holds: the call graph (relative
E8 calls carry no relocation, so the reloc sweep cannot see them), thunk
identity, and which external/imported routine a function reaches. Strings are
sniffed straight from memory rather than trusted to be defined data - on this
stripped image most literals were never typed.

One row per function; list columns are `;`-joined and truncated (the namer
only needs the dominant few).
"""
from homm3.carve import common

MAX_STRING = 48
MAX_LIST = 6


def _export(program):
    listing = program.getListing()
    fm = program.getFunctionManager()
    memory = program.getMemory()
    base = program.getImageBase().getOffset()
    ref_mgr = program.getReferenceManager()

    def rva(address):
        return address.getOffset() - base

    def sniff_string(address):
        """Printable NUL-terminated run at `address`, or None."""
        try:
            block = memory.getBlock(address)
            if block is None or block.isExecute():
                return None
            out = []
            for i in range(MAX_STRING):
                value = memory.getByte(address.add(i)) & 0xFF
                if value == 0:
                    break
                if not (0x20 <= value < 0x7F):
                    return None
                out.append(chr(value))
            return "".join(out) if len(out) >= 4 else None
        except Exception:
            return None

    rows = []
    for fn in fm.getFunctions(True):
        entry = fn.getEntryPoint()
        block = memory.getBlock(entry)
        if block is None or not block.isExecute():
            continue
        body = fn.getBody()

        callers = set()
        for ref in ref_mgr.getReferencesTo(entry):
            caller = fm.getFunctionContaining(ref.getFromAddress())
            if caller is not None and caller.getEntryPoint() != entry:
                callers.add(rva(caller.getEntryPoint()))

        callees, externals, strings = set(), [], []
        for called in fn.getCalledFunctions(None):
            if called.isExternal() or called.isThunk():
                name = called.getName()
                if name not in externals:
                    externals.append(name)
            if not called.isExternal():
                callees.add(rva(called.getEntryPoint()))

        instructions = listing.getInstructions(body, True)
        while instructions.hasNext():
            instruction = instructions.next()
            for ref in instruction.getReferencesFrom():
                if ref.isMemoryReference():
                    text = sniff_string(ref.getToAddress())
                    if text and text not in strings:
                        strings.append(text)
                        if len(strings) >= MAX_LIST:
                            break

        thunked = fn.getThunkedFunction(True)
        rows.append((
            "0x%x" % rva(entry),
            fn.getName(),
            "1" if fn.isThunk() else "0",
            ("0x%x" % rva(thunked.getEntryPoint())) if thunked is not None
            and not thunked.isExternal() else
            (thunked.getName() if thunked is not None else "-"),
            str(len(callers)),
            str(len(callees)),
            ";".join("0x%x" % c for c in sorted(callers)[:MAX_LIST]) or "-",
            ";".join(externals[:MAX_LIST]) or "-",
            ";".join(s.replace("\t", " ").replace(";", ",")
                     for s in strings[:MAX_LIST]) or "-",
        ))

    rows.sort(key=lambda row: int(row[0], 16))
    out = common.HOMM3_DIR / "build/dna/function_xrefs.tsv"
    common.write_tsv(out, "homm3.carve.ghidra.export_xrefs",
                     ["entry_rva", "ghidra_name", "is_thunk", "thunk_target",
                      "caller_count", "callee_count", "callers", "externals",
                      "strings"], rows)
    print("[export_xrefs] wrote %d function rows -> %s" % (len(rows), out))


_export(currentProgram)  # noqa: F821 - injected by the pyghidra harness
