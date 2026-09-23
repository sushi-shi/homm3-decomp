"""Locate source storage and compiler pools through independently checked code.

Named allocations use compiler-derived source extents. Complete COMDAT symbol
spans retain their distinct COFF extent evidence. Neither data values nor pointer initializer
fields supply addresses. Ordinary candidate section spacing is never imposed on
retail: each allocation needs its own code reference. Typed private EH maps use
local offsets within one code-anchored compiler contribution.
"""
from bisect import bisect_right
from collections import Counter, defaultdict
import hashlib
import json
import re

from homm3.analysis import candidate_data, code_reference_roles, compiler_eh, data_declarations, vendor_bindings, vendor_data


def definitions(declared, rows):
    """Join unannotated typed definitions using the existing VC6 symbol rules."""
    by_unit, result, by_usr = defaultdict(list), defaultdict(list), defaultdict(list)
    for row in rows:
        by_unit[row['unit']].append(row)
    names = {unit: candidate_data.named_index(values) for unit, values in by_unit.items()}
    for unit in declared['units']:
        if not unit.get('errors'):
            for fact in unit.get('storage_declarations', []):
                if fact.get('usr'):
                    by_usr[fact['usr']].append(fact)
    for unit in declared['units']:
        if unit.get('errors'):
            continue
        for fact in unit.get('definitions', []):
            declaration = dict(fact, symbols=[fact['symbol']], definition_units=[unit['unit']],
                               units=[unit['unit']])
            matches = candidate_data.named_matches(declaration, by_unit[unit['unit']],
                index=names.get(unit['unit'], {}))
            choices = [row for row in by_unit[unit['unit']] if row['id'] in matches]
            for row in choices:
                result[row['id']].append(dict(fact, emitted_choices=len(choices),
                    symbol_matches=matches[row['id']],
                    declarations=by_usr[fact['usr']]))
    return result


def extent(row, facts, bindings, coff, records=()):
    """Keep source size, physical span and unknown extent distinguishable."""
    declarations = [d for f in facts for d in f.get('declarations', [])]
    sizes = {f['size'] for f in facts+declarations if f.get('size')}
    sources = {f['usr'] for f in facts if f.get('usr')}
    sizes.update(b['size'] for b in bindings if b.get('size'))
    if len(sizes) > 1:
        return None, 'source', 'conflicting-source-size'
    shapes = [json.loads(s) for s in sorted({json.dumps(f['shape'], sort_keys=True)
              for f in facts+declarations if 'shape' in f})]
    if any(data_declarations.conflicting_shapes(a, b) for i, a in enumerate(shapes) for b in shapes[i+1:]):
        return None, 'source', 'conflicting-source-shape'
    if len(sources) > 1 or any(f['emitted_choices'] != 1 for f in facts):
        return None, 'source', 'ambiguous-source-definition'
    if sizes:
        size = next(iter(sizes))
        if size > row['physical_size']:
            return size, 'source', 'truncated-storage'
        return size, 'source', 'bound'
    if records:
        extents = {(r['kind'], r['size']) for r in records}
        if len(extents) != 1:
            return None, 'compiler-record', 'conflicting-compiler-extent'
        return records[0]['size'], 'compiler-record', 'bound'
    # A narrow, complete scalar suffix is positive VC6 type evidence even when
    # Clang skipped the owning body. Arrays, records and pointers deliberately
    # require other evidence; neither padding nor a symbol's suggestive name
    # establishes their extent.
    widths = dict(C=1, D=1, E=1, F=2, G=2, H=4, I=4, J=4, K=4, M=4, N=8,
                  _J=8, _K=8, _N=1)
    scalars = [re.fullmatch(r'_?\?.+@[0-4](_J|_K|_N|[CDEFGHIJKMN])[ABCD]', name)
               for name in row['symbols']]
    if len(scalars) == 1 and scalars[0]:
        size = widths[scalars[0][1]]
        return size, 'compiler-type', 'bound' if size <= row['physical_size'] else 'truncated-storage'
    if vendor_bindings.linker_identity(coff, row) and row['section_ordinal']:
        return row['physical_size'], 'coff-contribution', 'bound'
    return None, 'unknown', 'source-extent-unproved'


