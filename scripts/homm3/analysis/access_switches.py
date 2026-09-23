"""Reachable i386 control flow and bounded, embedded jump-table evidence.

Only an unsigned guard followed by an unchanged direct index or a zero-extended
index lookup can supply indirect edges. Tables and destinations must remain in
the supplied immutable code body. COFF fields require a caller's local-symbol
resolver; a placeholder word is never a linked pointer.
"""
from collections import defaultdict, deque

import capstone as cs
from capstone import x86_const as x86

from homm3.analysis.data_effects import ALIASES

MAX_CASES = 4096


def branch(ins):
    return ins.group(cs.CS_GRP_JUMP) or ins.id in (
        x86.X86_INS_LOOP, x86.X86_INS_LOOPE, x86.X86_INS_LOOPNE,
        x86.X86_INS_JCXZ, x86.X86_INS_JECXZ)


def graph(raw, start=0, *, resolve_field=None, validate_instruction=None):
    """Decode from entry and prove supported switch edges to a fixed point.

    resolve_field(site, width, role, decoded) returns a local address/literal,
    or None. Roles are branch, address, pointer and literal. The default reads
    linked machine code. A COFF provider must resolve local relocations for
    addresses/pointers, reject relocated literals, and resolve branch fields.
    """
    stop = start+len(raw)
    dis = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    dis.detail = True

    def field(site, width, role, decoded=None):
        if not start <= site < site+width <= stop:
            return None
        value = int.from_bytes(raw[site-start:site-start+width], 'little') if decoded is None else decoded
        return resolve_field(site, width, role, value) if resolve_field else value

    def direct(ins):
        return field(ins.address+ins.imm_offset, ins.imm_size, 'branch', ins.operands[0].imm)

    def walk(switches):
        instructions, edges, occupied, issues = {}, {}, {}, []
        pending = deque([start])
        visited = set()
        while pending:
            site = pending.popleft()
            if site in visited:
                continue
            visited.add(site)
            if not start <= site < stop:
                issues.append(dict(site=site, kind='undecoded-control-flow'))
                continue
            ins = next(dis.disasm(raw[site-start:site-start+15], site, count=1), None)
            if ins is None:
                issues.append(dict(site=site, kind='undecoded-control-flow'))
                continue
            if validate_instruction and not validate_instruction(ins):
                issues.append(dict(site=site, kind='malformed-instruction-relocation'))
                continue
            if any(pos in occupied for pos in range(site, site+ins.size)):
                issues.append(dict(site=site, kind='overlapping-control-flow'))
                continue
            instructions[site] = ins
            occupied.update((pos, site) for pos in range(site, site+ins.size))
            targets = []
            if branch(ins):
                if len(ins.operands) == 1 and ins.operands[0].type == x86.X86_OP_IMM:
                    target = direct(ins)
                    if target is not None and start <= target < stop:
                        targets.append(target)
                    else:
                        issues.append(dict(site=site, kind='external-tail-edge'))
                elif site in switches:
                    targets.extend(switches[site]['targets'])
                if ins.id not in (x86.X86_INS_JMP, x86.X86_INS_LJMP):
                    targets.append(site+ins.size)
            elif any(ins.group(group) for group in (cs.CS_GRP_RET, cs.CS_GRP_IRET, cs.CS_GRP_INT)) or ins.id in (
                    x86.X86_INS_HLT, x86.X86_INS_UD2):
                if not ins.group(cs.CS_GRP_RET):
                    issues.append(dict(site=site, kind='unsupported-control-termination'))
            else:
                targets.append(site+ins.size)
            edges[site] = list(dict.fromkeys(targets))
            pending.extend(edges[site])
        previous = defaultdict(set)
        previous[start].add(None)
        for site, targets in edges.items():
            for target in targets:
                previous[target].add(site)
        return dict(instructions=instructions, edges=edges, occupied=occupied,
                    previous=previous, issues=issues)

    def prove(flow, jump):
        insns, previous = flow['instructions'], flow['previous']
        trail = [jump]

        def preceding(ins):
            parents = previous[ins.address]
            parent = insns.get(next(iter(parents))) if len(parents) == 1 else None
            if parent is None or parent.address+parent.size != ins.address:
                return None
            trail.append(parent)
            return parent

        def register(op):
            return jump.reg_name(op.reg) if op.type == x86.X86_OP_REG and op.size == 4 else None

        if (jump.id != x86.X86_INS_JMP or len(jump.operands) != 1 or
                jump.operands[0].type != x86.X86_OP_MEM or jump.operands[0].size != 4 or jump.addr_size != 4):
            return None, 'not an indexed near jump'
        mem = jump.operands[0].mem
        if mem.segment or mem.base or not mem.index or mem.scale != 4 or jump.disp_size != 4:
            return None, 'not a dword table indexed by one register'
        index = jump.reg_name(mem.index)
        table = field(jump.address+jump.disp_offset, 4, 'address')
        if table is None:
            return None, 'unresolved target-table address'
        guard = preceding(jump)
        lookup = None
        input_register = index
        if guard and guard.mnemonic in ('mov', 'movzx') and len(guard.operands) == 2:
            load = guard
            destination, source = load.operands
            full, width, shift = ALIASES.get(load.reg_name(destination.reg), ('', 0, 0)) if destination.type == x86.X86_OP_REG else ('', 0, 0)
            if (source.type != x86.X86_OP_MEM or source.size not in (1, 2) or full != index or shift or
                    load.addr_size != 4 or source.mem.segment or load.disp_size != 4):
                return None, 'unsupported index lookup'
            registers = [(source.mem.base, 1), (source.mem.index, source.mem.scale)]
            registers = [(reg, scale) for reg, scale in registers if reg]
            if len(registers) != 1 or registers[0][1] != source.size:
                return None, 'lookup does not use one index scaled by its element width'
            input_register = load.reg_name(registers[0][0])
            if input_register == index:
                return None, 'lookup aliases its input and output registers'
            lookup = dict(site=load.address, width=source.size,
                          address=field(load.address+load.disp_offset, 4, 'address'))
            guard = preceding(load)
            if load.mnemonic == 'mov':
                if (width != source.size*8 or guard is None or guard.mnemonic != 'xor' or
                        len(guard.operands) != 2 or any(register(op) != index for op in guard.operands)):
                    return None, 'partial index lacks proved zero upper bits'
                guard = preceding(guard)
            elif width != 32:
                return None, 'lookup is not zero extended to 32 bits'
        if guard is None or guard.mnemonic not in ('ja', 'jae'):
            return None, 'missing adjacent unsigned guard'
        compare = preceding(guard)
        if (compare is None or compare.mnemonic != 'cmp' or len(compare.operands) != 2 or
                register(compare.operands[0]) != input_register or compare.operands[1].type != x86.X86_OP_IMM):
            return None, 'guard does not compare the unmodified input index'
        limit = field(compare.address+compare.imm_offset, compare.imm_size, 'literal', compare.operands[1].imm)
        count = (limit+int(guard.mnemonic == 'ja')) if limit is not None else None
        if count is None or not 0 < count <= MAX_CASES:
            return None, 'guard domain is unknown or exceeds the case limit'
        default = direct(guard)
        if default is None or compare.address <= default < jump.address+jump.size:
            return None, 'guard can enter the dispatch without the bound'
        lookup_values, spans = list(range(count)), []
        if lookup:
            address, width = lookup['address'], lookup['width']
            if address is None or not start <= address < address+count*width <= stop:
                return None, 'index lookup is outside the bounded code body'
            lookup_values = [field(address+i*width, width, 'literal') for i in range(count)]
            if any(value is None for value in lookup_values):
                return None, 'unresolved index-table bytes'
            lookup.update(count=count, values=lookup_values,
                          bytes=raw[address-start:address-start+count*width].hex())
            spans.append((address, address+count*width))
        entries = []
        for value in sorted(set(lookup_values)):
            address = table+value*4
            target = field(address, 4, 'pointer')
            if target is None or not start <= target < stop:
                return None, 'unresolved or nonlocal table target'
            if compare.address <= target < jump.address+jump.size:
                return None, 'table target enters the dispatch prefix'
            entries.append(dict(index=value, site=address, target=target,
                                bytes=raw[address-start:address-start+4].hex()))
            spans.append((address, address+4))
        for i, (a, b) in enumerate(spans):
            if any(pos in flow['occupied'] for pos in range(a, b)):
                return None, 'table overlaps reachable instructions'
            if any(a < d and c < b for c, d in spans[:i]):
                return None, 'index and target tables overlap'
        targets = sorted({entry['target'] for entry in entries})
        if any(any(a <= target < b for a, b in spans) for target in targets):
            return None, 'table target enters table bytes'
        if any(target in flow['occupied'] and target not in insns for target in targets):
            return None, 'table target is inside an instruction'
        by_index = {entry['index']: entry['target'] for entry in entries}
        return dict(site=jump.address, compare_site=compare.address, guard_site=guard.address,
                    index_register=input_register, index_domain=[0, count-1], default_target=default,
                    table=table, lookup=lookup, entries=entries, targets=targets,
                    case_targets=[by_index[value] for value in lookup_values],
                    instruction_path=[dict(site=ins.address, bytes=bytes(ins.bytes).hex(),
                                           instruction=ins.mnemonic+' '+ins.op_str) for ins in reversed(trail)]), ''

    accepted, blocked = {}, {}
    while True:
        flow = walk(accepted)
        proposed, reasons = {}, {}
        for site, ins in flow['instructions'].items():
            if not branch(ins) or len(ins.operands) == 1 and ins.operands[0].type == x86.X86_OP_IMM:
                continue
            if site in blocked:
                reasons[site] = blocked[site]
                continue
            proof, reason = prove(flow, ins)
            if proof:
                proposed[site] = proof
            else:
                reasons[site] = reason
                if site in accepted:
                    blocked[site] = reason
        # A later table can introduce another predecessor or an instruction
        # over a former table. Recheck every prefix after expanding the graph;
        # rejected proofs are never reused through an oscillating fixed point.
        if proposed == accepted:
            flow['switches'] = list(accepted.values())
            flow['issues'].extend(dict(site=site, kind='indirect-control-flow', reason=reason)
                                  for site, reason in sorted(reasons.items()))
            return flow
        accepted = proposed
