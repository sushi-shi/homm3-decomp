"""Consumer evidence from raw retail/candidate x86 address calculations.

This pass compares observed address expressions, not function behavior. Joins,
memory mutation, unsupported instructions and unresolved operands lose proof.
The polynomial coefficients always denote machine byte addressing, independently
of source type names. Table bounds and semantic units require additional evidence.
"""
from collections import Counter, defaultdict, deque
from dataclasses import dataclass, field
from bisect import bisect_right
import hashlib
import json
from pathlib import Path

import capstone as cs
from capstone import x86_const as x86

from homm3.analysis.access_expressions import Expression as E, ZERO, affine_range
from homm3.analysis.data_effects import ALIASES
from homm3.analysis import access_bounds


@dataclass
class State:
    registers: dict
    stack: dict
    memory_clean: bool = True
    bounds: dict = field(default_factory=dict)
    predicate: tuple | None = None

    def copy(self):
        return State(dict(self.registers), dict(self.stack), self.memory_clean, dict(self.bounds), self.predicate)

    def reg(self, name):
        full, width, shift = ALIASES.get(name, (name, 32, 0))
        value = self.registers.get(full)
        if value is None or width == 32:
            return value
        number = value.number()
        return E.constant((number >> shift) & ((1 << width)-1)) if number is not None else None

    def set_reg(self, name, value):
        full, width, shift = ALIASES.get(name, (name, 32, 0))
        if width != 32:
            old = self.registers.get(full)
            a, b = old.number() if old is not None else None, value.number() if value is not None else None
            mask = ((1 << width)-1) << shift
            value = E.constant((a & ~mask) | ((b << shift) & mask)) if a is not None and b is not None else None
        if value is None:
            self.registers.pop(full, None)
        else:
            self.registers[full] = value

    def join(self, other):
        return State({k: v for k, v in self.registers.items() if other.registers.get(k) == v},
                     {k: self.stack.get(k) if self.stack.get(k) == other.stack.get(k) else None
                      for k in self.stack.keys() | other.stack.keys()},
                     self.memory_clean and other.memory_clean,
                     {k: (min(v[0], other.bounds[k][0]), max(v[1], other.bounds[k][1]))
                      for k, v in self.bounds.items() if k in other.bounds},
                     self.predicate if self.predicate == other.predicate else None)

    def store_stack(self, start, width, value):
        for key in list(self.stack):
            s, w = key
            if s < start+width and start < s+w:
                self.stack[key] = None
        self.stack[start, width] = value


