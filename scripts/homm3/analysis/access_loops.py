"""Proved finite domains for simple counted x86 loops, without sampling.

Only a single linear body, one induction update, an adjacent compare/latch,
and a unique linear prefix are supported. Everything else remains unbounded.
"""
import capstone as cs
from capstone import x86_const as x86

from homm3.analysis.access_expressions import Expression as E
from homm3.analysis.data_effects import ALIASES


def is_branch(instruction):
    # LOOP/LOOPE/LOOPNE are relative branches even though Capstone does not
    # put them in CS_GRP_JUMP. Calls also belong to BRANCH_RELATIVE.
    return instruction.group(cs.CS_GRP_JUMP) or (
        instruction.group(cs.CS_GRP_BRANCH_RELATIVE) and not instruction.group(cs.CS_GRP_CALL))


def candidates(instructions, start):
    ordered = sorted(instructions)
    positions = {site: i for i, site in enumerate(ordered)}
    jumps = [(i.address, i.operands[0].imm) for i in instructions.values()
             if is_branch(i) and i.operands and i.operands[0].type == x86.X86_OP_IMM]
    result = {}
    for site, header in jumps:
        latch = instructions[site]
        if header >= site or header not in positions or latch.mnemonic not in (
                'jl', 'jb', 'jle', 'jbe', 'jg', 'ja', 'jge', 'jae', 'jne'):
            continue
        body = [instructions[s] for s in ordered[positions[header]:positions[site]]]
        prefix = [instructions[s] for s in ordered if start <= s < header]
        if not body or any(is_branch(i) or i.group(cs.CS_GRP_RET) for i in prefix):
            continue
        if any(start <= target < header and source >= header for source, target in jumps):
            continue  # An enclosing loop can change the initial counter.
        if any(is_branch(i) or i.group(cs.CS_GRP_CALL) or i.group(cs.CS_GRP_RET) for i in body):
            continue
        if any(header <= target <= site and not header <= source <= site for source, target in jumps):
            continue
        compare = body[-1]
        if compare.mnemonic != 'cmp' or len(compare.operands) != 2:
            continue
        left, right = compare.operands
        if left.type != x86.X86_OP_REG or left.size != 4 or right.type != x86.X86_OP_IMM:
            continue
        register = compare.reg_name(left.reg)
        writers = [i for i in body if any(ALIASES.get(i.reg_name(r), (i.reg_name(r),))[0] == register
                                        for r in i.regs_access()[1])]
        if len(writers) != 1:
            continue
        update = writers[0]
        if not update.operands or update.operands[0].type != x86.X86_OP_REG or update.operands[0].size != 4:
            continue
        step = (1 if update.mnemonic == 'inc' else -1 if update.mnemonic == 'dec' else
                (1 if update.mnemonic == 'add' else -1)*update.operands[1].imm
                if update.mnemonic in ('add', 'sub') and update.operands[1].type == x86.X86_OP_IMM else 0)
        if not step:
            continue
        result[header] = dict(header=header, latch=site, register=register, step=step,
                              limit=right.imm, condition=latch.mnemonic, compare=compare)
    return result


def domain(spec, initial, normalize=lambda e: e, relocation=None):
    if initial is None:
        return None
    limit = E.constant(spec['limit'])
    present, target = relocation(spec['compare'], 'imm') if relocation else (False, None)
    if present:
        if not target or target[0] != 'integer':
            return None
        limit = E.constant(target[1])
    limit = normalize(limit)
    if limit is None:
        return None
    number, endpoint = initial.number(), limit.number()
    # Subtraction in Expression wraps. Loop trip counts require mathematical
    # subtraction *before* any wrapping, including ranges crossing zero.
    if number is None or endpoint is None:
        return None
    step, condition = spec['step'], spec['condition']
    unsigned = condition in ('jb', 'jbe', 'ja', 'jae')
    if unsigned:
        number, endpoint = number & 0xffffffff, endpoint & 0xffffffff
    distance = endpoint-number
    if (step > 0 and condition not in ('jl', 'jb', 'jle', 'jbe', 'jne') or
            step < 0 and condition not in ('jg', 'ja', 'jge', 'jae', 'jne')):
        return None
    direction = 1 if step > 0 else -1
    distance, stride = distance*direction, abs(step)
    if condition == 'jne':
        if distance <= 0 or distance % stride:
            return None
        iterations = distance//stride
    elif condition in ('jle', 'jbe', 'jge', 'jae'):
        iterations = max(1, distance//stride+1)
    else:
        iterations = max(1, (distance+stride-1)//stride)
    # Machine overflow would invalidate the mathematical induction. For pointer
    # expressions the normalized limit is insufficient to prove numeric limits;
    # retain those as unbounded until a linked-address no-wrap proof is supplied.
    lower, upper = (0, 0xffffffff) if unsigned else (-0x80000000, 0x7fffffff)
    end = number+step*iterations
    if not lower <= min(number, end) <= max(number, end) <= upper:
        return None
    atom = ('loop-iteration', spec['header'], spec['register'])
    value = initial.plus(E.atom(*atom).times(E.constant(step)))
    return dict(atom=atom, bounds=(0, iterations-1), value=value, iterations=iterations,
                initial=initial, step=step, limit=limit,
                proof='single linear counted body; complete induction update and no arithmetic wrap')
