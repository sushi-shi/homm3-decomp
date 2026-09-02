"""homm3.sema.disasm - one function's body, either side.

Sides:
  (default)  TARGET: the delinked unit object when one covers the
             function; otherwise the retail image bytes via capstone
             (any of the 11,943 functions - most of the game has no
             delinked unit yet)
  --base     your compiled object (delinked manifest units only), refreshed
             in place first (ninja target + normalize); --no-build skips that
  --source   your compiled object with verified VC6 /Z7 statement labels;
             implies --base (a compiler-generated body with no /Z7
             statements falls back to the unlabelled base listing)

Views (the default carries what matching reads; the columns are opt-in):
  (default)  address column + mnemonic/operands with every call and data
             symbol folded in from its reloc (`call ?f@@YAXXZ`, `[gVar]`,
             `push offset gVar`, `[4*ecx + $L1]`), cross-symbol <notes>,
             and the trailing switch/lookup pool as `dd`/`db` rows
  --verbose  the raw llvm-objdump rows: byte columns, IMAGE_REL reloc
             lines, <ownfn+0xNNN> notes - for encoding questions only
  --blocks   basic-block CFG view: skeleton lines by default,
             masked instruction bodies with --verbose
  --range    restrict any view to an end-exclusive function-local span;
             branches leaving the span render as external edges

rc: 0 = rendered, 2 = error.
"""
from __future__ import annotations

from homm3.sema import _asm
from homm3.sema import source as source_view
from homm3.sema._common import die
from homm3.sema.context import get_context


def run(args) -> None:
    ctx = get_context()
    name, unit, rva, size, ordinal = ctx.symbols.resolve_fn(args.target)

    if args.source and args.blocks:
        die("--source labels the linear candidate listing and does not combine "
            "with --blocks")
    if getattr(args, "target_side", False) and (args.base or args.source):
        die("--target is the retail side; it does not combine with --base/"
            "--candidate/--source")

    if args.base or args.source:
        obj = _asm.BASE / f"{unit}.obj"
        if unit and not getattr(args, "no_build", False):
            note = _asm.refresh_unit(unit)
            if note:
                print(note)
        if not obj.is_file():
            die(f"{name} [{unit or 'no unit'}] has no compiled base object - "
                "only manifest units (config/units.toml) compile")
        text = _asm.objdump(obj, name, ordinal)
        title = (f"[disasm BASE (compiled): {name}  "
                 f"build/objdiff/base/{unit}.obj]")
        if args.source:
            try:
                source_map = source_view.load(unit, name, ordinal, obj)
            except source_view.NoLineRecords as exc:
                print(f"[{exc}]")
                print("[no statements to label - showing the unlabelled "
                      "base listing]")
                args.source = False
            except source_view.SourceError as exc:
                die(str(exc))
            else:
                title = (f"[disasm BASE+SOURCE (compiled): {name}  "
                         f"build/objdiff/base/{unit}.obj + "
                         f"build/debug/{unit}.obj]")
    else:
        obj = _asm.TARGET / f"{unit}.c.obj"
        if obj.is_file():
            text = _asm.objdump(obj, name, ordinal)
            title = (f"[disasm TARGET (delinked): {name}  "
                     f"build/objdiff/target/{unit}.c.obj]")
        else:
            if not size:
                die(f"{name} has no recorded size - cannot carve its "
                    "image span")
            text = _asm.image_text(ctx, rva, size, name)
            title = (f"[disasm TARGET (image): {name} @ rva 0x{rva:x} "
                     f"(va 0x{ctx.image.image_base + rva:x}), {size} B]")

    if args.range:
        try:
            span = _asm.parse_local_range(args.range)
            text = _asm.slice_local_range(text, span)
        except ValueError as exc:
            die(f"invalid --range {args.range!r}: {exc}")
        title += f"  [local range {args.range}, end exclusive]"

    print(title)
    if args.source:
        print(source_view.render_disassembly(
            text, source_map, verbose=args.verbose), end="")
    elif args.blocks:
        print(_asm.blocks(text, skeleton=not args.verbose), end="")
    else:
        print(text if args.verbose else _asm.lite(text), end="")
