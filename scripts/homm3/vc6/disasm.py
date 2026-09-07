"""Labeled, hash-gated compiler assembly: ``homm3 vc6 disasm``.

Inferred roles live beside their evidence in docs/vc6, using explicit
``c2-role`` annotations. They are display overlays, never original symbols,
game claims, or edits to the Ghidra database. Ghidra supplies instruction and
reference boundaries; every displayed instruction is checked against C2.DLL.
"""
from __future__ import annotations

from dataclasses import dataclass
import re

from homm3.vc6 import _common

IMAGE_BASE = 0x10700000
_ROLE = re.compile(
    r"<!-- c2-role: (function|global|site) (0x[0-9a-fA-F]+) "
    r"([A-Za-z_][A-Za-z_0-9]*) -->")


@dataclass(frozen=True)
class Role:
    rva: int
    kind: str
    name: str
    evidence: str


@dataclass(frozen=True)
class Reference:
    rva: int
    kind: str
    name: str = ""


@dataclass(frozen=True)
class Instruction:
    rva: int
    raw: bytes
    text: str
    references: tuple[Reference, ...] = ()


def load_roles(root=None) -> dict[int, Role]:
    """The owning prose supplies role names; no parallel symbol inventory."""
    root = root or _common.REPO
    roles, names = {}, set()
    for path in sorted((root / "docs/vc6").glob("*.md")):
        for line_number, line in enumerate(path.read_text().splitlines(), 1):
            if not line.startswith("<!-- c2-role:"):
                continue
            match = _ROLE.fullmatch(line)
            evidence = f"{path.relative_to(root)}:{line_number}"
            if not match:
                raise ValueError(f"malformed C2 role at {evidence}")
            kind, address, name = match.groups()
            rva = int(address, 16)
            if rva in roles or name in names:
                raise ValueError(f"duplicate C2 role at {evidence}: {address} {name}")
            roles[rva] = Role(rva, kind, name, evidence)
            names.add(name)
    return roles


def resolve_selector(selector: str, roles, symbols=()) -> int:
    matches = {r.rva for r in roles.values() if r.name == selector}
    matches.update(rva for rva, name in symbols if name == selector)
    if matches:
        if len(matches) != 1:
            raise ValueError(f"ambiguous C2 selector {selector!r}")
        return matches.pop()
    try:
        value = int(selector, 16)
    except ValueError:
        raise ValueError(f"unknown C2 selector {selector!r}; use a role, Ghidra name, RVA or VA") from None
    rva = value - IMAGE_BASE if value >= IMAGE_BASE else value
    if not 0 <= rva < 0x100000000 - IMAGE_BASE:
        raise ValueError(f"C2 address {selector!r} is outside the 32-bit image address space")
    return rva


def local_range(spec: str) -> tuple[int, int]:
    try:
        lo, hi = (int(s.lstrip("+"), 16) for s in spec.split(":"))
    except ValueError:
        raise ValueError("--range requires two hexadecimal offsets, e.g. +0:+0x80") from None
    if not 0 <= lo < hi:
        raise ValueError("--range must be nonnegative, nonempty and end-exclusive")
    return lo, hi


def render(rows, roles, *, verbose=False, title=None, focus=()):
    """Label actual references, never arbitrary address-looking immediates.

    A target is either an exact role, a local branch label, or Ghidra's exact
    name/address. In particular, proximity never transfers a function role to
    an unrelated cold block (C2's Ghidra bodies are fragmented).
    """
    rows = list(rows)
    sites = {row.rva for row in rows}
    targets = {ref.rva for row in rows for ref in row.references
               if ref.kind in ("call", "jump") and ref.rva in sites}
    used = set(focus) & roles.keys()
    lines = [title] if title else []
    for row in rows:
        if row.rva in roles:
            role = roles[row.rva]
            used.add(row.rva)
            lines.append(f"{role.name}:  ; inferred {role.kind}")
        elif row.rva in targets:
            lines.append(f"loc_{row.rva:06x}:")
        text, notes = row.text, []
        for ref in row.references:
            role = roles.get(ref.rva)
            if role:
                name = role.name
                used.add(ref.rva)
            elif ref.rva in targets:
                name = f"loc_{ref.rva:06x}"
            else:
                name = ref.name or f"rva_0x{ref.rva:x}"
            va = IMAGE_BASE + ref.rva
            text = re.sub(rf"(?<![\w])0x{va:x}(?![\w])", name, text,
                          flags=re.IGNORECASE)
            notes.append(f"{ref.kind} {name} (rva 0x{ref.rva:x})")
        raw = f"{row.raw.hex(' '):<29} " if verbose else ""
        suffix = "  ; " + "; ".join(dict.fromkeys(notes)) if notes else ""
        lines.append(f"  {row.rva:06x}  {raw}{text}{suffix}")
    if used:
        lines.append("; Inferred roles, not original compiler symbols:")
        for rva in sorted(used):
            role = roles[rva]
            lines.append(f"; {role.name} [0x{rva:x}, {role.kind}] — {role.evidence}")
    return "\n".join(lines) + "\n"


def run(args) -> int:
    from homm3.vc6 import atlas, _toolchain
    try:
        if args.refs and args.range:
            raise ValueError("--refs and --range select different views")
        roles = load_roles()
        span = local_range(args.range) if args.range else None
        binary = _toolchain.Binary("C2.DLL")
        import_c2 = atlas._load_script("import_c2")
        backend = atlas._load_script("c2_disasm")
        project, program = import_c2.open_program()
        try:
            return backend.run(program, binary, roles, args, span)
        finally:
            import_c2.close_program(project, program)
    except ValueError as exc:
        _common.die(str(exc))
