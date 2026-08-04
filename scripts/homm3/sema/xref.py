"""homm3.sema.xref - who calls / references this function?

The caller-side complement of `disasm`. Callers come from one linear
E8/E9 (call/jmp rel32) scan of retail .text, each site attributed
SIZE-BOUNDED to its containing function - the boundary map
(config/retail-functions.tsv via symbol_names.csv) is complete, so
attribution always lands or the site is genuinely unowned padding/data.

Modes:
  (default)   caller ancestry TREE (--depth 4; 0 = unlimited): expands
              through game-target and zlib callers, stops at
              runtime/thunk/funclet callers with a frontier tag
  --flat      only the direct rel32 callers (deduped by owner)
  --raw       with --flat: every site, no dedup
  --callees   forward: the function's own direct call targets, plus a
              count of the FF /2 & /4 indirect sites rel32 cannot see

Every mode ends with the data-side references, resolved EXACTLY from
the admitted dir32 reloc sites (config/retail-relocs.tsv) - fn-ptr
tables, command tables, vtable slots (named slot-precisely via
config/retail-vtables.tsv), and address-takings inside .text. No blind
byte scan, no false positives.

rc: 0 = answered (even "no callers"), 2 = error.
"""
from __future__ import annotations

import struct

from homm3.sema._common import die
from homm3.sema.context import get_context

# Universe categories the caller tree expands THROUGH; everything else
# is a frontier (printed, tagged, not expanded).
_EXPAND = {"target", "zlib"}


def _label(ctx, rva: int) -> str:
    row = ctx.symbols.funcs.get(rva)
    if row:
        return f"0x{rva:08x} {row[0]} [{row[1]}]"
    return f"0x{rva:08x} ?"


def _frontier_tag(ctx, rva: int):
    cat = ctx.classes.get(rva, "target")
    return None if cat in _EXPAND else cat


def _data_refs(ctx, target: int):
    """[(site_rva, description)] for every dir32 site holding target's VA."""
    want = ctx.image.image_base + target
    out = []
    for site, value in ctx.relocs:
        if value != want:
            continue
        owner = ctx.symbols.owner(site)
        if owner is not None:
            name, unit, _sz, _p = ctx.symbols.funcs[owner]
            out.append((site, f"address-taken in {name} [{unit}] "
                              f"(+0x{site - owner:x})"))
            continue
        slot = ctx.vtable_of(site)
        if slot:
            vt, k = slot
            vrow = ctx.symbols.datas.get(vt)
            vname = f" {vrow[0]}" if vrow else ""
            out.append((site, f"vtable 0x{vt:x}{vname} slot {k}"))
            continue
        near = ctx.symbols.nearest_data(site)
        section = ctx.image.section_of(site)
        where = section.name if section else "?"
        if near:
            _start, name, delta = near
            out.append((site, f"[{where}] ~{name}+0x{delta:x}"))
        else:
            out.append((site, f"[{where}]"))
    return out


def _print_data_refs(ctx, target: int, indent: str = "  ") -> None:
    refs = _data_refs(ctx, target)
    if not refs:
        return
    print(f"{indent}-- referenced as data (fn-ptr table / vtable slot / "
          "address-taken):")
    for site, description in refs[:16]:
        print(f"{indent}   @0x{site:08x}  {description}")
    if len(refs) > 16:
        print(f"{indent}   ... (+{len(refs) - 16} more)")


def _callers_flat(ctx, targets, raw: bool) -> None:
    for t in targets:
        print(f"\n==== callers of {_label(ctx, t)} ====")
        sites = ctx.call_index.get(t, [])
        if not sites:
            print("  (no direct call/jmp rel32 caller in .text)")
            _print_data_refs(ctx, t)
            continue
        seen = set()
        for site, op in sites:
            kind = "call" if op == 0xE8 else "jmp "
            owner = ctx.symbols.owner(site)
            if owner is None:
                if raw:
                    print(f"  {kind} @0x{site:08x}  (unowned site)")
                elif ("unowned", site) not in seen:
                    seen.add(("unowned", site))
                    print(f"  {kind} @0x{site:08x}  (unowned site - "
                          "padding/data in .text)")
                continue
            name, unit, _sz, _p = ctx.symbols.funcs[owner]
            if raw:
                print(f"  {kind} @0x{site:08x}  in 0x{owner:08x} {name} [{unit}]")
            elif owner not in seen:
                seen.add(owner)
                print(f"  {kind} in 0x{owner:08x} {name} [{unit}]")
        _print_data_refs(ctx, t)


