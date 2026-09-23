"""Prove comparison-address operands without treating them as memory accesses.

The caller supplies an independently anchored raw COFF code contribution. This
module checks its instruction boundaries and relocation field; it never supplies
an address from initializer bytes, expands storage, or proves loop/access bounds.
"""
from collections import deque

import capstone as cs
from capstone import x86_const as x86

from homm3.build.canonicalize_data_symbols import RELOCATION_WIDTHS


class Roles:
    def __init__(self):
        self.cache = {}

    def graph(self, obj, ordinal):
        key = (obj, ordinal)
        if key in self.cache:
            return self.cache[key]
        section = obj.sections[ordinal-1]
        raw = obj.section_bytes(section)
        entries = {s.value: s.index for s in reversed(list(obj.symbols.values()))
                   if s.section == ordinal and s.typ & 0x20 and 0 <= s.value < len(raw)}
        decoded, previous, origins, occupied = {}, {}, {}, {}
        pending = deque((offset, None, symbol) for offset, symbol in sorted(entries.items()))
        dis = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
        dis.detail = True
        bad = ''
        relocations = [r for r in obj.relocations if r.section == ordinal]
        while pending:
            offset, parent, origin = pending.popleft()
            if offset in decoded or not 0 <= offset < len(raw):
                continue
            choices = list(dis.disasm(raw[offset:offset+15], offset, count=1))
            if not choices:
                continue
            ins = choices[0]
            if any(i in occupied for i in range(offset, offset+ins.size)):
                bad = 'overlapping instruction decodes'
                break
            encoded = {offset+start: width for start, width in (
                (ins.imm_offset, ins.imm_size), (ins.disp_offset, ins.disp_size)) if width}
            fields = {}
            for ref in relocations:
                width = RELOCATION_WIDTHS.get(ref.typ, 4)
                if ref.site < offset+ins.size and offset < ref.site+width:
                    if (ref.typ not in (6, 7, 20) or width != 4 or
                            encoded.get(ref.site) != 4 or ref.site in fields):
                        bad = 'relocation does not occupy one encoded instruction field'
                        break
                    fields[ref.site] = ref
            if bad:
                break
            decoded[offset], previous[offset], origins[offset] = ins, parent, origin
            for i in range(offset, offset+ins.size):
                occupied[i] = offset
            loop = ins.id in (x86.X86_INS_LOOP, x86.X86_INS_LOOPE, x86.X86_INS_LOOPNE,
                              x86.X86_INS_JCXZ, x86.X86_INS_JECXZ)
            if ins.group(cs.CS_GRP_JUMP) or loop:
                if len(ins.operands) == 1 and ins.operands[0].type == x86.X86_OP_IMM:
                    target = ins.operands[0].imm
                    branch = fields.get(offset+ins.imm_offset)
                    if branch:
                        symbol = obj.symbols[branch.symbol_index]
                        target = (symbol.value+int.from_bytes(raw[branch.site:branch.site+4],
                                  'little', signed=True)) if branch.typ == 20 and symbol.section == ordinal else None
                    if target is not None:
                        pending.append((target, offset, origin))
                if ins.id in (x86.X86_INS_JMP, x86.X86_INS_LJMP):
                    continue
            elif (any(ins.group(g) for g in (cs.CS_GRP_RET, cs.CS_GRP_IRET, cs.CS_GRP_INT)) or
                  ins.id in (x86.X86_INS_HLT, x86.X86_INS_UD2)):
                continue
            pending.append((offset+ins.size, offset, origin))
        result = dict(raw=raw, instructions=decoded, previous=previous,
                      origins=origins, occupied=occupied, error=bad)
        self.cache[key] = result
        return result

    def inspect(self, obj, ordinal, site):
        result = dict(role='unproved', reason='', section_ordinal=ordinal, site_offset=site)
        if (int.from_bytes(obj.data[:2], 'little') != 0x14c or not 1 <= ordinal <= len(obj.sections) or
                not obj.sections[ordinal-1].characteristics & 0x20):
            return dict(result, reason='not i386 code')
        refs = [r for r in obj.relocations if r.section == ordinal and r.site == site]
        if len(refs) != 1 or refs[0].typ != 6:
            return dict(result, reason='not one DIR32 relocation')
        overlapping = [r for r in obj.relocations if r.section == ordinal and r is not refs[0] and
                       r.site < site+4 and site < r.site+RELOCATION_WIDTHS.get(r.typ, 4)]
        if overlapping:
            return dict(result, reason='overlapping relocations')
        graph = self.graph(obj, ordinal)
        if graph['error']:
            return dict(result, reason=graph['error'])
        start = graph['occupied'].get(site)
        ins = graph['instructions'].get(start)
        if ins is None:
            return dict(result, reason='no instruction path from an emitted function entry')
        trail, at = [], start
        while at is not None:
            trail.append(at)
            at = graph['previous'][at]
        result.update(instruction_offset=start, instruction_bytes=bytes(ins.bytes).hex(),
                      instruction=ins.mnemonic+' '+ins.op_str,
                      entry_symbol_index=graph['origins'][start], instruction_path=trail[::-1],
                      relocation_type=refs[0].typ, relocation_symbol_index=refs[0].symbol_index,
                      addend=int.from_bytes(graph['raw'][site:site+4], 'little', signed=True))
        if (ins.id == x86.X86_INS_CMP and len(ins.operands) == 2 and
                ins.operands[0].type == x86.X86_OP_REG and ins.operands[0].size == 4 and
                ins.operands[1].type == x86.X86_OP_IMM and ins.imm_size == 4 and
                start+ins.imm_offset == site):
            return dict(result, role='comparison-immediate', reason='', operand_index=1,
                        statement='address comparison only; no memory access or loop-bound proof')
        if ins.disp_size and start+ins.disp_offset == site and any(
                op.type == x86.X86_OP_MEM for op in ins.operands):
            return dict(result, role='address-formation' if ins.id == x86.X86_INS_LEA else 'memory-displacement',
                        reason='outside reference needs separate access/address-flow proof')
        return dict(result, reason='relocation is not a supported comparison immediate')
