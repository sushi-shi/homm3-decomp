"""Absolute operand sites reached from independently recovered code starts."""
import struct

from capstone import x86

from homm3.census.find_relocs import Site


def sites(census):
    """Supplement linear disassembly after embedded data disrupts its alignment.

    The function census supplies instruction boundaries. A four-byte encoded
    operand must equal the decoded absolute address and point to a mapped
    section; relative branches and register-relative displacements stay excluded.
    """
    image = census.image
    out = {}
    reached = set().union(*census.reached.values()) if census.reached else set()
    for address in sorted(reached):
        ins = census.insn(address)
        for op in ins.operands:
            if op.type == x86.X86_OP_IMM and ins.imm_size == 4:
                offset, value = ins.imm_offset, op.imm & 0xFFFFFFFF
            elif op.type == x86.X86_OP_MEM and ins.disp_size == 4 and op.mem.base == 0:
                offset, value = ins.disp_offset, op.mem.disp & 0xFFFFFFFF
            else:
                continue
            if image.section_of(value - image.image_base) is None:
                continue
            if struct.unpack_from('<I', ins.bytes, offset)[0] != value:
                continue
            site = address + offset
            out[site] = Site(site, value, 'code', ins.mnemonic)
    return out