def _caller_tree(ctx, targets, depth_cap: int) -> None:
    def effective_callers(rva):
        for site, op in ctx.call_index.get(rva, []):
            owner = ctx.symbols.owner(site)
            if owner == rva:
                continue  # intra-fn / self
            yield owner, op, site

    for t in targets:
        print(f"\n==== caller tree of {_label(ctx, t)} ====")
        seen = set()

        def walk(rva, depth):
            if depth_cap and depth > depth_cap:
                print("  " * (depth + 1) + "... (--depth cap)")
                return
            owners, uniq = [], set()
            for owner, op, site in effective_callers(rva):
                key = (owner, op) if owner is not None else ("unowned", site)
                if key in uniq:
                    continue
                uniq.add(key)
                owners.append((owner, op, site))
            if not owners:
                if depth == 0:
                    print("  (no direct call/jmp rel32 caller in .text)")
                return
            for owner, op, site in owners:
                kind = "call" if op == 0xE8 else "jmp "
                pad = "  " * (depth + 1)
                if owner is None:
                    print(pad + f"<- {kind} (unowned site @0x{site:08x})")
                    continue
                if owner in seen:
                    print(pad + f"<- {kind} {_label(ctx, owner)} (*seen)")
                    continue
                seen.add(owner)
                tag = _frontier_tag(ctx, owner)
                if tag is None:
                    print(pad + f"<- {kind} {_label(ctx, owner)}")
                    walk(owner, depth + 1)
                else:
                    print(pad + f"<- {kind} {_label(ctx, owner)}  "
                                f"[{tag} - frontier]")

        walk(t, 0)
        _print_data_refs(ctx, t)


def _callees(ctx, targets) -> None:
    section, blob = ctx.text
    for t in targets:
        row = ctx.symbols.funcs.get(t)
        if row is None:
            print(f"\n==== callees of 0x{t:08x} ====\n  (not a function start)")
            continue
        name, _unit, size, _p = row
        end = t + size if size else t + 0x400
        print(f"\n==== callees of {_label(ctx, t)}  "
              f"(span 0x{t:x}..0x{end:x}) ====")
        body = blob[t - section.rva:end - section.rva]
        seen = set()
        indirect = 0
        for i in range(max(0, len(body) - 4)):
            if body[i] in (0xE8, 0xE9):
                rel = struct.unpack_from("<i", body, i + 1)[0]
                tgt = t + i + 5 + rel
                if (section.rva <= tgt < section.rva + section.size
                        and tgt not in seen):
                    seen.add(tgt)
                    print(f"  -> {_label(ctx, tgt)}")
            elif body[i] == 0xFF and i + 1 < len(body) \
                    and ((body[i + 1] >> 3) & 7) in (2, 4):
                indirect += 1  # call/jmp r/m32 - vtable/IAT/fn-ptr dispatch
        if not seen:
            print("  (no direct call/jmp rel32 callee)")
        if indirect:
            print(f"  (~{indirect} indirect call/jmp site(s) - "
                  "vtable/IAT/fn-ptr; invisible to rel32 xref)")


def run(args) -> None:
    if args.raw and not args.flat:
        die("--raw only modifies --flat (the tree and --callees have no "
            "per-site rendering)")
    ctx = get_context()
    targets = []
    for arg in args.target:
        rva = ctx.symbols.resolve(arg)
        if rva not in ctx.symbols.funcs:
            owner = ctx.symbols.owner(rva)
            if owner is not None:
                print(f"[0x{rva:x} is +0x{rva - owner:x} into "
                      f"{ctx.symbols.funcs[owner][0]} - using the owner]")
                rva = owner
        targets.append(rva)
    if args.callees:
        _callees(ctx, targets)
    elif args.flat:
        _callers_flat(ctx, targets, raw=args.raw)
    else:
        _caller_tree(ctx, targets, depth_cap=args.depth)
