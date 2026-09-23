"""Independent vendor storage bindings from pinned COFF code contributions.

No data initializer is read to establish an address. A changed initializer must
remain enrolled and differ in the ordinary raw-byte comparison. Code roots and
code-to-code edges are fixed-byte checked; their data references establish spans.
Conflicting placements, missing roots and unsupported edges never become proof.
"""
from collections import Counter, defaultdict, deque
import hashlib
import json
from pathlib import Path

from homm3.analysis import candidate_data, vendor_data


def unit_name(obj):
    identity = hashlib.sha256((obj.library+'\0'+obj.member).encode()).hexdigest()[:20]
    return 'vendor_'+identity


def linker_identity(coff, row):
    """Only explicit COFF coalescing rules identify copies across members.

    Equal bytes (especially zero bytes) do not identify arbitrary storage.
    This identity never selects a passing copy: strict comparison retains every
    enrolled copy, and disagreements receive the worst applicable verdict.
    """
    names = [n for n, scope in zip(row['symbols'], row['scopes']) if scope == 'external']
    if len(names) != 1:
        return ''
    if row['allocation'] == 'common-request':
        return 'common:'+names[0]
    section = coff.sections[row['section_ordinal']-1]
    if (not section.characteristics & 0x1000 or row['section_offset'] or
            row['physical_size'] != section.raw_size):
        return ''
    headers = [s for s in coff.symbols.values() if s.section == section.index and
               s.name == section.name and s.storage_class == 3 and s.aux_count == 1]
    if len(headers) != 1:
        return ''
    selection = coff.data[headers[0].offset+18+14]
    return f'comdat:{selection}:{names[0]}' if selection in (2, 3, 4) else ''


