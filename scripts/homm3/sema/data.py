"""Bounded byte, pointer, string and vtable reads from the verified retail image."""
from __future__ import annotations

import contextlib
import json
import struct
import sys

from homm3.sema._common import die
from homm3.sema.context import get_context


def read(image, rva, size):
    section = image.section_of(rva)
    if size <= 0 or size > 1048576:
        raise ValueError("size must be between 1 and 1048576 bytes")
    if section is None or rva < section.rva or rva + size > section.rva + section.size:
        raise ValueError("span is not wholly backed by bytes in one PE section "
                         "(uninitialized memory has no on-disk value)")
    start = section.raw_offset + rva - section.rva
    return image.data[start:start + size]


def pointer(ctx, value):
    if value == 0:
        return {"value": value, "kind": "null", "name": None}
    if not ctx.image.in_image(value):
        return {"value": value, "kind": "external", "name": None}
    rva = value - ctx.image.image_base
    fn = rva if rva in ctx.symbols.funcs else ctx.symbols.owner(rva)
    if fn is not None:
        name, unit, _size, _prov = ctx.symbols.funcs[fn]
        return {"value": value, "kind": "function", "name": name,
                "unit": unit, "rva": rva, "offset": rva - fn}
    row = ctx.symbols.datas.get(rva)
    return {"value": value, "kind": "data", "name": row[0] if row else None,
            "rva": rva}


def run(args):
    ctx = get_context()
    with contextlib.redirect_stdout(sys.stderr):
        rva = ctx.symbols.resolve(args.addr)
    count = args.count
    is_pointers = args.format in ("pointers", "vtable")
    if is_pointers and args.size is not None:
        die("use --count for pointer slots")
    if not is_pointers and count is not None:
        die("use --size for hex/string bytes")
    if count is not None and count <= 0:
        die("--count must be positive")
    vt = None
    if args.format == "vtable":
        vt = ctx.vtable_of(rva)
        if count is None:
            if vt is None:
                die("unknown vtable extent; supply --count for an exploratory pointer read")
            count = next(n for addr, n in ctx.vtables if addr == vt[0]) - vt[1]
    size = (count or 16) * 4 if is_pointers else (64 if args.size is None else args.size)
    try:
        raw = read(ctx.image, rva, size)
    except ValueError as exc:
        die(str(exc))
    result = {"schema": "homm3.sema.data.v1", "rva": rva,
              "va": rva + ctx.image.image_base, "size": size,
              "format": args.format, "hex": raw.hex()}
    if args.format in ("pointers", "vtable"):
        result["entries"] = [dict(pointer(ctx, value), index=i,
                                  address=result["va"] + 4 * i)
                             for i, (value,) in enumerate(struct.iter_unpack("<I", raw))]
        if args.format == "vtable":
            result["admitted_vtable"] = vt
    elif args.format == "string":
        end = raw.find(b"\0")
        result.update(text=raw[:end if end >= 0 else len(raw)].decode("latin1"),
                      terminated=end >= 0)
    if args.json:
        print(json.dumps(result, indent=2))
    elif "entries" in result:
        for row in result["entries"]:
            label = row["name"] or row["kind"]
            if row.get("offset"):
                label += f"+0x{row['offset']:x}"
            print(f"{row['address']:08x} [{row['index']:3}] {row['value']:08x}  {label}")
    elif "text" in result:
        print(repr(result["text"]) + ("" if result["terminated"] else " [no NUL in span]"))
    else:
        for offset in range(0, len(raw), 16):
            chunk = raw[offset:offset + 16]
            ascii_text = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
            print(f"{result['va'] + offset:08x}  {chunk.hex(' '):47}  {ascii_text}")
    return 0