def analyze(raw, start=0, *, relocation=None, normalize_address=lambda e: e, call_pop=None, call_effect=None):
    disassembler = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    disassembler.detail = True
    instructions = {i.address: i for i in disassembler.disasm(raw, start)}
    from homm3.analysis import access_loops
    loop_candidates = access_loops.candidates(instructions, start)
    loop_domains = {}
    states = {start: State(dict(esp=E.atom('stack'), **{
        reg: E.atom('entry-register', reg) for reg in ('ecx', 'edx', 'ebx', 'esi', 'edi', 'ebp')}), {})}
    pending = deque([start])
    outcomes, edges = {}, {}

    def transfer(ins, before):
        state = before.copy()
        accesses, calls, issues, returns = [], [], [], []
        ops, mnemonic = ins.operands, ins.mnemonic
        if x86.X86_REG_EFLAGS in ins.regs_access()[1]:
            state.predicate = None

        def issue(kind, **details):
            issues.append(dict(site=ins.address, kind=kind, **details))

        def resolved(field, fallback):
            present, val = relocation(ins, field) if relocation else (False, None)
            if not present:
                return E.constant(fallback)
            if val and val[0] == 'integer':
                return normalize_address(E.constant(val[1]))
            # Code operands have identity, but are not data addresses. Preserve
            # the raw token on calls below; never reinterpret placeholders.
            issue('unresolved-'+field)
            return None

        def address(op):
            mem = op.mem
            if mem.segment:
                return None
            result = resolved('disp', mem.disp)
            for reg, scale in ((mem.base, 1), (mem.index, mem.scale)):
                if reg:
                    part = state.reg(ins.reg_name(reg))
                    part = part.times(E.constant(scale)) if part is not None else None
                    result = result.plus(part) if result is not None else None
            return normalize_address(result) if result is not None else None

        def value(op):
            if op.type == x86.X86_OP_REG:
                return state.reg(ins.reg_name(op.reg))
            if op.type == x86.X86_OP_IMM:
                return resolved('imm', op.imm)
            if op.type != x86.X86_OP_MEM:
                return None
            addr = address(op)
            if addr is None:
                return None
            stack = addr.stack_offset()
            if stack is not None:
                if (stack, op.size) in state.stack:
                    return state.stack[stack, op.size]
                # An earlier partial overwrite must not resurrect the original
                # incoming argument through a differently sized load.
                overlap = any(s < stack+op.size and stack < s+w for s, w in state.stack)
                if not overlap and state.memory_clean and stack >= 4:
                    return E.atom('argument', stack-4, op.size)
                return None
            if not state.memory_clean:
                issue('load-after-memory-mutation')
                return None
            return E.atom('load', op.size, addr.terms)

        def store(op, val):
            if op.type == x86.X86_OP_REG:
                state.set_reg(ins.reg_name(op.reg), val)
                return
            if op.type != x86.X86_OP_MEM:
                return
            addr = address(op)
            stack = addr.stack_offset() if addr is not None else None
            if stack is not None:
                # Unknown entries are retained as tombstones for overwritten
                # arguments. Overlapping cells also lose their former value.
                state.store_stack(stack, op.size, val)
            else:
                for row in accesses:
                    if row['access'] == 'write' and row['address'] == addr:
                        row['value'] = val
                state.memory_clean = False
                state.stack = {k: None for k in state.stack}

        # Capture the pre-instruction address of every explicit memory operand.
        # LEA calculates an address but does not read storage.
        if mnemonic != 'lea':
            for index, op in enumerate(ops):
                if op.type != x86.X86_OP_MEM:
                    continue
                addr = address(op)
                if addr is not None and addr.stack_offset() is not None:
                    continue
                for access, flag in (('read', cs.CS_AC_READ), ('write', cs.CS_AC_WRITE)):
                    if op.access & flag:
                        accesses.append(dict(site=ins.address, operand=index, access=access, width=op.size,
                                             address=addr, instruction=mnemonic,
                                             bounds=access_bounds.records(access_bounds.for_expression(addr, state.bounds)),
                                             execution='repeat-count-unproved' if ins.prefix[0] in (0xf2, 0xf3)
                                             else 'instruction-site'))

        if mnemonic == 'cmp' and len(ops) == 2 and ops[0].size == 4:
            state.predicate = ('cmp', value(ops[0]), value(ops[1]))
        elif mnemonic == 'test' and len(ops) == 2 and ops[0].size == 4 and ops[0].type == ops[1].type == x86.X86_OP_REG and ops[0].reg == ops[1].reg:
            state.predicate = ('test', value(ops[0]), ZERO)
        elif mnemonic == 'mov' and len(ops) == 2:
            store(ops[0], value(ops[1]))
        elif mnemonic in ('movzx', 'movsx') and len(ops) == 2:
            val = value(ops[1])
            number = val.number() if val is not None else None
            if number is not None:
                bits = ops[1].size*8
                number &= (1 << bits)-1
                if mnemonic == 'movsx' and number & (1 << (bits-1)):
                    number -= 1 << bits
                val = E.constant(number)
            elif val is not None:
                val = E.atom(mnemonic, ops[1].size, val.terms)
            store(ops[0], val)
        elif mnemonic == 'lea' and len(ops) == 2:
            store(ops[0], address(ops[1]))
        elif mnemonic in ('add', 'sub', 'imul', 'shl', 'sal') and len(ops) in (2, 3):
            left = value(ops[1] if len(ops) == 3 else ops[0])
            right = value(ops[-1])
            val = None
            if left is not None and right is not None:
                if mnemonic == 'sub':
                    right = right.times(E.constant(-1))
                if mnemonic in ('shl', 'sal'):
                    count = right.number()
                    right = E.constant(1 << (count & 31)) if count is not None else None
                val = left.plus(right) if mnemonic in ('add', 'sub') else left.times(right)
            store(ops[0], val)
        elif mnemonic in ('inc', 'dec') and len(ops) == 1:
            val = value(ops[0])
            store(ops[0], val.offset(1 if mnemonic == 'inc' else -1) if val is not None else None)
        elif mnemonic == 'xor' and len(ops) == 2 and ops[0].type == ops[1].type == x86.X86_OP_REG and ops[0].reg == ops[1].reg:
            store(ops[0], ZERO)
        elif mnemonic == 'and' and len(ops) == 2:
            left, right = value(ops[0]), value(ops[1])
            mask = right.number() if right is not None else None
            if mask is not None and left is not None and left.number() is not None:
                val = E.constant(left.number() & mask)
            elif mask == 0:
                val = ZERO
            elif mask == -1:
                val = left
            elif mask is not None and 0 <= mask <= 0x7fffffff and left is not None:
                val = E.atom('mask', mask, left.terms)
            else:
                val = None
            store(ops[0], val)
        elif mnemonic == 'push' and len(ops) == 1 and ops[0].size == 4:
            val, sp = value(ops[0]), state.reg('esp')
            sp = sp.offset(-4) if sp is not None else None
            state.set_reg('esp', sp)
            offset = sp.stack_offset() if sp is not None else None
            if offset is not None:
                state.store_stack(offset, 4, val)
        elif mnemonic == 'pop' and len(ops) == 1 and ops[0].size == 4:
            sp = state.reg('esp')
            offset = sp.stack_offset() if sp is not None else None
            val = state.stack.get((offset, 4)) if offset is not None else None
            state.set_reg('esp', sp.offset(4) if sp is not None else None)
            store(ops[0], val)
        elif mnemonic == 'leave':
            bp = state.reg('ebp')
            offset = bp.stack_offset() if bp is not None else None
            state.set_reg('ebp', state.stack.get((offset, 4)) if offset is not None else None)
            state.set_reg('esp', bp.offset(4) if bp is not None else None)
        elif mnemonic == 'call':
            state.predicate = None
            present, target = relocation(ins, 'imm') if relocation else (False, None)
            if not present:
                target = ('integer', ops[0].imm & 0xffffffff) if ops and ops[0].type == x86.X86_OP_IMM else None
            sp = state.reg('esp')
            offset = sp.stack_offset() if sp is not None else None
            arguments = [state.stack.get((offset+4*i, 4)) if offset is not None else None for i in range(16)]
            registers = dict(state.registers)
            effect = call_effect(target, arguments, registers, state.memory_clean, state.bounds) if call_effect else None
            calls.append(dict(site=ins.address, target=target, arguments=arguments,
                              registers=registers, bounds=access_bounds.records(state.bounds),
                              memory_clean=state.memory_clean,
                              status=effect['status'] if effect else 'effects-unproved'))
            pop = call_pop(target) if call_pop else None
            for reg in ('eax', 'ecx', 'edx'):
                state.set_reg(reg, None)
            state.set_reg('esp', sp.offset(pop) if sp is not None and pop is not None else None)
            if effect is not None:
                state.set_reg('eax', effect['value'])
                for row in effect['accesses']:
                    accesses.append(dict(row, call_path=[ins.address, *row.get('call_path', [])]))
                # A proved read-only callee preserves live caller stack cells;
                # its private frame can overwrite abandoned cells below ESP.
                state.stack = {k: v if offset is not None and k[0] >= offset else None for k, v in state.stack.items()}
            else:
                state.memory_clean = False
                state.stack = {k: None for k in state.stack}
                issue('call-effects-unproved')
        elif mnemonic == 'ret':
            returns.append(dict(site=ins.address, registers=dict(state.registers), stack=state.reg('esp'),
                                caller_stack_written=any(offset >= 0 for offset, _ in state.stack)))
        elif not (mnemonic in ('cmp', 'test', 'nop', 'ret') or ins.group(cs.CS_GRP_JUMP)):
            issue('unsupported-instruction', instruction=mnemonic)
            state.memory_clean = False
            for op in ops:
                if op.type == x86.X86_OP_MEM and op.access & cs.CS_AC_WRITE:
                    store(op, None)
            for reg in ins.regs_access()[1]:
                state.set_reg(ins.reg_name(reg), None)
        return state, accesses, calls, issues, returns

    while pending:
        site = pending.popleft()
        ins = instructions.get(site)
        if ins is None:
            outcomes[site] = ([], [], [dict(site=site, kind='undecoded-control-flow')], [])
            edges[site] = []
            continue
        if site in loop_candidates:
            spec = loop_candidates[site]
            if site not in loop_domains:
                loop_domains[site] = access_loops.domain(spec, states[site].reg(spec['register']),
                                                        normalize_address, relocation)
            domain = loop_domains[site]
            if domain is not None:
                states[site].set_reg(spec['register'], domain['value'])
                states[site].bounds[domain['atom']] = domain['bounds']
        after, accesses, calls, issues, returns = transfer(ins, states[site])
        successors = []
        if ins.group(cs.CS_GRP_RET):
            pass
        elif access_loops.is_branch(ins):
            if ins.operands and ins.operands[0].type == x86.X86_OP_IMM:
                target = ins.operands[0].imm
                if start <= target < start+len(raw):
                    successors.append(target)
                else:
                    issues.append(dict(site=site, kind='external-tail-edge'))
            else:
                issues.append(dict(site=site, kind='indirect-control-flow'))
            if ins.mnemonic != 'jmp':
                successors.append(site+ins.size)
        else:
            successors.append(site+ins.size)
        outcomes[site] = accesses, calls, issues, returns
        edges[site] = successors
        for index, successor in enumerate(successors):
            outgoing = after.copy()
            if access_loops.is_branch(ins) and ins.mnemonic != 'jmp':
                taken = bool(ins.operands and ins.operands[0].type == x86.X86_OP_IMM and
                             successor == ins.operands[0].imm and index == 0)
                refined = access_bounds.refine(after.bounds, after.predicate, ins.mnemonic, taken)
                if refined is None:
                    continue
                outgoing.bounds = refined
            merged = states[successor].join(outgoing) if successor in states else outgoing
            if successor not in states or merged != states[successor]:
                states[successor] = merged
                pending.append(successor)
    accesses = [a for site in sorted(outcomes) for a in outcomes[site][0]]
    calls = [a for site in sorted(outcomes) for a in outcomes[site][1]]
    issues = [a for site in sorted(outcomes) for a in outcomes[site][2]]
    for row in accesses:
        row['status'] = 'expression-known' if row['address'] is not None else 'address-unproved'
    return dict(accesses=accesses, calls=calls, issues=issues,
                returns=[r for site in sorted(outcomes) for r in outcomes[site][3]],
                branch_sites=[s for s, targets in edges.items() if len(targets) > 1],
                back_edges=[(s, t) for s, targets in edges.items() for t in targets if t <= s],
                loops={s: d for s, d in loop_domains.items() if d is not None})


