"""Typed records inside code-anchored, per-function VC6 EH contributions.

A checked mov-eax/jmp-CxxFrameHandler stub establishes a FuncInfo root. Local
COFF offsets place its maps in the same indivisible compiler contribution;
retail data pointers never supply addresses or expected relocation values.
Record lengths exclude inter-record alignment and trailing contribution bytes.
"""
from collections import defaultdict
import struct

import capstone


def associative_identity(obj, section):
    """One private EH contribution selected with one named parent COMDAT."""
    def auxiliary(sec):
        headers = [s for s in obj.symbols.values() if s.section == sec.index and
                   s.name == sec.name and s.storage_class == 3 and s.aux_count == 1]
        if not sec.characteristics & 0x1000 or len(headers) != 1:
            return None
        return obj.data[headers[0].offset+18:headers[0].offset+36]

    aux = auxiliary(section)
    if aux is None or aux[14] != 5:
        return ''
    parent = int.from_bytes(aux[12:14], 'little')
    if not 1 <= parent <= len(obj.sections) or parent == section.index:
        return ''
    owner = obj.sections[parent-1]
    parent_aux = auxiliary(owner)
    public = [s for s in obj.symbols.values() if s.section == parent and s.storage_class == 2]
    if (not owner.characteristics & 0x20 or parent_aux is None or parent_aux[14] not in (2, 3, 4) or
            len(public) != 1 or public[0].value or not public[0].typ & 0x20):
        return ''
    siblings = []
    for sec in obj.sections:
        if sec.name == section.name:
            other = auxiliary(sec)
            if other is not None and other[14] == 5 and int.from_bytes(other[12:14], 'little') == parent:
                siblings.append(sec.index)
    if siblings != [section.index]:
        return ''
    return f'eh-associative:{parent_aux[14]}:{public[0].name}'


def infer(objects, rows, proposals, units, code_claims):
    runtime = {c['rva'] for c in code_claims if c['symbol'] == '___CxxFrameHandler'
               and not c.get('unit')}
    if len(runtime) != 1:
        return [], []
    by_id = {r['id']: r for r in rows}
    by_position = {(r['unit'], r['section_ordinal'], r['section_offset']): r
                   for r in rows if r['section_ordinal']}
    records, issues = [], []
    disassembler = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    instructions = {}
    for (candidate, rva), refs in sorted(proposals.items()):
        row = by_id[candidate]
        unit, ordinal = row['unit'], row['section_ordinal']
        if not ordinal:
            continue
        obj = objects[unit]
        section = obj.sections[ordinal-1]
        if section.name != '.xdata$x' or row['section_offset']:
            continue
        roots = []
        for ref in refs:
            (oi, sec), parent_rva = ref['parent']
            parent = objects[units[oi]]
            at = ref['site_rva']-parent_rva
            raw = parent.section_bytes(parent.sections[sec-1])
            if (ref['candidate_symbol_offset']+ref['candidate_addend'] or
                    ref['relocation_type'] != 6 or at < 1 or
                    raw[at-1:at] != b'\xb8' or raw[at+4:at+5] != b'\xe9'):
                continue
            key = units[oi], sec
            if key not in instructions:
                instructions[key] = {(address, size): mnemonic for address, size, mnemonic, _ in
                                     disassembler.disasm_lite(raw, 0)}
            if (instructions[key].get((at-1, 5)) != 'mov' or
                    instructions[key].get((at+4, 5)) != 'jmp'):
                continue
            jumps = [r for r in parent.relocations if r.section == sec and r.site == at+5]
            if (len(jumps) == 1 and jumps[0].typ == 20 and
                    parent.symbols[jumps[0].symbol_index].name == '___CxxFrameHandler' and
                    raw[at+5:at+9] == bytes(4)):
                roots.append(ref)
        if not roots:
            continue
        identity = associative_identity(obj, section)
        raw = obj.section_bytes(section)
        relocations = defaultdict(list)
        for relocation in obj.relocations:
            if relocation.section == ordinal:
                relocations[relocation.site].append(relocation)

        def issue(detail):
            issues.append(dict(kind='unsupported-eh-record', unit=unit, candidate_id=candidate,
                               rva=rva, detail=detail))

        def record(offset, size, kind, pointer_site=None):
            owner = by_position.get((unit, ordinal, offset))
            if owner is None or size <= 0 or size > owner['physical_size']:
                raise ValueError(f'{kind}: typed extent has no complete emitted owner')
            if any(s.section == ordinal and s.storage_class == 2 for s in obj.symbols.values()):
                raise ValueError('EH contribution has externally selectable storage')
            result = dict(candidate_id=owner['id'], root_candidate_id=candidate,
                          unit=unit, section_ordinal=ordinal, section_offset=offset,
                          rva=rva+offset, size=size, kind=kind, pointer_site=pointer_site,
                          root_rva=rva, references=roots,
                          linker_identity=f'{identity}:{kind}:{offset}' if identity else '',
                          evidence='checked frame-handler root plus local COFF contribution offsets')
            records.append(result)
            return result

        def pointer(site, size, kind):
            refs = relocations[site]
            if len(refs) != 1 or refs[0].typ != 6:
                raise ValueError(f'{kind}: unique DIR32 relocation required')
            symbol = obj.symbols[refs[0].symbol_index]
            addend = struct.unpack_from('<i', raw, site)[0]
            if symbol.section != ordinal or symbol.storage_class != 3:
                raise ValueError(f'{kind}: target leaves the private EH contribution')
            offset = symbol.value+addend
            record(offset, size, kind, site)
            return offset

        try:
            record(0, 28, 'eh-funcinfo')
            magic, states, _, tries, _, ip_count, ip_map = struct.unpack_from('<7I', raw)
            if magic != 0x19930520 or states > 0x1000 or tries > 0x100:
                raise ValueError('unsupported FuncInfo version or map count')
            if states:
                pointer(8, states*8, 'eh-unwind-map')
            if tries:
                start = pointer(16, tries*20, 'eh-try-map')
                for index in range(tries):
                    count, = struct.unpack_from('<I', raw, start+index*20+12)
                    if not 1 <= count <= 64:
                        raise ValueError('unsupported catch-map count')
                    pointer(start+index*20+16, count*16, 'eh-catch-map')
            if ip_count or ip_map or relocations[24]:
                raise ValueError('x86 IP-to-state map is unsupported')
        except ValueError as exc:
            issue(str(exc))
    return records, issues
