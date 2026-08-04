"""homm3.sema.disasm - one function's body, either side, lite by default.

Sides:
  (default)  TARGET: the delinked unit object when one covers the
             function; otherwise the retail image bytes via capstone
             (any of the 11,943 functions - most of the game has no
             delinked unit yet)
  --base     your compiled object (delinked manifest units only)

Views (lite is the default; the full columns are the opt-in):
  (default)  asm only - mnemonic+operands with <symbol> annotations
  --verbose  addresses + byte columns + reloc lines
  --blocks   basic-block CFG view: skeleton lines by default,
             masked instruction bodies with --verbose

rc: 0 = rendered, 2 = error.
"""
from __future__ import annotations

from homm3.sema import _asm
from homm3.sema._common import die
from homm3.sema.context import get_context


def run(args) -> None:
    ctx = get_context()
    name, unit, rva, size, ordinal = ctx.symbols.resolve_fn(args.target)

    if args.base:
        obj = _asm.BASE / f"{unit}.obj"
        if not obj.is_file():
            die(f"{name} [{unit or 'no unit'}] has no compiled base object - "
                "only manifest units (config/units.toml) compile")
        text = _asm.objdump(obj, name, ordinal)
        title = (f"[disasm BASE (compiled): {name}  "
                 f"build/objdiff/base/{unit}.obj]")
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

    print(title)
    if args.blocks:
        print(_asm.blocks(text, skeleton=not args.verbose), end="")
    else:
        print(text if args.verbose else _asm.lite(text), end="")
