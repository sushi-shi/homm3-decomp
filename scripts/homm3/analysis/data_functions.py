"""Raw function bodies and independent COFF address resolution for data analyses."""
from collections import defaultdict
import struct

import capstone as cs

from homm3.analysis import data_effects


class Functions:
    def __init__(self, layout, sizes, runtime, objects=None, identities=None, code_claims=()):
        self.layout, self.sizes, self.runtime = layout, sizes, runtime
        self.objects, self.identities = objects, identities
        self.bodies, self.locations, self.external = {}, {}, defaultdict(list)
        self.anchors = defaultdict(set)
        self.effects, self.pop_counts = {}, {}
        self.runtime_names = defaultdict(set)
        for name, rvas in runtime.items():
            for rva in rvas:
                self.runtime_names[rva].add(name)
        if objects is None:
            return
        claims = defaultdict(set)
        for claim in code_claims:
            claims[claim['symbol']].add(claim['rva'])
        for unit, obj in objects.items():
            sections = defaultdict(list)
            for symbol in obj.symbols.values():
                if symbol.section > 0 and symbol.typ & 0x20 and obj.sections[symbol.section-1].characteristics & 0x20:
                    sections[symbol.section].append(symbol)
            for ordinal, symbols in sections.items():
                end = obj.sections[ordinal-1].raw_size
                positions = sorted({s.value for s in symbols} | {end})
                for symbol in symbols:
                    if not 0 <= symbol.value < end:
                        continue
                    stop = positions[positions.index(symbol.value)+1]
                    key = (unit, symbol.index)
                    self.bodies[key] = (ordinal, symbol.value, stop)
                    self.locations[unit, ordinal, symbol.value] = key
                    if symbol.storage_class == 2:
                        self.external[symbol.name].append(key)
                    self.anchors[key].update(claims[symbol.name])

    def body(self, key):
        if self.objects is None:
            return self.layout.read(key, self.sizes[key]), self.layout.base+key
        ordinal, start, end = self.bodies[key]
        obj = self.objects[key[0]]
        return obj.section_bytes(obj.sections[ordinal-1])[start:end], start

    def name(self, key):
        if self.objects is None:
            return '|'.join(sorted(self.runtime_names[key])) or f'retail:{key:#x}'
        return self.objects[key[0]].symbols[key[1]].name

    def key(self, target):
        if not target:
            return None
        if target[0] == 'code':
            return target[1] if target[1] in self.bodies else None
        if self.objects is None and target[0] == 'integer':
            rva = target[1]-self.layout.base
            return rva if rva in self.sizes else None
        if self.objects is not None and target[0] == 'external':
            keys = self.external[target[1]]
            return keys[0] if len(keys) == 1 else None
        return None

    def target_names(self, target):
        if not target:
            return set()
        if target[0] == 'external':
            return {target[1]}
        key = self.key(target)
        if key is not None:
            return {self.name(key)} if self.objects is not None else self.runtime_names[key]
        if target[0] == 'integer':
            return self.runtime_names[target[1]-self.layout.base]
        return set()

    def pop_count(self, target):
        if '_atexit' in self.target_names(target):
            return 0
        key = self.key(target)
        if key is None:
            return None
        if key not in self.pop_counts:
            raw, start = self.body(key)
            disassembler = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
            disassembler.detail = True
            counts = {i.operands[0].imm if i.operands else 0 for i in disassembler.disasm(raw, start)
                      if i.mnemonic == 'ret'}
            self.pop_counts[key] = next(iter(counts)) if len(counts) == 1 else None
        return self.pop_counts[key]

    def analyze(self, key):
        if key in self.effects:
            return self.effects[key]
        raw, start = self.body(key)
        resolver = None
        if self.objects is not None:
            unit = key[0]
            obj = self.objects[unit]
            ordinal = self.bodies[key][0]
            relocs = defaultdict(list)
            for ref in obj.relocations:
                if ref.section == ordinal:
                    relocs[ref.site].append(ref)
            def resolver(ins, field):
                offset = ins.imm_offset if field == 'imm' else ins.disp_offset
                width = ins.imm_size if field == 'imm' else ins.disp_size
                if not width:
                    return False, None
                site = ins.address+offset
                refs = [r for pos, rs in relocs.items() if pos < site+width and site < pos+4 for r in rs]
                if not refs:
                    return False, None
                if len(refs) != 1 or refs[0].site != site or width != 4 or refs[0].typ not in (6, 20):
                    return True, None
                ref = refs[0]
                symbol = obj.symbols[ref.symbol_index]
                addend = struct.unpack_from('<i', obj.section_bytes(obj.sections[ordinal-1]), site)[0]
                if symbol.section > 0 and obj.sections[symbol.section-1].characteristics & 0x20:
                    function = self.locations.get((unit, symbol.section, symbol.value+addend))
                    return True, ('code', function) if function is not None else None
                if ref.typ == 20:
                    return True, ('external', symbol.name) if symbol.section == 0 and not addend else None
                anchors = self.identities.resolve(unit, symbol, addend)
                targets = {a['target_rva'] for a in anchors}
                if len(targets) == 1:
                    return True, data_effects.integer(self.layout.base+next(iter(targets)))
                if symbol.section == 0 and symbol.typ & 0x20 and not addend:
                    return True, ('external', symbol.name)
                return True, None
        result = data_effects.analyze(raw, start, relocation=resolver, call_pop=self.pop_count)
        self.effects[key] = result
        return result

    def closure(self, key, *, limit=256):
        """Collect reachable direct-call effects; cutoffs and unknown calls stay gaps.

        No argument substitution is claimed here. Register-indirect writes in a
        callee remain unknown even if the caller supplies a known receiver.
        """
        pending, seen, writes, reads, calls, issues = [key], set(), [], [], [], []
        while pending:
            current = pending.pop()
            if current in seen:
                continue
            if len(seen) >= limit:
                issues.append(dict(kind='call-closure-limit', function=str(current)))
                break
            seen.add(current)
            analysis = self.analyze(current)
            issues.extend(dict(i, function=str(current)) for i in analysis['issues'])
            for event in analysis['events']:
                if event['kind'] in ('write', 'read'):
                    address = event['address']
                    rva = address[1]-self.layout.base if address and address[0] == 'integer' else None
                    section = next((s for s in self.layout.sections if rva is not None and
                                    s.name in ('.data', '.rdata', '.bss') and
                                    s.rva <= rva < rva+event['width'] <= s.rva+s.mapped_size), None)
                    if section:
                        (writes if event['kind'] == 'write' else reads).append(dict(event, rva=rva, function=str(current)))
                    else:
                        issues.append(dict(kind='unresolved-'+event['kind'], function=str(current), site=event['site']))
                else:
                    names = self.target_names(event['target'])
                    calls.append(dict(event, names=sorted(names), function=str(current)))
                    callee = self.key(event['target'])
                    if '_atexit' in names:
                        continue  # Registration records the callback; it does not execute it.
                    runtime = bool(self.runtime_names.get(callee)) if self.objects is None else bool(
                        names.intersection(self.runtime))
                    if callee is None or runtime:
                        issues.append(dict(kind='unmodeled-call', function=str(current), site=event['site'], names=sorted(names)))
                    elif callee not in seen:
                        pending.append(callee)
        registration_gaps = [i for i in issues if i['kind'] not in ('unresolved-read', 'unresolved-write')]
        return dict(writes=writes, reads=reads, calls=calls, issues=issues, functions=len(seen),
                    registration_complete=not registration_gaps and all(self.analyze(k)['linear_return'] for k in seen),
                    complete=self.analyze(key)['complete'] and not issues)
