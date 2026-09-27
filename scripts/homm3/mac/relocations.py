"""Resolve reviewed same-section PowerPC calls and MWLink reload-slot collapse.

MWLinkPPC 2.4's ``-collapsereloads on`` removes the NOP reserved after a direct
call when no RTOC reload is needed. Branches within the hunk must be rebased
after removing those slots. No relocation or instruction bits are masked.
"""
from __future__ import annotations

from dataclasses import dataclass

from homm3.mac.object import CodeHunk, ObjectError


@dataclass(frozen=True)
class Address:
    section: int
    offset: int


@dataclass(frozen=True)
class CallTarget:
    address: Address
    # These two CFM glue forms change r2. The caller's reserved NOP must
    # become a TOC restore, even though the branch reaches the same section.
    kind: str = "direct"
    import_identity: str | None = None

    @property
    def restores_toc(self) -> bool:
        return self.kind in ("indirect_tvector", "import")


def call_target(value: Address | CallTarget) -> CallTarget:
    return value if isinstance(value, CallTarget) else CallTarget(value)


@dataclass(frozen=True)
class ResolvedCall:
    object_offset: int
    linked_offset: int
    symbol: str
    section: int
    target_offset: int
    kind: str = "direct"


@dataclass(frozen=True)
class JumpTable:
    object_offsets: tuple[int, ...]
    target_entries: tuple[int, ...]
    code_section_base: int = 0


@dataclass(frozen=True)
class TocBinding:
    displacement: int
    target: Address
    indirect: bool
    address_load: bool = False
    declaration_only: bool = False
    jump_table: JumpTable | None = None


@dataclass(frozen=True)
class ResolvedData:
    object_offset: int
    linked_offset: int
    symbol: str
    section: int
    target_offset: int
    toc_displacement: int
    indirect: bool
    address_load: bool = False
    declaration_only: bool = False


@dataclass(frozen=True)
class ResolvedJumpTable:
    symbol: str
    section: int
    target_offset: int
    candidate_entries: tuple[int, ...]
    target_entries: tuple[int, ...]
    matching_bytes: int
    exact: bool

    @property
    def size(self) -> int:
        return 4 * len(self.target_entries)


@dataclass(frozen=True)
class LinkedCode:
    data: bytes
    calls: tuple[ResolvedCall, ...]
    removed_reload_slots: tuple[int, ...]
    data_references: tuple[ResolvedData, ...] = ()
    restored_reload_slots: tuple[int, ...] = ()
    jump_tables: tuple[ResolvedJumpTable, ...] = ()


def _signed(value: int, bits: int) -> int:
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def _displacement(word: int, delta: int, bits: int) -> int:
    if delta % 4 or not -(1 << (bits - 1)) <= delta < (1 << (bits - 1)):
        raise ObjectError(f"PowerPC {bits}-bit branch displacement out of range: {delta:#x}")
    mask = ((1 << bits) - 1) & ~3
    return (word & ~mask) | (delta & mask)


