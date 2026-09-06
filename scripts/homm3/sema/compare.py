"""Explicit retail-to-candidate comparison without creating a source claim."""
from __future__ import annotations

import contextlib
import json
import re
import struct
import sys

from homm3.sema import _asm, candidates, data, diff
from homm3.sema._common import die
from homm3.sema.context import get_context


def _reference(ctx, rva, size, text):
    """Express admitted image addresses as COFF relocation names/addends.

    Original image bytes remain available in the payload and --asm view.
    Comparison rows carry relocatable fields, just like the candidate.
    """
    def identity(va):
        addr = va - ctx.image.image_base
        row = ctx.symbols.funcs.get(addr) or ctx.symbols.datas.get(addr)
        if row:
            return row[0], 0
        owner = ctx.symbols.owner(addr)
        if owner is not None:
            return ctx.symbols.funcs[owner][0], addr - owner
        return f"data_{addr:x}", 0

    sites = {site + ctx.image.image_base: value for site, value in ctx.relocs
             if rva <= site < rva + size}
    start = rva + ctx.image.image_base
    lines = []
    for off, raw, body, _relocs in _asm.reloc_rows(text):
        raw = bytearray(raw)
        refs = []
        for site in range(off, off + len(raw)):
            if site in sites and site + 4 <= off + len(raw):
                name, addend = identity(sites[site])
                struct.pack_into("<I", raw, site - off, addend & 0xffffffff)
                refs.append((site, "DIR32", name))
        if len(raw) == 5 and raw[0] in (0xe8, 0xe9):
            destination = off + 5 + struct.unpack_from("<i", raw, 1)[0]
            if not start <= destination < start + size:
                name, addend = identity(destination)
                struct.pack_into("<I", raw, 1, addend & 0xffffffff)
                refs.append((off + 1, "REL32", name))
        body = _asm._NOTE.sub("", body).strip()
        body = re.sub(r"(?<![\w])(-?[0-9]+)(?![\w])",
                      lambda m: hex(int(m.group(1))), body)
        mnemonic, _, operands = body.partition(" ")
        lines.append(f"{off:x}: {raw.hex(' ')}\t{mnemonic}\t{operands}")
        for site, kind, name in refs:
            lines.append(f"\t\t\t{site:x}: IMAGE_REL_I386_{kind}\t{name}")
    return "\n".join(lines) + "\n"


def run(args):
    if args.ordinal < 0:
        die("--ordinal must be >= 0")
    ctx = get_context()
    with contextlib.redirect_stdout(sys.stderr):
        name, unit, rva, size, _ordinal = ctx.symbols.resolve_fn(args.target)
        obj = candidates.object_path(args)
        base, symbol = candidates.selected_text(obj, args.symbol, args.ordinal)
    raw_target = _asm.image_text(ctx, rva, size, name)
    target = _reference(ctx, rva, size, raw_target)
    facts = diff._summary_facts(ctx, base, target, rva, name, unit, 0,
                                args.why_bytes, source_enabled=False,
                                report_enabled=False)
    payload = {"schema": "homm3.sema.compare.v1", "exploratory": True,
               "comparison_fields": "COFF relocation names and addends",
               "retail": {"rva": rva, "va": rva + ctx.image.image_base,
                          "size": size, "hex": data.read(ctx.image, rva, size).hex()},
               "candidate": {"object": str(obj), "symbol": symbol,
                             "ordinal": args.ordinal}, "summary": facts}
    if args.json:
        print(json.dumps(diff._json_value(payload), indent=2))
    else:
        print(f"[exploratory comparison: {obj.name}:{symbol} ordinal={args.ordinal}; "
              "retail identity is not asserted]")
        if args.asm:
            import difflib
            for line in difflib.unified_diff(base.splitlines(), raw_target.splitlines(),
                                             "candidate", "retail", lineterm=""):
                print(line)
        else:
            for line in diff._summary_lines(facts)[0]:
                print(line)
            if args.why_bytes and facts["divergence"] is not None:
                for line in diff._render_divergence(facts["divergence"]):
                    print(line)
    return 0 if facts["agree"] else 1
