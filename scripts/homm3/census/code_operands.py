"""Absolute operand sites reached from independently recovered code starts."""
import struct

from capstone import x86

from homm3.census.find_relocs import Site


def _reached(census):
    return sorted(set().union(*census.reached.values())) if census.reached else []


def sites(census):
    """Supplement linear disassembly after embedded data disrupts its alignment.

    The function census supplies instruction boundaries. A four-byte encoded
    operand must equal the decoded absolute address inside the image, the
    linear decoder's own predicate; relative branches and register-relative
    displacements stay excluded. Jump tables the descent bounded
    (`jmp [4*reg+TABLE]`) contribute their in-text entries, as
    `find_relocs.code_sites` walks them.
    """
    image = census.image
    out = {}
    for address in _reached(census):
        ins = census.insn(address)
        for op in ins.operands:
            if op.type == x86.X86_OP_IMM and ins.imm_size == 4:
                offset, value = ins.imm_offset, op.imm & 0xFFFFFFFF
            elif op.type == x86.X86_OP_MEM and ins.disp_size == 4 and op.mem.base == 0:
                offset, value = ins.disp_offset, op.mem.disp & 0xFFFFFFFF
            else:
                continue
            if not image.in_image(value):
                continue
            if struct.unpack_from('<I', ins.bytes, offset)[0] != value:
                continue
            site = address + offset
            out[site] = Site(site, value, 'code', ins.mnemonic)
    for table, (_owner, size, kind) in sorted(census.tables.items()):
        if kind != 'jump':
            continue
        for entry in range(table, table + size - 3, 4):
            value = census.dword(entry)
            if value is None or not census.in_text(value - image.image_base):
                break
            out.setdefault(entry, Site(entry, value, 'switch', f'table@0x{table:x}'))
    return out


def misaligned(census, linear):
    """Linear code sites that split an instruction the descent decoded.

    After embedded data a linear decoder can resynchronise mid-instruction
    and report four bytes that straddle real operands. Where the function
    census decoded those bytes, only a four-byte immediate or displacement
    field of that instruction is an operand site.
    """
    covered, fields = set(), set()
    for address in _reached(census):
        ins = census.insn(address)
        covered.update(range(address, address + ins.size))
        if ins.imm_size == 4:
            fields.add(address + ins.imm_offset)
        if ins.disp_size == 4:
            fields.add(address + ins.disp_offset)
    return {rva for rva, site in linear.items()
            if site.channel == 'code' and rva not in fields
            and any(byte in covered for byte in range(rva, rva + 4))}
