"""homm3.sema.strings - literal evidence for function identification.

Literal starts are the printable-ASCII runs (>= 4 chars) of the
non-executable sections; references are the admitted dir32 reloc sites
whose stored operand lands EXACTLY on a run start, attributed
size-bounded to the owning function. No blind immediate scan - a
coincidental 4-byte pattern cannot fabricate a reference.

  strings 0x<addr>      the literals a function references; when the
                        address IS a literal start, the functions
                        referencing it (the exact reverse row)
  strings --find TEXT   which functions reference a matching literal

The reverse lookup is the primary identification lever for the unnamed
target functions: a distinctive literal names its consumer.

rc: 0 = answered (even an empty set), 2 = error.
"""
from __future__ import annotations

import re

from homm3.sema._common import die
from homm3.sema.context import get_context

_RUN = re.compile(rb"[\x20-\x7e]{4,}")


def _string_map(ctx) -> dict:
    """{va_of_run_start: text} over every non-executable section."""
    out = {}
    image = ctx.image
    for section in image.sections:
        if section.executable:
            continue
        blob = image.blob(section)
        for m in _RUN.finditer(blob):
            va = image.image_base + section.rva + m.start()
            out[va] = m.group().decode("latin1")
    return out


def _fn_refs(ctx, wanted: set) -> dict:
    """{fn_rva: {referenced_va}} for reloc sites in function bodies whose
    operand is one of `wanted`."""
    refs = {}
    for site, value in ctx.relocs:
        if value in wanted:
            owner = ctx.symbols.owner(site)
            if owner is not None:
                refs.setdefault(owner, set()).add(value)
    return refs


def run(args) -> None:
    ctx = get_context()
    if not args.find and not args.target:
        die("give an 0x<addr> or --find <text>")
    strings = _string_map(ctx)

    if args.find:
        needle = args.find.lower()
        matches = {va: text for va, text in strings.items()
                   if needle in text.lower()}
        if not matches:
            print(f"(no literal contains {args.find!r})")
            return
        refs = _fn_refs(ctx, set(matches))
        by_string = {}
        for fn, vas in refs.items():
            for va in vas:
                by_string.setdefault(va, []).append(fn)
        shown = 0
        for va, text in sorted(matches.items()):
            preview = text if len(text) <= 70 else text[:67] + "..."
            print(f"@0x{va:08x}  {preview!r}")
            for fn in sorted(by_string.get(va, [])):
                print(f"    <- {ctx.symbols.label(fn)}")
            if va not in by_string:
                print("    (no reloc-site reference from a function body)")
            shown += 1
            if shown >= 24:
                print(f"... (+{len(matches) - shown} more literal(s))")
                break
        return

    rva = ctx.symbols.resolve(args.target)
    owner = ctx.symbols.owner(rva)
    if owner is None:
        va = ctx.image.image_base + rva
        if va in strings:
            # The address IS a literal: answer the reverse question.
            text = strings[va]
            preview = text if len(text) <= 70 else text[:67] + "..."
            print(f"@0x{va:08x}  {preview!r}")
            referrers = sorted(_fn_refs(ctx, {va}))
            for fn in referrers:
                print(f"    <- {ctx.symbols.label(fn)}")
            if not referrers:
                print("    (no reloc-site reference from a function body)")
            return
        owner = rva
    refs = _fn_refs(ctx, set(strings))
    mine = refs.get(owner)
    print(f"==== literals referenced by {ctx.symbols.label(owner)} ====")
    if not mine:
        print("  (none - no reloc site in this body lands on a literal start)")
        return
    for va in sorted(mine):
        text = strings[va]
        preview = text if len(text) <= 70 else text[:67] + "..."
        print(f"  @0x{va:08x}  {preview!r}")
