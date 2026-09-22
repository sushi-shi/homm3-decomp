"""Overlay source DATA storage intervals without changing retail evidence grades."""
from __future__ import annotations

from collections import Counter, defaultdict


DATA_SECTIONS = {'.rdata', '.data', '.bss'}


def intervals(layout, snapshot):
    """Invalid/ambiguous ranges stay diagnostic; never silently resize to fit."""
    declarations = snapshot['declarations']
    issues = [dict(i) for i in snapshot['issues']]
    usable = []
    by_entity = defaultdict(list)
    for d in declarations:
        by_entity[d['usr'] or d['id']].append(d)
    bad_addresses = set()
    for entity, members in by_entity.items():
        if len({d['rva'] for d in members}) > 1:
            bad_addresses.add(entity)
            for d in members:
                issues.append(dict(kind='conflicting-address', source=d['source'], rva=d['rva'],
                                   detail=f"{d['name']} is assigned multiple retail addresses"))
    for d in declarations:
        if d['status'] != 'sized' or (d['usr'] or d['id']) in bad_addresses:
            continue
        if not any(s.rva <= d['rva'] < d['end'] <= s.rva + s.mapped_size for s in layout.sections):
            issues.append(dict(kind='out-of-section', source=d['source'], rva=d['rva'], end=d['end'],
                               detail=f"{d['name']}: declared storage crosses or falls outside a PE section"))
            continue
        usable.append(d)
    return usable, issues


def overlay(layout, rows, snapshot):
    usable, issues = intervals(layout, snapshot)
    issue_anchors = defaultdict(list)
    for i, issue in enumerate(issues):
        if issue.get('rva') is not None:
            issue_anchors[issue['rva']].append(i)
    # Declared byte coverage and retail object identification are orthogonal.
    result, overlap_ranges = [], []
    for domain in ('file', 'image'):
        events = defaultdict(lambda: [set(), set()])
        extra_anchors = set()
        domain_regions = layout.file_regions if domain == 'file' else layout.image_regions
        for region in domain_regions:
            if region.rva is None:
                continue
            extra_anchors.update(region.start + rva - region.rva for rva in issue_anchors
                                 if region.rva <= rva < region.rva + region.size)
            for i, d in enumerate(usable):
                lo = max(d['rva'], region.rva)
                hi = min(d['end'], region.rva + region.size)
                if lo < hi:
                    events[region.start + lo - region.rva][1].add(i)
                    events[region.start + hi - region.rva][0].add(i)
        cuts = sorted(set(events) | extra_anchors)
        cursor, active = 0, set()
        for row in (r for r in rows if r['domain'] == domain):
            while cursor < len(cuts) and cuts[cursor] < row['start']:
                removed, added = events[cuts[cursor]]
                active.difference_update(removed)
                active.update(added)
                cursor += 1
            bounds = [row['start']]
            while cursor < len(cuts) and cuts[cursor] < row['end']:
                if cuts[cursor] != row['start']:
                    bounds.append(cuts[cursor])
                cursor += 1
            bounds.append(row['end'])
            for start, end in zip(bounds, bounds[1:]):
                removed, added = events[start]
                active.difference_update(removed)
                active.update(added)
                ds = [usable[i] for i in sorted(active)]
                entities = {d['usr'] or d['id'] for d in ds}
                offset = start - row['start']
                rva = row['rva'] + offset if row['rva'] is not None else None
                disk = row['file_offset'] + offset if row['file_offset'] is not None else None
                scoped = row['section'] in DATA_SECTIONS or bool(ds)
                state = 'not-data-scope' if not scoped else 'overlap' if len(entities) > 1 else 'covered' if ds else 'uncovered'
                definition = ('defined' if any(d['definition'] for d in ds) else 'extern-only' if ds else 'none')
                copied = dict(row, start=start, end=end, size=end - start, rva=rva, file_offset=disk,
                              va=layout.base + rva if rva is not None else None)
                if offset:
                    copied.update(labels=[], label_source=[], incoming_references=0, outgoing_references=0)
                blob = layout.data[disk:disk + min(end - start, 32)] if disk is not None else b''
                copied.update(preview_hex=blob.hex(), preview_ascii=''.join(chr(b) if 32 <= b < 127 else '.' for b in blob),
                              data_coverage_status=state, declaration_ids=sorted(d['id'] for d in ds),
                              declared_owners=sorted({d['name'] for d in ds}),
                              declared_ranges=sorted({f"0x{d['rva']:x}:0x{d['end']:x}" for d in ds}),
                              declaration_sources=sorted({d['source'] for d in ds}),
                              definition_status=definition,
                              definition_sources=sorted({s for d in ds for s in d['definition_sources']}),
                              size_evidence=sorted({d['size_evidence'] for d in ds}),
                              coverage_issues=issue_anchors.get(rva, []), data_analysis_complete=snapshot['analysis_complete'])
                if state == 'overlap':
                    copied['data_next_action'] = 'Review all overlapping DATA declarations; establish aliasing or correct inconsistent ranges.'
                    if domain == 'image':
                        overlap_ranges.append(dict(kind='overlapping-declarations', rva=rva, end=rva + end - start,
                                                   source='; '.join(copied['declaration_sources']),
                                                   detail=', '.join(copied['declared_owners']), declaration_ids=copied['declaration_ids']))
                elif state == 'uncovered':
                    copied['data_next_action'] = 'No sized DATA declaration covers these bytes; inspect references and unresolved annotation/type issues.'
                elif definition == 'extern-only':
                    copied['data_next_action'] = 'Declared range has no parsed source definition; locate or recover the storage definition.'
                elif state == 'covered':
                    copied['data_next_action'] = 'Declared storage covers this range; bind VC6 storage and compare initializer bytes and pointer referents separately.'
                else:
                    copied['data_next_action'] = 'Outside the data-section denominator; retained in complete retail accounting.'
                result.append(copied)
    # Merge adjoining overlap fragments introduced by independent byte-map anchors.
    for issue in overlap_ranges:
        if issues and issues[-1]['kind'] == issue['kind'] and issues[-1].get('end') == issue['rva'] and issues[-1].get('declaration_ids') == issue['declaration_ids']:
            issues[-1]['end'] = issue['end']
        else:
            issues.append(issue)
    counts = Counter({key + '_bytes': 0 for key in
                      ('total', 'covered', 'overlap', 'uncovered', 'declared', 'defined', 'extern-only')})
    by_section = defaultdict(Counter)
    for row in result:
        if row['domain'] != 'image' or row['data_coverage_status'] == 'not-data-scope':
            continue
        size, status = row['size'], row['data_coverage_status']
        counts['total_bytes'] += size
        counts[status + '_bytes'] += size
        by_section[row['section']][status + '_bytes'] += size
        if status in ('covered', 'overlap'):
            counts['declared_bytes'] += size
            counts[row['definition_status'] + '_bytes'] += size
    summary = dict(counts, by_section={s: dict(c) for s, c in by_section.items()},
                   analysis_complete=snapshot['analysis_complete'],
                   declaration_count=len(snapshot['declarations']), usable_declarations=len(usable),
                   issue_counts=dict(Counter(i['kind'] for i in issues)),
                   source_fingerprint=snapshot['fingerprint'],
                   statement='Declared/source-definition coverage only; emitted storage and byte matching are not verified.')
    return result, issues, summary


def gaps(rows):
    """All data-section holes, including bytes already identified on retail."""
    return [r for r in rows if r['domain'] == 'image' and r['data_coverage_status'] == 'uncovered']