def locate(layout, objects, seeds, *, source_definitions=None):
    nodes, definitions, symbol_nodes = vendor_data.graph_nodes(objects)
    # Ordinary duplicate definitions in different archive members are choices,
    # not aliases. A code reference cannot by itself identify the chosen layout.
    coalescible = {}
    for oi, obj in enumerate(objects):
        for row in candidate_data.inventory(unit_name(obj), obj.coff, obj.digest):
            section = row['section_ordinal'] if row['section_ordinal'] else -row['symbol_indices'][0]-1
            key = (oi, section)
            if row['section_offset'] == 0 and key in nodes and row['physical_size'] == len(nodes[key]['raw']):
                coalescible[key] = linker_identity(obj.coff, row)
    queue, roots = deque(), set()
    for key, node in nodes.items():
        if not node['code']:
            continue
        for name, offset in node['symbols']:
            for address in seeds.get((node['object'].unit, name), ()):
                marker = (key, address-offset)
                roots.add(marker)
                queue.append(marker)
    seen, accepted, rejected, locations = set(), {}, {}, defaultdict(set)
    edges, references, data_locations = defaultdict(list), defaultdict(list), defaultdict(set)
    while queue:
        marker = queue.popleft()
        if marker in seen:
            continue
        seen.add(marker)
        key, rva = marker
        node = nodes[key]
        try:
            if not any(s.flags & 0x20000000 and s.rva <= rva < rva+len(node['raw']) <=
                       s.rva+s.mapped_size for s in layout.sections):
                raise ValueError('code contribution is outside executable retail storage')
            fixed = vendor_data.compare(layout, rva, node['raw'], node['relocs'], code=True)
        except ValueError as exc:
            rejected[marker] = str(exc)
            continue
        accepted[marker] = fixed
        locations[key].add(rva)
        for ref in node['relocs']:
            symbol = node['object'].coff.symbols[ref.symbol_index]
            target = vendor_data.relocation_target(layout, rva, node['raw'], ref)
            anchors = seeds.get((node['object'].unit, symbol.name), set()) | seeds.get(('', symbol.name), set())
            if anchors and anchors != {target}:
                rejected[marker] = f'code reference at {ref.site:#x} contradicts admitted {symbol.name}'
            targets = ([symbol_nodes[key[0], symbol.index]] if (key[0], symbol.index) in symbol_nodes
                       else definitions.get(symbol.name, []))
            for child, offset in targets:
                destination = (child, target-offset)
                if nodes[child]['code']:
                    edges[marker].append(destination)
                    queue.append(destination)
                else:
                    references[destination].append(dict(parent=marker, site_rva=rva+ref.site,
                        target_symbol=symbol.name, symbol_offset=offset,
                        alternatives=[list(k) for k, _ in targets],
                        ambiguous_definition=len(targets) > 1 and not all(
                            coalescible.get(k) and coalescible[k] == coalescible.get(child) and
                            off == offset and len(nodes[k]['raw']) == len(nodes[child]['raw'])
                            for k, off in targets),
                        relocation_type=ref.typ, evidence='fixed-byte matched code relocation'))
                    data_locations[child].add(target-offset)
    bad = set(rejected) | {m for m in accepted if len(locations[m[0]]) != 1}
    paths = {}
    queue = deque((m, []) for m in sorted((roots & accepted.keys())-bad))
    while queue:
        marker, trail = queue.popleft()
        if marker in paths or marker in bad or marker not in accepted:
            continue
        paths[marker] = [*trail, marker]
        queue.extend((child, paths[marker]) for child in edges[marker])
    placements = []
    for marker, refs in sorted(references.items()):
        key, rva = marker
        node = nodes[key]
        overrides = [dict(node=list(other), symbol=name, offset=offset)
                     for name, _ in node['symbols'] for other, offset in definitions.get(name, ())
                     if key[1] < 0 and other[1] > 0]
        if key[1] < 0:
            overrides.extend(definition for name, _ in node['symbols']
                             for definition in (source_definitions or {}).get(name, ()))
        surviving = [r for r in refs if r['parent'] in paths and not r['ambiguous_definition']]
        # A conflicting code root is not allowed to select the most convenient
        # data placement. All independently proposed placements remain visible.
        status = ('conflicting-code-placement' if len(data_locations[key]) != 1 else
                  'ambiguous-archive-definition' if not surviving and any(
                      r['parent'] in paths and r['ambiguous_definition'] for r in refs) else
                  'unproved-code-path' if not surviving else 'bound')
        if status == 'bound' and overrides:
            # A COMMON record requests storage but supplies no initializer.
            # An archive's strong definition can override it. Selection must
            # be proved before treating the request as zero-filled storage;
            # do not select a definition by looking at its initializer bytes.
            status = 'common-initializer-unproved'
        if not any(s.name in ('.data', '.rdata', '.bss') and
                   s.rva <= rva < rva+len(node['raw']) <= s.rva+s.mapped_size for s in layout.sections):
            status = 'outside-retail-data'
        placements.append(dict(node=key, rva=rva, size=len(node['raw']), status=status,
            possible_strong_definitions=overrides,
            references=[dict(r, code_path=paths.get(r['parent'], [])) for r in refs]))
    code = [dict(node=m[0], rva=m[1], fixed_bytes=accepted.get(m, 0), root=m in roots,
                 status='anchored' if m in paths else 'unproved',
                 detail=rejected.get(m, 'conflicting code placement' if m in bad else ''),
                 code_path=paths.get(m, [])) for m in sorted(seen)]
    return placements, code, nodes