def bind(layout, objects, rows, source_bindings, declared, code_claims, *, first_id=0):
    """Reuse code-only paths; split source data references at emitted owners."""
    units = sorted(({r['unit'] for r in rows} | {u['unit'] for u in declared['units']}) & objects.keys())
    contributions = [vendor_data.Object('source', unit+'.obj', unit,
        hashlib.sha256(objects[unit].data).hexdigest(), objects[unit], unit) for unit in units]
    seeds = defaultdict(set)
    for claim in code_claims:
        seeds[claim.get('unit') or '', claim['symbol']].add(claim['rva'])
    strong = defaultdict(list)
    for unit, obj in objects.items():
        for symbol in obj.symbols.values():
            if symbol.section > 0 and symbol.storage_class == 2:
                strong[symbol.name].append(dict(unit=unit, symbol=symbol.name,
                    section=symbol.section, offset=symbol.value))
    placements, code, _ = vendor_bindings.locate(layout, contributions, seeds, source_definitions=strong)
    by_section, by_common, existing = defaultdict(list), {}, defaultdict(list)
    for row in rows:
        if row['section_ordinal']:
            by_section[row['unit'], row['section_ordinal']].append(row)
        else:
            for index in row['symbol_indices']:
                by_common[row['unit'], index] = row
    for values in by_section.values():
        values.sort(key=lambda r: r['section_offset'])
    for binding in source_bindings:
        for candidate in binding['candidate_ids']:
            existing[candidate].append(binding)
    typed = definitions(declared, rows)
    proposals = defaultdict(list)
    issues = []
    for placement in placements:
        oi, ordinal = placement['node']
        unit = units[oi]
        for ref in placement['references']:
            if not ref['code_path'] or ref['ambiguous_definition']:
                continue
            (parent_oi, section), parent_rva = ref['parent']
            parent = contributions[parent_oi].coff
            reloc = next(r for r in parent.relocations if r.section == section and
                         parent_rva+r.site == ref['site_rva'])
            symbol = parent.symbols[reloc.symbol_index]
            raw = parent.section_bytes(parent.sections[section-1])
            addend = int.from_bytes(raw[reloc.site:reloc.site+4], 'little', signed=True)
            symbol_offset = ref['symbol_offset']
            section_symbol = bool(symbol.section > 0 and symbol.aux_count and
                                  symbol.name == parent.sections[symbol.section-1].name)
            if ordinal < 0:
                row = by_common.get((unit, -ordinal-1))
            else:
                choices = by_section[unit, ordinal]
                position = symbol_offset+addend if section_symbol else symbol_offset
                index = bisect_right([r['section_offset'] for r in choices], position)-1
                row = choices[index] if index >= 0 else None
                if row and not row['section_offset'] <= position < row['section_offset']+row['physical_size']:
                    row = None
            if row is None:
                issues.append(dict(kind='no-emitted-owner', unit=unit, site_rva=ref['site_rva'],
                                   symbol=symbol.name, addend=addend))
                continue
            rva = placement['rva']+row['section_offset']
            # relocation_target removed the candidate's addend. A section-symbol
            # reference identifies the chosen allocation through that same offset;
            # no neighboring allocation gets a placement as a side effect.
            evidence = dict(ref, candidate_id=row['id'], candidate_symbol=symbol.name,
                            candidate_addend=addend, candidate_symbol_offset=symbol_offset,
                            section_symbol=section_symbol, contribution_rva=placement['rva'])
            proposals[row['id'], rva].append(evidence)
    proposal_locations = defaultdict(set)
    for candidate, rva in proposals:
        proposal_locations[candidate].add(rva)
    compiler_roots = {(candidate, rva): refs for (candidate, rva), refs in proposals.items()
                      if len(proposal_locations[candidate]) == 1 and
                      not any(b['rva'] != rva for b in existing.get(candidate, []))}
    records, compiler_issues = compiler_eh.infer(objects, rows, compiler_roots, units, code_claims)
    compiler_extents = defaultdict(list)
    for record in records:
        compiler_extents[record['candidate_id']].append(record)
        if record['candidate_id'] != record['root_candidate_id']:
            for ref in record['references']:
                proposals[record['candidate_id'], record['rva']].append(dict(ref,
                    compiler_contribution_root=record['root_candidate_id'],
                    compiler_record_kind=record['kind']))
    bindings, reference_roles = [], []
    operand_roles = code_reference_roles.Roles()
    by_id = {r['id']: r for r in rows}
    locations = defaultdict(set)
    for candidate, rva in proposals:
        locations[candidate].add(rva)
    for (candidate, rva), refs in sorted(proposals.items()):
        row = by_id[candidate]
        facts, prior = typed[candidate], existing.get(candidate, [])
        size, kind, status = extent(row, facts, prior, objects[row['unit']], compiler_extents[candidate])
        if len(locations[candidate]) != 1:
            status = 'conflicting-code-placement'
        elif prior and any(b['rva'] != rva for b in prior):
            status = 'conflicting-source-address'
        elif row['allocation'] == 'common-request' and any(
                strong.get(name) for name in row['symbols']):
            status = 'common-initializer-unproved'
        elif size and not any(s.name in ('.rdata', '.data', '.bss') and
                s.rva <= rva < rva+size <= s.rva+s.mapped_size for s in layout.sections):
            status = 'outside-retail-data'
        for ref in refs:
            if not size or ref.get('compiler_contribution_root'):
                continue
            relative = ref['candidate_symbol_offset']+ref['candidate_addend']-row['section_offset']
            if 0 <= relative <= size:
                continue
            (parent_oi, ordinal), parent_rva = ref['parent']
            role = operand_roles.inspect(contributions[parent_oi].coff, ordinal, ref['site_rva']-parent_rva)
            # A section-symbol address in padding does not identify the intended
            # logical owner. Only the already-identified named allocation can
            # borrow the comparison-only operand proof for an outside addend.
            accepted = (role['role'] == 'comparison-immediate' and not ref['section_symbol'] and
                        role['addend'] == ref['candidate_addend'] and
                        role['relocation_type'] == ref['relocation_type'])
            ref['extent_role'] = dict(role, candidate_relative_offset=relative,
                logical_size=size, accepted_outside_comparison=accepted)
            reference_roles.append(dict(candidate_id=candidate, rva=rva, source_size=size,
                site_rva=ref['site_rva'], parent=ref['parent'], unit=units[parent_oi],
                candidate_relative_offset=relative, section_symbol=ref['section_symbol'],
                accepted_outside_comparison=accepted, proof=role))
            if not accepted:
                status = 'code-reference-outside-source-extent'
        identity = next((f['usr'] for f in facts if f.get('usr')), '')
        compiler_identities = {r['linker_identity'] for r in compiler_extents[candidate]}
        linker_identity = next(iter(compiler_identities)) if len(compiler_identities) == 1 else ''
        if not linker_identity and size == row['physical_size']:
            linker_identity = vendor_bindings.linker_identity(objects[row['unit']], row)
        bindings.append(dict(id=first_id+len(bindings), name='|'.join(row['symbols']), macro='CODE',
            source=';'.join(sorted({f"{f['path']}:{f['line']}" for f in facts})), declaration_id='',
            rva=rva, size=size, units=[row['unit']], candidate_ids=[candidate], status=status,
            proof='checked EH root and local COFF contribution offsets' if any(
                ref.get('compiler_contribution_root') for ref in refs) else
                'independently matched code reference to one emitted allocation',
            retail_extent='source extent; retail boundary unproved' if kind == 'source' else
                          'compiler scalar type; retail boundary unproved' if kind == 'compiler-type' else
                          'compiler record extent; retail boundary unproved' if kind == 'compiler-record' else
                          'COFF symbol span; source and retail boundaries unproved',
            extent_kind=kind, candidate_match='not-compared', literal_sha256='',
            source_identity=candidate, source_usr=identity,
            linker_identity=linker_identity,
            references=refs, definitions=facts, compiler_records=compiler_extents[candidate]))
    # A wrong code addend can contradict a correctly annotated data owner.
    # Preserve that independent anchor so consumer comparisons can expose the
    # access error; the new code binding remains explicitly non-exact.
    contradicted = {b['candidate_ids'][0] for b in bindings if b['status'] in (
        'conflicting-code-placement', 'conflicting-source-address')}
    emitted_by_usr = defaultdict(set)
    for candidate, facts in typed.items():
        for fact in facts:
            emitted_by_usr[fact['usr']].add(candidate)
    bound_locations = defaultdict(set)
    for binding in source_bindings+bindings:
        if binding['status'] == 'bound':
            for candidate in binding['candidate_ids']:
                bound_locations[candidate].add(binding['rva'])
    declaration_rows = []
    for unit in declared['units']:
        for fact in unit.get('storage_declarations', []):
            emitted = sorted(c for c in emitted_by_usr[fact['usr']] if fact['linkage'] == 'EXTERNAL'
                             or by_id[c]['unit'] == unit['unit'])
            declaration_rows.append(dict(fact, candidate_ids=emitted,
                bound_rvas=sorted({r for c in emitted for r in bound_locations[c]}),
                proposed_code_rvas=sorted({r for c in emitted for r in locations[c]})))
    claims = []
    for anchor in code:
        if anchor['status'] != 'anchored':
            continue
        oi, ordinal = anchor['node']
        obj = contributions[oi].coff
        checked_size = vendor_data.compared_code_size(obj.section_bytes(obj.sections[ordinal-1]))
        for symbol in obj.symbols.values():
            if (symbol.section == ordinal and (symbol.typ & 0x20 or symbol.storage_class == 6) and
                    0 <= symbol.value < checked_size):
                claims.append(dict(unit=units[oi], symbol=symbol.name, rva=anchor['rva']+symbol.value,
                    symbol_index=symbol.index,
                    linkage='EXTERNAL' if symbol.storage_class == 2 else 'INTERNAL',
                    evidence='independently matched source code path'))
    return dict(data_bindings=bindings, source_bindings=list(source_bindings), code=code, issues=issues,
        declarations=declaration_rows, code_claims=claims, reference_roles=reference_roles,
        compiler_records=records, compiler_issues=compiler_issues,
        provenance=[dict(object_index=i, unit=unit, object_sha256=contributions[i].digest)
                    for i, unit in enumerate(units)],
        summary=dict(binding_statuses=dict(Counter(b['status'] for b in bindings)),
                     code_statuses=dict(Counter(c['status'] for c in code)),
                     issue_counts=dict(Counter(i['kind'] for i in issues)),
                     compiler_issue_counts=dict(Counter(i['kind'] for i in compiler_issues)),
                     compiler_record_counts=dict(Counter(r['kind'] for r in records)),
                     source_address_disagreements=len(contradicted & existing.keys()),
                     storage_declarations=len(declaration_rows),
                     outside_reference_roles=dict(Counter(r['proof']['role'] for r in reference_roles)),
                     proved_comparison_references=sum(r['accepted_outside_comparison'] for r in reference_roles)))


def export(report, directory):
    from pathlib import Path
    from homm3.core import tsv
    for filename, key in (('code-data-bindings', 'data_bindings'), ('code-data-anchors', 'code'),
                          ('code-data-objects', 'provenance'), ('code-data-issues', 'issues'),
                          ('code-data-declarations', 'declarations'),
                          ('code-data-reference-roles', 'reference_roles'),
                          ('code-compiler-records', 'compiler_records'), ('code-compiler-issues', 'compiler_issues')):
        rows = report.get(key, [])
        fields = list(dict.fromkeys(k for row in rows for k in row)) or ['detail']
        tsv.write(Path(directory)/(filename+'.tsv'), ['# Allocation addresses from checked code; initializer bytes do not bind addresses.'],
                  fields, [{k: json.dumps(v, sort_keys=True) if isinstance(v, (dict, list, tuple, bool)) or
                            isinstance(v, str) and any(c in v for c in '\t\r\n') else
                            '' if v is None else v for k, v in row.items()} for row in rows])