def compare(retail, candidate):
    """Address-expression multisets only; never claim full behavior equivalence."""
    def expressions(report):
        return Counter((r['access'], r['width'], r['address']) for r in report['accesses'] if r['address'] is not None)
    left, right = expressions(retail), expressions(candidate)
    from homm3.analysis.data_consumers import scale_changes, range_changes
    ranges = range_changes(retail, candidate)
    debt = any(p['issues'] or p['branch_sites'] or p['back_edges'] or
               any(r['address'] is None for r in p['accesses']) for p in (retail, candidate))
    return dict(status='no-data-access-observed' if not left and not right and not debt else
                'observed-expressions-agree' if left == right and not debt and not ranges else
                'observed-expressions-differ' if left != right else
                'observed-domains-differ' if ranges else 'expressions-unproved',
                missing=list((left-right).elements()), extra=list((right-left).elements()),
                complete_observation=not debt,
                scale_changes=scale_changes(retail, candidate),
                range_changes=ranges,
                policy='byte-address expressions only; execution order, bounds and behavior not proved')


class Storage:
    """Independent storage anchors; source bounds remain separate from COFF spans."""
    def __init__(self, image_base, projections):
        self.base = image_base
        self.extents = defaultdict(set)
        self.source_extents = defaultdict(set)
        self.emitted = defaultdict(set)
        self.conflicts = []
        for p in projections:
            if p['status'] == 'binding-conflict':
                self.conflicts.append((p['rva'], p['rva']+p['size']))
            else:
                self.extents[p['rva']].add(p['size'])
                if p.get('extent_kind', 'coff-contribution' if p.get('macro') == 'VENDOR' else 'source') == 'source':
                    self.source_extents[p['rva']].add(p['size'])
                if p.get('physical_size') is not None:
                    self.emitted[p['rva']].add((p.get('unit', ''), p['physical_size']))
        self.starts = sorted(self.extents)

    def normalize(self, expression):
        if expression is None:
            return None
        if any(f and any(a[0] in ('storage', 'stack') for a in f) for f, _ in expression.terms):
            return expression
        constant = sum(c for f, c in expression.terms if not f) & 0xffffffff
        rva = constant-self.base
        if any(lo <= rva < hi for lo, hi in self.conflicts):
            return None
        index = bisect_right(self.starts, rva)-1
        if index < 0:
            return expression
        owner = self.starts[index]
        sizes = self.extents[owner]
        if len(sizes) != 1 or not owner <= rva < owner+next(iter(sizes)):
            return expression
        return E.make([(f, c) for f, c in expression.terms if f]+[
            ((('storage', owner),), 1), ((), rva-owner)])

    def extent(self, expression, width, bounds=None):
        if expression is None:
            return dict(owner_rva=None, status='address-unproved', byte_range=None)
        owners = [(f[0][1], c) for f, c in expression.terms if len(f) == 1 and f[0][0] == 'storage']
        if len(owners) != 1 or owners[0][1] != 1:
            return dict(owner_rva=None, status='referent-unbound', byte_range=None)
        owner = owners[0][0]
        offset = E.make([(f, c) for f, c in expression.terms if f != (('storage', owner),)])
        span = affine_range(offset, bounds or {})
        sizes = self.source_extents.get(owner, ())
        size = next(iter(sizes)) if len(sizes) == 1 else None
        status = ('range-unproved' if span is None else 'extent-unproved' if size is None else
                  'outside-source-extent' if span[0] < 0 or span[1]+width > size else 'within-source-extent')
        return dict(owner_rva=owner, status=status, source_size=size,
                    byte_range=[span[0], span[1]+width] if span is not None else None,
                    emitted_spans=[dict(unit=u, size=n) for u, n in sorted(self.emitted[owner])],
                    within_all_emitted_spans=(all(0 <= span[0] and span[1]+width <= n for _, n in self.emitted[owner])
                                              if span is not None and self.emitted[owner] else None),
                    evidence='source projection and candidate spans; possible footprint, not a proved overrun or retail boundary')


