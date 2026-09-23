"""Admit reviewed function-local switch tables without pinning MWOB counters.

The current form is deliberately bounded: absolute 32-bit entries, every entry
relocated to this function, and one unambiguous emitted table of the reviewed
extent. MWLink reload removal is applied to the candidate labels by link_code;
the resulting table bytes participate in the function's matching verdict.
"""
from __future__ import annotations

import hashlib
from pathlib import Path

from homm3.mac.loader import Loader
from homm3.mac.object import CodeHunk, DataHunk, ObjectError
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, JumpTable, TocBinding
from homm3.mac.source import data_rows, load_pairs


def bindings(root: Path, pef: PEF, loader: Loader, code: CodeHunk,
             hunks: tuple[DataHunk, ...], *, unit: str | None,
             retail_va: int | None) -> dict[str, TocBinding]:
    rows = [row for row in data_rows(root, "jump_tables")
            if row.get("owner_va") == retail_va and row.get("unit") == unit]
    if not rows:
        return {}
    owners = [pair for pair in load_pairs(root)
              if pair.retail_va == retail_va and pair.unit == unit and pair.mac_symbol == code.name]
    if len(owners) != 1:
        raise ObjectError("jump table needs one admitted owning function")
    owner = owners[0]
    toc = loader.toc()
    references = {name for _, kind, name in code.xrefs if kind == "HUNK_XREF_16BIT_IL"}
    result = {}
    spans = []
    for row in rows:
        if not isinstance(row.get("evidence"), str) or not row["evidence"].strip():
            raise ObjectError("Mac jump table lacks pairing evidence")
        fields = ("mac_section", "mac_offset", "mac_size", "reference_offset")
        if any(type(row.get(field)) is not int for field in fields):
            raise ObjectError("Mac jump table needs integer span and reference offsets")
        section, offset, size, reference = (row[field] for field in fields)
        target = Address(section, offset)
        if (offset % 4 or size <= 0 or size % 4 or pef.section(section).kind not in (1, 2)
                or reference % 4 or not 0 <= reference < owner.mac_size):
            raise ObjectError("invalid reviewed Mac jump-table span/reference")
        if any(section == other and offset < end and start < offset + size
               for other, start, end in spans):
            raise ObjectError("overlapping reviewed Mac jump tables")
        spans.append((section, offset, offset + size))
        payload = pef.read(section, offset, size)
        if hashlib.sha256(payload).hexdigest() != row.get("sha256"):
            raise ObjectError("reviewed Mac jump-table payload changed")
        instruction = int.from_bytes(pef.code(owner.mac_section, owner.mac_offset + reference, 4), "big")
        if instruction >> 26 != 32 or (instruction >> 16) & 31 != 2:
            raise ObjectError("reviewed jump-table reference is not an RTOC pointer load")
        displacement = instruction & 0xffff
        if displacement & 0x8000:
            displacement -= 0x10000
        if loader.pointers.get(Address(toc.section, toc.offset + displacement)) != target:
            raise ObjectError("retail jump-table TOC load does not select its reviewed span")
        for at in range(0, size, 4):
            destination = loader.pointers.get(Address(section, offset + at))
            if (not isinstance(destination, Address) or destination.section != owner.mac_section
                    or destination.offset % 4
                    or not owner.mac_offset <= destination.offset < owner.mac_offset + owner.mac_size):
                raise ObjectError("retail jump-table entry is not a relocated instruction in its owner")
        values = [hunk for hunk in hunks
                  if hunk.name in references and hunk.name.startswith("@")
                  and hunk.storage_class in ("RW", "RO") and len(hunk.data) == size
                  and hunk.xrefs]
        if len(values) != 1:
            raise ObjectError(f"reviewed jump table has {len(values)} candidate tables of its extent")
        value = values[0]
        expected = tuple((at, "HUNK_XREF_32BIT", code.name) for at in range(0, size, 4))
        if tuple(sorted(value.xrefs)) != expected:
            raise ObjectError("candidate jump table must relocate every word to its owning function")
        addends = tuple(int.from_bytes(value.data[at:at + 4], "big") for at in range(0, size, 4))
        if any(at % 4 or not 0 <= at < len(code.data) for at in addends):
            raise ObjectError("candidate jump table points outside its owner's instructions")
        cells = [hunk for hunk in hunks if hunk.name == value.name and hunk.storage_class == "TC"]
        if (len(cells) != 1 or cells[0].data != bytes(4)
                or cells[0].xrefs != ((0, "HUNK_XREF_32BIT", value.name),)):
            raise ObjectError("candidate jump table lacks its MWOB TOC pointer cell")
        if value.name in result:
            raise ObjectError("ambiguous candidate-to-retail jump-table pairing")
        entries = tuple(int.from_bytes(payload[at:at + 4], "big") for at in range(0, size, 4))
        result[value.name] = TocBinding(displacement, target, True,
                                        jump_table=JumpTable(addends, entries,
                                                             pef.section(owner.mac_section).default_address))
    return result
