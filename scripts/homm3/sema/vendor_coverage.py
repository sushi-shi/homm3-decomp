"""Keep vendor accounting and source DATA coverage as separate measurements."""
from collections import Counter, defaultdict
from dataclasses import asdict

from homm3.sema.coverage import system_claims


def import_contributions(layout):
    """The importing PE proves DLL ownership of its loader records, not DLL code."""
    claims = system_claims(layout.data, [asdict(s) for s in layout.sections], layout.optional,
                           with_library=True)
    return [dict(library=c['library'], member='PE imports', origin='retail import directory',
                 object_sha256='', section='imports', section_index=0,
                 rva=c['rva'], size=c['size'], end=c['rva'] + c['size'], storage='initialized',
                 symbols=[], status='verified', fixed_bytes=c['size'], relocation_count=0,
                 unresolved_relocations=[], conflicting_relocations=[], evidence=[c['evidence']],
                 anchor_path=['retail PE import directory'], detail='')
            for c in claims if c['library']]


def overlay(layout, rows, contributions):
    usable = [c for c in contributions if c['status'] != 'rejected' and
              any(s.rva <= c['rva'] < c['end'] <= s.rva + s.mapped_size for s in layout.sections)]
    result = []
    counts = Counter({key: 0 for key in ('data_bytes', 'data_gap_bytes', 'vendor_verified_bytes',
                                       'vendor_gap_bytes', 'unaccounted_gap_bytes', 'vendor_candidate_bytes',
                                       'vendor_conflict_bytes')})
    by_library = defaultdict(Counter)
    for domain in ('file', 'image'):
        events = defaultdict(lambda: [set(), set()])
        for region in (layout.file_regions if domain == 'file' else layout.image_regions):
            if region.rva is None:
                continue
            for index, c in enumerate(usable):
                lo, hi = max(c['rva'], region.rva), min(c['end'], region.rva + region.size)
                if lo < hi:
                    events[region.start + lo - region.rva][1].add(index)
                    events[region.start + hi - region.rva][0].add(index)
        cuts, cursor, active = sorted(events), 0, set()
        for row in (r for r in rows if r['domain'] == domain):
            bounds = [row['start']]
            while cursor < len(cuts) and cuts[cursor] < row['end']:
                if cuts[cursor] < row['start']:
                    remove, add = events[cuts[cursor]]
                    active.difference_update(remove)
                    active.update(add)
                elif cuts[cursor] != row['start']:
                    bounds.append(cuts[cursor])
                cursor += 1
            bounds.append(row['end'])
            for start, end in zip(bounds, bounds[1:]):
                remove, add = events[start]
                active.difference_update(remove)
                active.update(add)
                cs = [usable[i] for i in sorted(active)]
                verified = [c for c in cs if c['status'] == 'verified']
                # Distinct overlapping contributions are not declared aliases.
                # Exact same extent from byte-identical archive copies is okay;
                # partial overlaps need independent review.
                extents = {(c['rva'], c['end']) for c in verified}
                conflict = any(c['status'] == 'conflict' for c in cs) or len(extents) > 1
                state = 'conflict' if conflict else 'verified' if verified else 'candidate' if cs else 'unattributed'
                offset = start - row['start']
                rva = row['rva'] + offset if row['rva'] is not None else None
                disk = row['file_offset'] + offset if row['file_offset'] is not None else None
                copied = dict(row, start=start, end=end, size=end - start, rva=rva, file_offset=disk,
                              va=layout.base + rva if rva is not None else None)
                if offset:
                    copied.update(labels=[], label_source=[], incoming_references=0, outgoing_references=0,
                                  coverage_issues=[])
                blob = layout.data[disk:disk + min(end - start, 32)] if disk is not None else b''
                scoped = row['data_coverage_status'] != 'not-data-scope'
                gap = row['data_coverage_status'] == 'uncovered'
                accounting = ('not-data-scope' if not scoped else 'vendor' if gap and state == 'verified' else
                              'unresolved' if gap else 'declaration-conflict' if row['data_coverage_status'] == 'overlap' else 'declared')
                copied.update(preview_hex=blob.hex(), preview_ascii=''.join(chr(b) if 32 <= b < 127 else '.' for b in blob),
                              vendor_status=state, vendor_ids=sorted(c['id'] for c in cs),
                              vendor_libraries=sorted({c['library'] for c in cs}),
                              vendor_verified_libraries=sorted({c['library'] for c in verified}) if state == 'verified' else [],
                              vendor_members=sorted({c['member'] for c in cs}),
                              vendor_evidence=sorted({e for c in cs for e in c['evidence']}),
                              data_accounting_status=accounting)
                if accounting == 'vendor':
                    copied['data_next_action'] = 'Vendor contribution accounts for this DATA gap; retain library provenance and relocation checks.'
                elif gap and cs:
                    copied['data_next_action'] = 'Review candidate/conflicting vendor contribution; this DATA gap remains unaccounted.'
                result.append(copied)
                if domain != 'image' or not scoped:
                    continue
                size = end - start
                counts['data_bytes'] += size
                if state in ('verified', 'candidate', 'conflict'):
                    counts['vendor_' + state + '_bytes'] += size
                if gap:
                    counts['data_gap_bytes'] += size
                    counts['vendor_gap_bytes' if state == 'verified' else 'unaccounted_gap_bytes'] += size
                for library in {c['library'] for c in verified} if state == 'verified' else ():
                    by_library[library]['verified_bytes'] += size
                    if gap:
                        by_library[library]['gap_bytes'] += size
    return result, dict(counts, by_library={k: dict(v) for k, v in sorted(by_library.items())},
                        statement='Vendor contribution accounting; source DATA gaps remain separately visible. '
                                  'Per-library totals may share bytes and must not be summed.')


def unaccounted(rows):
    return [r for r in rows if r['domain'] == 'image' and r['data_accounting_status'] == 'unresolved']


def overlap_issues(rows):
    issues = []
    for row in rows:
        if row['domain'] != 'image' or row['vendor_status'] != 'conflict':
            continue
        if issues and issues[-1]['end'] == row['rva'] and issues[-1]['vendor_ids'] == row['vendor_ids']:
            issues[-1]['end'] = row['rva'] + row['size']
        else:
            issues.append(dict(kind='conflicting-vendor-extents', rva=row['rva'], end=row['rva'] + row['size'],
                               library=', '.join(row['vendor_libraries']), member=', '.join(row['vendor_members']),
                               vendor_ids=row['vendor_ids'], detail='Review differing extents or conflicting placements; bytes remain unaccounted.'))
    return issues