def generate(root, *, evidence=None, static_report=None, jobs=4):
    from homm3.analysis.data_functions import Functions
    from homm3.core import tsv
    from homm3.sema import data_match
    paths = ['scripts/homm3/analysis/'+n+'.py' for n in ('data_accesses', 'access_expressions', 'access_loops', 'access_bounds', 'access_calls', 'data_consumers', 'data_functions')]
    implementation_hashes = {p: hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths}
    evidence = evidence or data_match.prepare(root, jobs=jobs)
    static_report = static_report or data_match.generate(root, evidence=evidence)
    layout, objects, candidates = (evidence[k] for k in ('layout', 'objects', 'candidate_report'))
    sizes = {int(r['rva'], 0): int(r['size'], 0) for r in tsv.read(root/'config/retail/functions.tsv')[2]}
    runtime = defaultdict(set)
    for row in tsv.read(root/'config/retail/runtime-map.tsv')[2]:
        runtime[row['name']].add(int(row['rva'], 0))
    projections = static_report['enrollment']
    rejected = {i for p in projections if p['status'] == 'binding-conflict' for i in p['binding_ids']}
    identities = data_match.Identities(candidates['candidate_data'],
        [b for b in candidates['data_bindings'] if b['id'] not in rejected], objects, evidence['code_claims'])
    retail = Functions(layout, sizes, runtime)
    candidate = Functions(layout, sizes, runtime, objects, identities, evidence['code_claims'])
    storage = Storage(layout.base, projections)
    from homm3.analysis.access_calls import Summaries
    summaries = {id(p): Summaries(p, storage.normalize) for p in (retail, candidate)}
    accesses, comparisons, issues, analyzed, calls = [], [], [], {}, []

    def profile(provider, key, side):
        cache_key = (side, key)
        if cache_key in analyzed:
            return analyzed[cache_key]
        result = summaries[id(provider)].analyze(key)
        index = len(analyzed)
        analyzed[cache_key] = index, result
        for row in result['accesses']:
            bounds = {r['atom']: r['bounds'] for r in row['bounds']}
            extent = storage.extent(row['address'], row['width'], bounds)
            if row['execution'] != 'instruction-site':
                extent.update(status='repeat-count-unproved', byte_range=None)
            accesses.append(dict(row, function_id=index, side=side, function=list(key) if isinstance(key, tuple) else key,
                                 extent=extent,
                                 loop_bounds=list(result['loops'].values())))
        issues.extend(dict(row, function_id=index, side=side) for row in result['issues'])
        for call in result['calls']:
            callee = provider.key(call['target'])
            anchors = ({callee} if provider.objects is None and callee is not None else provider.anchors.get(callee, set()))
            calls.append(dict(call, function_id=index, side=side,
                              callee=callee, callee_anchors=sorted(anchors)))
        if result['branch_sites'] or result['back_edges']:
            issues.append(dict(function_id=index, side=side, kind='control-flow-range-unproved',
                               branch_sites=result['branch_sites'], back_edges=result['back_edges']))
        return index, result

    for key in sorted(candidate.bodies):
        anchors = candidate.anchors[key]
        ci, c = profile(candidate, key, 'candidate')
        if len(anchors) != 1 or next(iter(anchors)) not in sizes:
            issues.append(dict(kind='consumer-entry-unpaired', unit=key[0], symbol=candidate.name(key),
                               function_id=ci, anchors=sorted(anchors)))
            continue
        rva = next(iter(anchors))
        ri, r = profile(retail, rva, 'retail')
        comparisons.append(dict(unit=key[0], symbol=candidate.name(key), rva=rva,
                                candidate_function_id=ci, retail_function_id=ri, **compare(r, c)))
    # The retail denominator includes every admitted function, even when no
    # authored candidate body was paired. Missing evidence is never empty success.
    for rva in sorted(sizes):
        if ('retail', rva) not in analyzed:
            index, _ = profile(retail, rva, 'retail')
            issues.append(dict(kind='retail-consumer-unpaired', rva=rva, function_id=index))
    from homm3.analysis.data_consumers import relationships
    storage_roles, storage_differences = relationships(accesses, comparisons, evidence['declared'], projections)
    issues.extend(storage_differences)
    for row in evidence['declared']['declarations']:
        if row.get('shape_conflict'):
            issues.append(dict(kind='conflicting-consumer-dimensions', rva=row['rva'], declarations=row['shapes'],
                               source=row['source'], detail='compiler-derived dimensions or element widths disagree'))
    input_hashes = dict(static_report['input_sha256'], **implementation_hashes)
    for path, expected in input_hashes.items():
        if hashlib.sha256((root/path).read_bytes()).hexdigest() != expected:
            raise ValueError(f'{path}: consumer inputs changed during analysis')
    return dict(schema='homm3.data-accesses.v1', retail_sha256=static_report['retail_sha256'],
        accesses=accesses, comparisons=comparisons, calls=calls, storage=storage_roles, issues=issues,
        analysis_issues=static_report['analysis_issues'],
        input_sha256=input_hashes,
        summary=dict(functions=len(analyzed), retail_functions=len(sizes), pairs=len(comparisons),
                     accesses=len(accesses), access_statuses=dict(Counter(r['status'] for r in accesses)),
                     extent_statuses=dict(Counter(r['extent']['status'] for r in accesses)),
                     comparison_statuses=dict(Counter(r['status'] for r in comparisons)),
                     call_statuses=dict(Counter(r['status'] for r in calls)),
                     scale_changes=sum(len(r['scale_changes']) for r in comparisons),
                     range_changes=sum(len(r['range_changes']) for r in comparisons),
                     storage_relationship_differences=len(storage_differences),
                     issue_counts=dict(Counter(r['kind'] for r in issues))),
        policy=dict(expressions='machine byte addresses; independent entry/storage anchors',
                    bounds='source extents only; unbounded index domains remain unproved',
                    units='scaling compared without inferring semantic units from names',
                    completion='expression agreement is not whole-function or memory-safety proof'))


