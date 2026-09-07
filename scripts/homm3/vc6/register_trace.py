"""Readable, verified snapshots of C2's temporary register-binding stores."""
from homm3.core import undname
from homm3.vc6 import disasm

REGISTERS = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")


def format_observations(rows: list[str]) -> str:
    symbols = {}
    for row in rows:
        if row.startswith("sym "):
            _, address, name = row.split(" ", 2)
            symbols[address] = name
    names = undname.demangle(symbols.values())
    roles = disasm.load_roles()

    def value_label(value):
        if value in ("free", "unreadable"):
            return value
        category, handle = value.split(":")
        # Category 3 is an expression temporary; its handle is local to
        # this compiler run/function, not an original C++ variable name.
        kind = "temporary" if category == "3" else f"category-{category} value"
        return f"{kind} #{int(handle, 16):x}"

    lines = [
        "Verified passive temporary-register observations.",
        "Only the two documented binding stores are observed; this is not a complete allocator trace.",
        "Handles identify compiler values within this function/run, not original source variables.",
        "Each snapshot precedes the store; unlisted binding-table slots are empty at that instant.",
        f"Observed stores: {sum(row.startswith('register ') for row in rows)}",
    ]
    previous_root = None
    count = 0
    for row in rows:
        if not row.startswith("register "):
            continue
        fields = dict(part.split("=", 1) for part in row.split()[1:])
        root = fields["root"]
        if root != previous_root:
            name = symbols.get(root, f"<unresolved function {root}>")
            lines.extend(["", f"Function: {names.get(name, name)}"])
            previous_root = root
        site = int(fields["site"], 16)
        role = roles.get(site)
        label = role.name if role else "unlabeled store"
        count += 1
        lines.append(f"#{count} {fields['selected']} <- {value_label(fields['value'])}"
                     f"  at {label} (C2 RVA 0x{site:x})")
        occupied = [f"{reg}={value_label(fields[reg])}" for reg in REGISTERS
                    if fields[reg] != "free"]
        lines.append("  Binding table before: " + ("; ".join(occupied) or "empty"))
        if role:
            lines.append(f"  Inferred site: {role.evidence}")
    return "\n".join(lines) + "\n"


def run(args) -> int:
    from homm3.vc6.shim import build
    return build.runRegisterTrace(args.unit, args.fn)