def bind(layout, objects, seeds, *, first_id=0, source_definitions=None):
    placements, code, nodes = locate(layout, objects, seeds, source_definitions=source_definitions)
    rows, bindings, used, inventory = [], [], {}, {}
    # Code-only ancestors also belong in the provenance ledger: every path in
    # the exported proof must lead back to a reconstructible archive member.
    provenance = [dict(object_index=i, unit=unit_name(obj), library=obj.library,
                       member=obj.member, origin=obj.origin, object_sha256=obj.digest)
                  for i, obj in enumerate(objects)]
    for placement in placements:
        oi, section = placement['node']
        obj = objects[oi]
        unit = unit_name(obj)
        if unit not in inventory:
            inventory[unit] = candidate_data.inventory(unit, obj.coff, obj.digest)
            rows.extend(inventory[unit])
            used[unit] = obj
        candidates = [r for r in inventory[unit] if
            (r['section_ordinal'] == section if section > 0 else
             r['allocation'] == 'common-request' and -section-1 in r['symbol_indices'])]
        for row in candidates:
            bindings.append(dict(id=first_id+len(bindings), name='|'.join(row['symbols']), macro='VENDOR',
                source=obj.library+':'+obj.member, declaration_id='',
                rva=placement['rva']+row['section_offset'], size=row['physical_size'], units=[unit],
                candidate_ids=[row['id']], status=placement['status'],
                proof='pinned COFF contribution placed only by independently anchored code references',
                retail_extent='COFF span; individual source boundary unproved', candidate_match='not-compared',
                literal_sha256='', source_identity=f'{obj.library}:{obj.member}:{section}:{row["section_offset"]}',
                linker_identity=linker_identity(obj.coff, row),
                vendor_node=list(placement['node']), references=placement['references'],
                possible_strong_definitions=placement['possible_strong_definitions']))
    # Rejected alternatives retain their inventory and binding diagnostics, but
    # never receive coverage.
    code_claims = []
    for anchor in code:
        if anchor['status'] != 'anchored':
            continue
        oi, section = anchor['node']
        if unit_name(objects[oi]) not in used:
            continue
        for symbol in objects[oi].coff.symbols.values():
            if symbol.section == section and symbol.typ & 0x20:
                code_claims.append(dict(unit=unit_name(objects[oi]), symbol=symbol.name,
                    rva=anchor['rva']+symbol.value, evidence='independently matched vendor code path'))
    return dict(candidate_data=rows, data_bindings=bindings, objects=used, code_claims=code_claims,
                provenance=provenance, placements=placements, code=code,
                summary=dict(objects=len(used), placements=len(placements),
                    binding_statuses=dict(Counter(b['status'] for b in bindings)),
                    code_statuses=dict(Counter(r['status'] for r in code))))


def extract(root, layout, *, first_id=0, build=False, source_objects=None):
    objects, seeds, issues, hashes = vendor_data.inputs(root, build=build)
    source_definitions = defaultdict(list)
    for unit, obj in (source_objects or {}).items():
        for symbol in obj.symbols.values():
            if symbol.section > 0 and symbol.storage_class == 2:
                source_definitions[symbol.name].append(dict(unit=unit, symbol=symbol.name,
                    section=symbol.section, offset=symbol.value))
    report = bind(layout, objects, seeds, first_id=first_id, source_definitions=source_definitions)
    directory = root/'build/gen/vendor-bindings'
    directory.mkdir(parents=True, exist_ok=True)
    for unit, obj in report['objects'].items():
        path = directory/(unit+'.obj')
        if not path.is_file() or path.read_bytes() != obj.coff.data:
            path.write_bytes(obj.coff.data)
        hashes[str(path)] = obj.digest
    report['object_paths'] = {unit: str(directory/(unit+'.obj')) for unit in report['objects']}
    # Keep source/header/compiler freshness independently checkable at objdiff
    # invocation time, even when the private objects themselves are unchanged.
    for obj in objects:
        if obj.library == 'zlib-1.1.3':
            stamp = Path(obj.origin+'.compile.json')
            if stamp.is_file():
                hashes[str(stamp)] = hashlib.sha256(stamp.read_bytes()).hexdigest()
            else:
                issues.append(dict(kind='missing-compiler-provenance', library=obj.library,
                                   member=obj.member, detail=str(stamp)))
    for module in (__file__, vendor_data.__file__, candidate_data.__file__):
        path = Path(module)
        hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
    report['input_sha256'] = hashes
    report['issues'] = issues
    return report


def export(report, directory):
    from homm3.core import tsv
    directory = Path(directory)
    for name, key in (('vendor-data-bindings', 'data_bindings'), ('vendor-code-anchors', 'code'),
                      ('vendor-binding-objects', 'provenance'), ('vendor-binding-issues', 'issues')):
        rows = report.get(key, [])
        fields = list(dict.fromkeys(k for row in rows for k in row)) or ['detail']
        tsv.write(directory/(name+'.tsv'), ['# Vendor identities from independent code evidence; data fields supply no addresses.'],
                  fields, [{k: json.dumps(v, sort_keys=True) if isinstance(v, (dict, list, tuple, bool)) else
                            '' if v is None else v for k, v in row.items()} for row in rows])