def link_code(hunk: CodeHunk, origin: Address, symbols: dict[str, Address | CallTarget], *,
              collapse_reloads: bool, toc: dict[str, TocBinding] | None = None) -> LinkedCode:
    """Resolve reviewed calls/TOC loads; reject unknown symbols and fixup forms."""
    if len(hunk.data) % 4 or origin.offset % 4:
        raise ObjectError("unaligned PowerPC code")
    words = [int.from_bytes(hunk.data[i:i + 4], "big")
             for i in range(0, len(hunk.data), 4)]
    refs = {}
    data_refs = {}
    removed = set()
    restored = set()
    for offset, kind, symbol in hunk.xrefs:
        if offset % 4 or not 0 <= offset < len(hunk.data) or offset in refs or offset in data_refs:
            raise ObjectError(f"{hunk.name}: invalid/duplicate relocation at {offset:#x}")
        if kind in ("HUNK_XREF_16BIT_IL", "HUNK_XREF_16BIT"):
            binding = (toc or {}).get(symbol)
            word = words[offset // 4]
            if (binding is None or (binding.indirect and binding.address_load)
                    or (binding.indirect or binding.address_load) != (kind == "HUNK_XREF_16BIT_IL")):
                raise ObjectError(f"{hunk.name}: unresolved TOC reference {symbol!r}")
            if (word & 0x1fffff != 0x20000 or
                    word >> 26 not in ((32,) if kind == "HUNK_XREF_16BIT_IL" else (14, 32, 34, 48, 50))):
                raise ObjectError(f"{hunk.name}: unsupported TOC opcode/addend at {offset:#x}")
            if not -0x8000 <= binding.displacement < 0x8000:
                raise ObjectError(f"{hunk.name}: TOC displacement out of range")
            data_refs[offset] = (symbol, binding)
            continue
        if kind != "HUNK_XREF_24BIT" or not symbol or symbol not in symbols:
            raise ObjectError(f"{hunk.name}: unresolved relocation {kind} {symbol!r} at {offset:#x}")
        callee = call_target(symbols[symbol])
        target = callee.address
        if target.section != origin.section or target.offset % 4:
            raise ObjectError(f"{hunk.name}: {symbol} needs unsupported cross-section call glue")
        word = words[offset // 4]
        if word & 0xFFFFFFFE != 0x48000000:
            raise ObjectError(f"{hunk.name}: unsupported branch opcode/addend at {offset:#x}")
        refs[offset] = (symbol, callee)
        if callee.restores_toc:
            if not word & 1 or offset + 4 >= len(hunk.data) or words[offset // 4 + 1] != 0x60000000:
                raise ObjectError(f"{hunk.name}: TOC-changing call lacks reserved reload slot at {offset:#x}")
            restored.add(offset + 4)
        elif (collapse_reloads and word & 1 and offset + 4 < len(hunk.data)
                and words[offset // 4 + 1] == 0x60000000):
            removed.add(offset + 4)
    if (removed | restored).intersection(refs.keys() | data_refs.keys()):
        raise ObjectError(f"{hunk.name}: relocation owns a rewritten reload slot")

    offsets = {}
    new_offset = 0
    for offset in range(0, len(hunk.data) + 1, 4):
        if offset not in removed:
            offsets[offset] = new_offset
            new_offset += 4

    result = bytearray()
    calls = []
    data_references = []
    for old_offset, word in enumerate(words):
        old_offset *= 4
        if old_offset in removed:
            continue
        new_offset = offsets[old_offset]
        if old_offset in refs:
            symbol, callee = refs[old_offset]
            target = callee.address
            word = _displacement(word, target.offset - origin.offset - new_offset, 26)
            calls.append(ResolvedCall(old_offset, new_offset, symbol,
                                      target.section, target.offset, callee.kind))
        elif old_offset in restored:
            word = 0x80410014  # lwz r2,20(r1), verified against MWLinkPPC
        elif old_offset in data_refs:
            symbol, binding = data_refs[old_offset]
            if binding.address_load:
                # MWLink relaxes an IL-marked lwz to addi when the addressed
                # RW/RO object is in signed-16-bit reach of the TOC anchor.
                word = (word & 0x03ffffff) | (14 << 26)
            word = (word & 0xffff0000) | (binding.displacement & 0xffff)
            data_references.append(ResolvedData(old_offset, new_offset, symbol,
                                                binding.target.section, binding.target.offset,
                                                binding.displacement, binding.indirect,
                                                binding.address_load, binding.declaration_only))
        elif word >> 26 in (16, 18) and not word & 2:
            bits = 16 if word >> 26 == 16 else 26
            mask = ((1 << bits) - 1) & ~3
            target = old_offset + _signed(word & mask, bits)
            if target not in offsets:
                raise ObjectError(f"{hunk.name}: branch to unmapped hunk offset {target:#x}")
            word = _displacement(word, offsets[target] - new_offset, bits)
        result.extend(word.to_bytes(4, "big"))
    tables = []
    for symbol, binding in sorted((toc or {}).items()):
        table = binding.jump_table
        if table is None:
            continue
        entries = []
        for at in table.object_offsets:
            if at not in offsets or at >= len(hunk.data):
                raise ObjectError(f"{hunk.name}: jump table {symbol!r} targets an unmapped instruction {at:#x}")
            value = table.code_section_base + origin.offset + offsets[at]
            if not 0 <= value <= 0xffffffff:
                raise ObjectError(f"{hunk.name}: jump table address overflows 32 bits")
            entries.append(value)
        candidate = b"".join(value.to_bytes(4, "big") for value in entries)
        target = b"".join(value.to_bytes(4, "big") for value in table.target_entries)
        tables.append(ResolvedJumpTable(symbol, binding.target.section, binding.target.offset,
                                         tuple(entries), table.target_entries,
                                         sum(a == b for a, b in zip(candidate, target)),
                                         candidate == target))
    return LinkedCode(bytes(result), tuple(calls), tuple(sorted(removed)),
                      tuple(data_references), tuple(sorted(restored)), tuple(tables))