def serial(value):
    if isinstance(value, E):
        return serial(value.terms)
    if isinstance(value, dict):
        return {k: serial(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [serial(v) for v in value]
    return value


def export(report, directory):
    from homm3.core import tsv
    directory = Path(directory)
    for name, key, fields in (
        ('data-accesses', 'accesses', ['function_id', 'side', 'function', 'site', 'operand', 'access', 'width', 'address', 'instruction', 'status', 'execution', 'extent', 'bounds', 'loop_bounds', 'call_path', 'value']),
        ('data-access-matches', 'comparisons', ['unit', 'symbol', 'rva', 'candidate_function_id', 'retail_function_id', 'status', 'missing', 'extra', 'scale_changes', 'range_changes', 'complete_observation', 'policy']),
        ('data-consumer-calls', 'calls', ['function_id', 'side', 'site', 'target', 'callee', 'callee_anchors', 'arguments', 'registers', 'bounds', 'memory_clean', 'status']),
        ('data-consumer-storage', 'storage', ['rva', 'declarations', 'retail_read_functions', 'retail_write_functions', 'candidate_read_functions', 'candidate_write_functions']),
        ('data-contract-issues', 'issues', ['kind', 'side', 'function_id', 'details'])):
        rows = []
        for source in report[key]:
            row = serial(source)
            if key == 'issues':
                row = dict(row, details={k: v for k, v in row.items() if k not in fields})
            rows.append({k: json.dumps(row[k], sort_keys=True, separators=(',', ':')) if isinstance(row.get(k), (dict, list, bool))
                         else '' if row.get(k) is None else row[k] for k in fields})
        tsv.write(directory/(name+'.tsv'), ['# Raw consumer expressions; unresolved evidence never matches.'], fields, rows)
    summary = {k: v for k, v in report.items() if k not in ('accesses', 'comparisons', 'calls', 'storage', 'issues')}
    (directory/'data-access-summary.json').write_text(json.dumps(summary, indent=2)+'\n')


def exact(report):
    return bool(report['comparisons']) and not report['analysis_issues'] and not report['issues'] and all(
        r['status'] in ('observed-expressions-agree', 'no-data-access-observed') for r in report['comparisons']) and all(
        r['extent']['status'] == 'within-source-extent' for r in report['accesses'])


def run(args):
    from homm3.core.common import HOMM3_DIR
    report = generate(HOMM3_DIR, jobs=args.jobs)
    export(report, args.output)
    print(json.dumps(report['summary'], indent=2))
    return int(bool(report['analysis_issues']) or args.require_exact and not exact(report))


def main():
    import argparse
    from homm3.core.common import HOMM3_DIR
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', default='build/data-accesses')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--require-exact', action='store_true')
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    return run(args)


if __name__ == '__main__':
    raise SystemExit(main())
