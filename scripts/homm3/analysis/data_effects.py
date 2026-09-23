"""Conservative x86 constant/stack propagation for data relationship evidence.

This is not execution or a general equivalence prover. Joins retain only equal
facts; unknown calls invalidate volatile registers and stack contents. Unsupported
effects remain explicit. Relocation resolution is supplied by the caller, so an
unresolved COFF word can never accidentally act as literal zero.
"""
from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import capstone as cs
from capstone import x86_const as x86


def integer(value):
    return ('integer', value & 0xffffffff)


def arithmetic(left, right, operation):
    if left is None or right is None:
        return None
    if left[0] == right[0] == 'integer':
        a, b = left[1], right[1]
        return integer({'add': lambda: a+b, 'sub': lambda: a-b,
                        'and': lambda: a & b, 'or': lambda: a | b,
                        'xor': lambda: a ^ b, 'shl': lambda: a << (b & 31),
                        'shr': lambda: a >> (b & 31)}[operation]())
    if left[0] == 'stack' and right[0] == 'integer' and operation in ('add', 'sub'):
        delta = right[1] if right[1] < 0x80000000 else right[1]-0x100000000
        return ('stack', left[1]+(delta if operation == 'add' else -delta))
    return None


ALIASES = {alias: (full, width, shift) for full, aliases in (
    ('eax', ('eax', 'ax', 'al', 'ah')), ('ebx', ('ebx', 'bx', 'bl', 'bh')),
    ('ecx', ('ecx', 'cx', 'cl', 'ch')), ('edx', ('edx', 'dx', 'dl', 'dh')),
    ('esi', ('esi', 'si')), ('edi', ('edi', 'di')), ('ebp', ('ebp', 'bp')),
    ('esp', ('esp', 'sp'))) for alias, width, shift in zip(aliases, (32, 16, 8, 8), (0, 0, 0, 8))}


@dataclass
class State:
    registers: dict
    stack: dict
    top: tuple = (None, None, None, None)

    def copy(self):
        return State(dict(self.registers), dict(self.stack), self.top)

    def reg(self, name):
        full, width, shift = ALIASES.get(name, (name, 32, 0))
        value = self.registers.get(full)
        if width == 32 or value is None:
            return value
        return integer(value[1] >> shift & ((1 << width)-1)) if value[0] == 'integer' else None

    def set_reg(self, name, value):
        full, width, shift = ALIASES.get(name, (name, 32, 0))
        if full == 'esp':
            self.top = (None, None, None, None)
        if width < 32:
            old = self.registers.get(full)
            if old is None or value is None or old[0] != 'integer' or value[0] != 'integer':
                value = None
            else:
                mask = ((1 << width)-1) << shift
                value = integer((old[1] & ~mask) | ((value[1] << shift) & mask))
        if value is None:
            self.registers.pop(full, None)
        else:
            self.registers[full] = value

    def store(self, address, width, value):
        if address is None:
            self.stack.clear()
            self.top = (None, None, None, None)
        elif address[0] == 'stack':
            start = address[1]
            self.stack = {key: v for key, v in self.stack.items()
                          if key[0]+key[1] <= start or start+width <= key[0]}
            if value is not None:
                self.stack[start, width] = value
            sp = self.reg('esp')
            if sp and sp[0] == 'stack':
                top = list(self.top)
                for index in range(len(top)):
                    slot = sp[1]+index*4
                    if start < slot+4 and slot < start+width:
                        top[index] = value if start == slot and width == 4 else None
                self.top = tuple(top)


def joined(left, right):
    return State({k: v for k, v in left.registers.items() if right.registers.get(k) == v},
                 {k: v for k, v in left.stack.items() if right.stack.get(k) == v},
                 tuple(a if a == b else None for a, b in zip(left.top, right.top)))


def analyze(raw, start=0, *, relocation=None, call_pop=None):
    """Return reachable writes, calls and explicit incomplete-effect reasons.

    relocation(instruction, field) returns (present, symbolic value) for `imm`
    or `disp`. A present unresolved relocation has value None. Integer values
    are linked addresses; `code` tokens preserve unmapped local callbacks.
    """
    disassembler = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    disassembler.detail = True
    instructions = {i.address: i for i in disassembler.disasm(raw, start)}
    states = {start: State({'esp': ('stack', 0)}, {})}
    pending = deque([start])
    outcomes, edges = {}, {}

    def transfer(ins, incoming):
        state = incoming.copy()
        events, issues = [], []
        ops = ins.operands
        relocated = {field: relocation(ins, field) if relocation else (False, None)
                     for field in ('imm', 'disp')}

        def address(op):
            if op.type != x86.X86_OP_MEM:
                return None
            mem = op.mem
            if mem.segment:
                return None  # FS/EH and other segment bases are not absolute data.
            present, value = relocated['disp']
            value = value if present else integer(mem.disp)
            for register, scale in ((mem.base, 1), (mem.index, mem.scale)):
                if not register:
                    continue
                part = state.reg(ins.reg_name(register))
                if part is not None and scale != 1:
                    part = integer(part[1]*scale) if part[0] == 'integer' else None
                if part is not None and part[0] == 'stack':
                    value = arithmetic(part, value, 'add')
                else:
                    value = arithmetic(value, part, 'add')
            return value

        def value(op):
            if op.type == x86.X86_OP_REG:
                return state.reg(ins.reg_name(op.reg))
            if op.type == x86.X86_OP_IMM:
                present, linked = relocated['imm']
                return linked if present else integer(op.imm)
            addr = address(op)
            return state.stack.get((addr[1], op.size)) if addr and addr[0] == 'stack' else None

        def assign(op, val):
            if op.type == x86.X86_OP_REG:
                state.set_reg(ins.reg_name(op.reg), val)
            elif op.type == x86.X86_OP_MEM:
                addr = address(op)
                if not addr or addr[0] != 'stack':
                    events.append(dict(kind='write', site=ins.address, address=addr,
                                       width=op.size, value=val))
                state.store(addr, op.size, val)

        mnemonic = ins.mnemonic
        for op in ops:
            if op.type == x86.X86_OP_MEM and op.access & cs.CS_AC_READ and mnemonic != 'lea':
                addr = address(op)
                if not addr or addr[0] != 'stack':
                    events.append(dict(kind='read', site=ins.address, address=addr, width=op.size, value=None))
        if mnemonic == 'mov' and len(ops) == 2:
            assign(ops[0], value(ops[1]))
        elif mnemonic in ('movzx', 'movsx') and len(ops) == 2:
            val = value(ops[1])
            if val and val[0] == 'integer':
                bits = ops[1].size*8
                val = val[1] & ((1 << bits)-1)
                if mnemonic == 'movsx' and val & (1 << (bits-1)):
                    val -= 1 << bits
                val = integer(val)
            else:
                val = None
            assign(ops[0], val)
        elif mnemonic == 'lea' and len(ops) == 2:
            assign(ops[0], address(ops[1]))
        elif mnemonic in ('xor', 'and', 'or', 'add', 'sub', 'shl', 'shr') and len(ops) == 2:
            lhs, rhs = value(ops[0]), value(ops[1])
            if mnemonic == 'xor' and ops[0].type == ops[1].type == x86.X86_OP_REG and ops[0].reg == ops[1].reg:
                val = integer(0)
            elif mnemonic == 'and' and rhs == integer(0):
                val = integer(0)
            elif mnemonic == 'or' and rhs == integer((1 << (ops[0].size*8))-1):
                val = rhs
            else:
                val = arithmetic(lhs, rhs, mnemonic)
            assign(ops[0], val)
        elif mnemonic in ('inc', 'dec') and len(ops) == 1:
            assign(ops[0], arithmetic(value(ops[0]), integer(1), 'add' if mnemonic == 'inc' else 'sub'))
        elif mnemonic == 'push' and len(ops) == 1:
            val = value(ops[0])
            top = state.top
            stack = arithmetic(state.reg('esp'), integer(4), 'sub')
            state.set_reg('esp', stack)
            state.store(stack, 4, val)
            state.top = (val, *top[:3]) if ops[0].size == 4 else (None, None, None, None)
            if ops[0].size != 4:
                issues.append(dict(site=ins.address, kind='unsupported-push-width'))
        elif mnemonic == 'pop' and len(ops) == 1:
            stack = state.reg('esp')
            val = state.stack.get((stack[1], 4)) if stack and stack[0] == 'stack' else None
            assign(ops[0], val)
            state.set_reg('esp', arithmetic(stack, integer(4), 'add'))
            if ops[0].size != 4:
                issues.append(dict(site=ins.address, kind='unsupported-pop-width'))
        elif mnemonic == 'call':
            target = value(ops[0]) if ops else None
            stack = state.reg('esp')
            args = [state.stack.get((stack[1]+4*i, 4), state.top[i]) if stack and stack[0] == 'stack' else state.top[i]
                    for i in range(4)]
            events.append(dict(kind='call', site=ins.address, target=target, arguments=args,
                               receiver=state.reg('ecx')))
            for reg in ('eax', 'ecx', 'edx'):
                state.set_reg(reg, None)
            state.stack.clear()
            pop = call_pop(target) if call_pop else None
            state.set_reg('esp', arithmetic(stack, integer(pop), 'add') if pop is not None else None)
        elif mnemonic == 'leave':
            stack = state.reg('ebp')
            state.set_reg('ebp', state.stack.get((stack[1], 4)) if stack and stack[0] == 'stack' else None)
            state.set_reg('esp', arithmetic(stack, integer(4), 'add'))
        elif not (mnemonic in ('cmp', 'test', 'nop', 'ret') or ins.group(cs.CS_GRP_JUMP)):
            issues.append(dict(site=ins.address, kind='unsupported-instruction', instruction=mnemonic))
            for op in ops:
                if op.type == x86.X86_OP_MEM and op.access & cs.CS_AC_WRITE:
                    assign(op, None)
            for reg in ins.regs_access()[1]:
                state.set_reg(ins.reg_name(reg), None)
        return state, events, issues

    while pending:
        address = pending.popleft()
        ins = instructions.get(address)
        if ins is None:
            outcomes[address] = ([], [dict(site=address, kind='undecoded-control-flow')])
            edges[address] = []
            continue
        after, events, issues = transfer(ins, states[address])
        successors = []
        if ins.group(cs.CS_GRP_RET):
            pass
        elif ins.group(cs.CS_GRP_JUMP):
            if ins.operands and ins.operands[0].type == x86.X86_OP_IMM:
                target = ins.operands[0].imm
                if start <= target < start+len(raw):
                    successors.append(target)
                else:
                    issues.append(dict(site=address, kind='external-tail-edge', target=target))
            else:
                issues.append(dict(site=address, kind='indirect-control-flow'))
            if ins.mnemonic != 'jmp':
                successors.append(address+ins.size)
        else:
            successors.append(address+ins.size)
        outcomes[address] = (events, issues)
        edges[address] = successors
        for successor in successors:
            merged = joined(states[successor], after) if successor in states else after.copy()
            if successor not in states or merged != states[successor]:
                states[successor] = merged
                pending.append(successor)

    events = [e for address in sorted(outcomes) for e in outcomes[address][0]]
    issues = [e for address in sorted(outcomes) for e in outcomes[address][1]]
    branches = sum(len(targets) > 1 for targets in edges.values())
    calls = sum(e['kind'] == 'call' for e in events)
    trace, cursor = [], start
    while cursor in instructions and cursor not in trace:
        trace.append(cursor)
        if len(edges.get(cursor, ())) != 1:
            break
        cursor = edges[cursor][0]
    returns = bool(trace and instructions[trace[-1]].group(cs.CS_GRP_RET))
    # Exact final byte effects are asserted only for a fully supported straight
    # line with known destinations/values and no calls. Other rows are evidence,
    # not a claim that every possible dynamic write has been recovered.
    complete = returns and not issues and not branches and not calls and all(
        e['address'] and e['address'][0] == 'integer' and e['value'] and e['value'][0] == 'integer'
        for e in events if e['kind'] == 'write')
    if complete:
        events = [e for address in trace for e in outcomes[address][0]]
    return dict(events=events, issues=issues, conditional=bool(branches),
                linear_return=returns and not branches and not issues, complete=complete)
