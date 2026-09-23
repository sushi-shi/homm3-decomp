"""CRT registration tables and independently identified initialization effects.

Retail bounds come from calls in an admitted CRT routine matched to the pinned
compiler archive, not runs of plausible pointers. Source data bindings identify
storage independently of the initializer being inspected.
"""
from __future__ import annotations

from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct

from homm3.analysis import vendor_data
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.core import tsv
from homm3.core.cc_wrap import find_ci
from homm3.core.project import Project


def crt_object(root):
    path = find_ci(Project(root).toolchain/'lib', 'LIBCMT.LIB')
    if path is None:
        raise ValueError('pinned LIBCMT.LIB is missing')
    payload = path.read_bytes()
    matches = []
    for offset, member, raw in vendor_data.archive_members(payload):
        if raw[:2] != b'\x4c\x01':
            continue
        obj = CoffObject(raw)
        names = {s.name for s in obj.symbols.values() if s.section > 0 and s.typ & 0x20}
        if {'__cinit', '__initterm'} <= names:
            matches.append((obj, member, offset))
    if len(matches) != 1:
        raise ValueError('CRT startup/initializer helper archive owner is ambiguous')
    return matches[0][0], dict(path=str(path), sha256=hashlib.sha256(payload).hexdigest(),
                              member=matches[0][1], offset=matches[0][2])


def retail_tables(layout, crt, runtime, functions):
    """Validate actual push(end), push(begin), call(_initterm) sequences.

    VC6's helper walks four-byte slots from begin inclusive to end exclusive.
    Both named boundary sentinels are included in accounting; end is not called.
    The helper is independently byte-checked at every call destination.
    """
    import capstone
    cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    tables, issues = [], []
    entries = [s for s in crt.symbols.values() if s.name == '__cinit' and s.section > 0]
    if len(entries) != 1 or len(runtime.get('__cinit', ())) != 1:
        return [], [dict(kind='crt-root-unavailable', detail='unique admitted __cinit entry required')]
    symbol = entries[0]
    start = next(iter(runtime['__cinit']))
    section = crt.sections[symbol.section-1]
    raw = crt.section_bytes(section)
    refs = {r.site: r for r in crt.relocations if r.section == section.index}
    if symbol.value != 0 or functions.get(start) != len(raw):
        return [], [dict(kind='crt-root-extent', detail='CRT entry does not own its admitted extent')]
    try:
        vendor_data.compare(layout, start, raw, list(refs.values()), code=True)
        instructions = list(cs.disasm_lite(raw, 0))
        for index, (offset, length, mnemonic, _) in enumerate(instructions):
            if mnemonic != 'call' or length != 5 or index < 2 or offset+1 not in refs:
                continue
            call = refs[offset+1]
            helper = crt.symbols[call.symbol_index]
            if helper.name != '__initterm':
                continue
            pushes = instructions[index-2:index]
            if any(i[1:3] != (5, 'push') or i[0]+1 not in refs for i in pushes):
                raise ValueError('initializer helper arguments are not adjacent pointer pushes')
            end_ref, begin_ref = (refs[i[0]+1] for i in pushes)
            end_name, begin_name = (crt.symbols[r.symbol_index].name for r in (end_ref, begin_ref))
            if (begin_name, end_name) not in (('___xi_a', '___xi_z'), ('___xc_a', '___xc_z')):
                raise ValueError('unsupported CRT boundary symbol pair')
            if call.typ != 20 or any(r.typ != 6 for r in (begin_ref, end_ref)):
                raise ValueError('unexpected CRT argument/call relocation kind')
            if any(struct.unpack_from('<i', raw, r.site)[0] for r in (call, begin_ref, end_ref)):
                raise ValueError('CRT arguments have nonzero raw addends')
            helper_rva = vendor_data.relocation_target(layout, start, raw, call)
            helper_raw = crt.section_bytes(crt.sections[helper.section-1])
            if (helper.value != 0 or functions.get(helper_rva) != len(helper_raw) or
                    layout.read(helper_rva, len(helper_raw)) != helper_raw):
                raise ValueError('retail initializer walker differs from pinned CRT helper')
            begin = vendor_data.relocation_target(layout, start, raw, begin_ref)
            end = vendor_data.relocation_target(layout, start, raw, end_ref)
            if not 0 <= begin < end or begin % 4 or end % 4:
                raise ValueError('invalid initializer table bounds/alignment')
            data = layout.read(begin, end-begin+4)
            slots = list(struct.iter_unpack('<I', data))
            if slots[0][0] or slots[-1][0]:
                raise ValueError('CRT boundary sentinel is nonzero')
            rows = []
            for ordinal, (value,) in enumerate(slots):
                target = value-layout.base if value else None
                if target is not None and not any(s.flags & 0x20 and
                        s.rva <= target < s.rva+s.raw_size for s in layout.sections):
                    raise ValueError(f'initializer slot {ordinal} does not point into code')
                rows.append(dict(ordinal=ordinal, rva=begin+ordinal*4, target_rva=target,
                                 status='sentinel' if ordinal in (0, len(slots)-1) else
                                 'null-slot' if target is None else 'registered' if target in functions
                                 else 'registered-unbounded'))
            tables.append(dict(id=len(tables), kind='c-initializers' if begin_name == '___xi_a' else
                               'cpp-initializers', rva=begin, end=end+4, size=len(data),
                               walk_end=end, caller_rva=start, call_rva=start+offset,
                               walker_rva=helper_rva, begin_symbol=begin_name, end_symbol=end_name,
                               evidence='admitted __cinit + pinned CRT code/relocations + bounded slots', slots=rows))
    except ValueError as exc:
        # A partially validated root must not leave earlier tables credited.
        return [], [dict(kind='invalid-crt-initialization', detail=str(exc))]
    if {t['kind'] for t in tables} != {'c-initializers', 'cpp-initializers'} or len(tables) != 2:
        return [], [dict(kind='missing-crt-table', detail='expected distinct C and C++ initializer walks')]
    return tables, issues


def candidate_slots(objects):
    """Keep every emitted registration, including malformed or missing bodies."""
    result = []
    for unit, obj in objects.items():
        for sec in obj.sections:
            if not sec.name.startswith('.CRT$'):
                continue
            raw = obj.section_bytes(sec)
            relocs = [r for r in obj.relocations if r.section == sec.index]
            if sec.raw_size % 4:
                result.append(dict(unit=unit, section=sec.name, section_ordinal=sec.index,
                    offset=0, target_index=None, target='', status='malformed-section', detail='partial pointer slot'))
                continue
            for offset in range(0, len(raw), 4):
                refs = [r for r in relocs if r.site < offset+4 and r.site+4 > offset]
                row = dict(unit=unit, section=sec.name, section_ordinal=sec.index, offset=offset,
                           target_index=None, target='', status='unresolved', detail='',
                           table_kind={'.CRT$XI': 'c-initializers', '.CRT$XC': 'cpp-initializers'}.get(sec.name[:7], 'unsupported'))
                result.append(row)
                value = struct.unpack_from('<I', raw, offset)[0]
                if not refs and value == 0:
                    row['status'] = 'null-slot'
                    continue
                if len(refs) != 1 or refs[0].site != offset or refs[0].typ != 6 or value:
                    row.update(status='invalid-slot', detail='expected one aligned zero-addend DIR32')
                    continue
                symbol = obj.symbols[refs[0].symbol_index]
                row.update(target_index=symbol.index, target=symbol.name)
                if symbol.section <= 0 or not symbol.typ & 0x20:
                    row.update(status='missing-body', detail='registration has no defined function entry')
                    continue
                code = obj.sections[symbol.section-1]
                if not code.characteristics & 0x20 or not 0 <= symbol.value < code.raw_size:
                    row.update(status='missing-body', detail='registration target is outside emitted code')
                    continue
                row['status'] = 'registered'
    return result


def final_bytes(profile):
    """Known final effects, only when the complete straight-line proof exists."""
    if not profile['complete']:
        return None
    result = {}
    for write in profile['writes']:
        value = write['value']
        if value is None or value[0] != 'integer':
            return None
        for offset in range(write['width']):
            result[write['rva']+offset] = value[1] >> (8*offset) & 255
    return result


def compare_profiles(retail, candidate, retail_callbacks=(), candidate_callbacks=()):
    """Diagnose missing writes and registrations without promoting partial effects."""
    def written(profile):
        return {w['rva']+i for w in profile['writes'] for i in range(w['width'])}
    missing = sorted(written(retail)-written(candidate))
    extra = sorted(written(candidate)-written(retail))
    left, right = final_bytes(retail), final_bytes(candidate)
    changed = sorted(k for k in (left or {}).keys() & (right or {}).keys() if left[k] != right[k])
    registrations = ('different' if len(retail_callbacks) != len(candidate_callbacks) else
                     'unresolved' if any(v is None for v in (*retail_callbacks, *candidate_callbacks)) else
                     'same' if list(retail_callbacks) == list(candidate_callbacks) else 'different')
    if registrations == 'different' and not all(p.get('registration_complete', p['complete']) for p in (retail, candidate)):
        registrations = 'observed-difference-unproved'
    exact = left is not None and right is not None and left == right and registrations == 'same'
    return dict(status='effects-exact' if exact else 'effects-differ' if
                (left is not None and right is not None and left != right) or registrations == 'different'
                else 'effects-unproved', missing_write_bytes=missing, extra_write_bytes=extra,
                changed_value_bytes=changed, registration_status=registrations,
                missing_writes_proven=bool(missing and candidate['complete'] and retail['complete']),
                matched_effect_bytes=len(left) if exact else 0)


def pair_initializers(retail, candidate):
    """Independent entry anchors first, then uniquely shared bound storage.

    Ownership overlap is a pairing lead, not an effect verdict. Wrong writes,
    omitted fields and incorrect values are compared after pairing. Ambiguous
    ownership graphs remain unpaired; array-slot position is never an identity.
    """
    possible = defaultdict(set)
    by_rva = {p['root']: i for i, p in enumerate(retail)}
    for ci, c in enumerate(candidate):
        for rva in c['anchors']:
            if rva in by_rva and c.get('table_kind') == retail[by_rva[rva]].get('table_kind'):
                possible[ci].add(by_rva[rva])
        if possible[ci]:
            continue
        owners = set(c['owners'])
        if owners:
            possible[ci].update(ri for ri, r in enumerate(retail) if c.get('table_kind') == r.get('table_kind')
                                and owners.intersection(r['owners']))
    inverse = defaultdict(set)
    for ci, choices in possible.items():
        for ri in choices:
            inverse[ri].add(ci)
    return {ci: next(iter(choices)) for ci, choices in possible.items()
            if len(choices) == 1 and len(inverse[next(iter(choices))]) == 1}, possible


def order_issues(retail, candidate, pairs):
    """Only order supported by one object's section suffix/slot offsets is asserted."""
    by_unit = defaultdict(list)
    for ci, ri in pairs.items():
        by_unit[candidate[ci]['unit']].append((ci, ri))
    issues = []
    for unit, rows in by_unit.items():
        rows.sort(key=lambda p: (candidate[p[0]]['section'], candidate[p[0]]['section_ordinal'], candidate[p[0]]['offset']))
        for index, (ci, ri) in enumerate(rows):
            for cj, rj in rows[index+1:]:
                if retail[ri]['table_id'] == retail[rj]['table_id'] and retail[ri]['ordinal'] > retail[rj]['ordinal']:
                    before, after = candidate[ci], candidate[cj]
                    writes = [{w['rva']+i for w in p['writes'] for i in range(w['width'])} for p in (before, after)]
                    reads = [{w['rva']+i for w in p.get('reads', []) for i in range(w['width'])} for p in (before, after)]
                    dependent = sorted((writes[0] & writes[1]) | (writes[0] & reads[1]) | (writes[1] & reads[0]))
                    issues.append(dict(kind='initializer-order-inversion', unit=unit, dependency_bytes=dependent,
                        candidate_before=ci, candidate_after=cj, retail_before=retail[rj]['root'],
                        retail_after=retail[ri]['root']))
    return issues


def generate(root, *, evidence=None, static_report=None, claims=None, jobs=4):
    from homm3.analysis.data_functions import Functions
    from homm3.sema import data_match
    evidence = evidence or data_match.prepare(root, jobs=jobs)
    static_report = static_report or data_match.generate(root, evidence=evidence)
    layout, objects, candidate_report = (evidence[k] for k in ('layout', 'objects', 'candidate_report'))
    functions = {int(r['rva'], 0): int(r['size'], 0) for r in tsv.read(root/'config/retail/functions.tsv')[2]}
    runtime = defaultdict(set)
    for row in tsv.read(root/'config/retail/runtime-map.tsv')[2]:
        runtime[row['name']].add(int(row['rva'], 0))
    crt, crt_evidence = crt_object(root)
    tables, issues = retail_tables(layout, crt, runtime, functions)
    analysis_issues = list(issues)
    from homm3.analysis import compiler_rtti
    if claims is None:
        from homm3.sema import retail_claims
        claims = retail_claims.collect(root, layout, Project(root).image)[0]
    rtti = compiler_rtti.collect(root, layout, functions, runtime, claims)
    slots = candidate_slots(objects)
    enrolled = static_report['enrollment']
    conflicted = {i for p in enrolled if p['status'] == 'binding-conflict' for i in p['binding_ids']}
    bindings = [b for b in candidate_report['data_bindings'] if b['id'] not in conflicted]
    identities = data_match.Identities(candidate_report['candidate_data'], bindings, objects, evidence['code_claims'])
    retail_functions = Functions(layout, functions, runtime)
    candidate_functions = Functions(layout, functions, runtime, objects, identities, evidence['code_claims'])
    walks = {t['call_rva'] for t in tables}
    for caller in {t['caller_rva'] for t in tables}:
        for event in retail_functions.analyze(caller)['events']:
            if event['kind'] == 'call' and event['site']-layout.base not in walks:
                issues.append(dict(kind='crt-nontable-call-unverified', caller_rva=caller,
                    site_rva=event['site']-layout.base, target=event['target'],
                    detail='Startup hook outside the proved initializer-array walks'))
    owners = sorted({(p['rva'], p['size']) for p in enrolled if p['status'] != 'binding-conflict'})

    def profile(provider, key):
        result = provider.closure(key)
        result['owners'] = sorted({start for write in result['writes'] for start, size in owners
                                  if start <= write['rva'] < write['rva']+write['width'] <= start+size})
        callbacks = []
        for call in result['calls']:
            if '_atexit' not in call['names']:
                continue
            argument = call['arguments'][0]
            callback = None
            if provider.objects is None:
                target = provider.key(argument)
                callback = target if target is not None else None
            else:
                key_ = provider.key(argument)
                anchors = provider.anchors.get(key_, ())
                if len(anchors) == 1:
                    callback = next(iter(anchors))
            callbacks.append(callback)
        result['callbacks'] = callbacks
        return result

    retail, candidate = [], []
    for table in tables:
        for slot in table['slots']:
            target = slot['target_rva']
            if target is None:
                continue
            if target not in functions:
                issues.append(dict(kind='initializer-body-unbounded', slot_rva=slot['rva'], target_rva=target))
                continue
            retail.append(dict(profile(retail_functions, target), root=target, table_id=table['id'],
                               table_kind=table['kind'], ordinal=slot['ordinal'], slot_rva=slot['rva']))
    for slot in slots:
        if slot['status'] != 'registered':
            if slot['status'] != 'null-slot':
                issues.append(dict(slot, kind='candidate-'+slot['status']))
            continue
        key = (slot['unit'], slot['target_index'])
        candidate.append(dict(profile(candidate_functions, key), root=list(key), anchors=sorted(candidate_functions.anchors[key]),
                              **slot))
    pairs, possible = pair_initializers(retail, candidate)
    comparisons = []
    for ci, ri in sorted(pairs.items()):
        r, c = retail[ri], candidate[ci]
        comparisons.append(dict(candidate_id=ci, retail_id=ri, unit=c['unit'], target=c['target'],
            retail_rva=r['root'], candidate_owners=c['owners'], retail_owners=r['owners'],
            pairing='entry-anchor' if r['root'] in c['anchors'] else 'unique-shared-storage',
            **compare_profiles(r, c, r['callbacks'], c['callbacks'])))
    issues.extend(order_issues(retail, candidate, pairs))
    for ci, row in enumerate(candidate):
        if ci not in pairs:
            issues.append(dict(kind='candidate-initializer-unpaired', candidate_id=ci, unit=row['unit'],
                               target=row['target'], choices=sorted(possible.get(ci, ())), owners=row['owners']))
    paired_retail = set(pairs.values())
    for ri, row in enumerate(retail):
        if ri not in paired_retail:
            issues.append(dict(kind='retail-initializer-unpaired', retail_id=ri, target_rva=row['root'],
                               owners=row['owners'], write_bytes=sum(w['width'] for w in row['writes'])))
    for side, profiles in (('retail', retail), ('candidate', candidate)):
        for index, row in enumerate(profiles):
            row.update(id=index, side=side)
    storage = []
    exact_pairs = {r['retail_id']: r['candidate_id'] for r in comparisons if r['status'] == 'effects-exact'}
    for projection in enrolled:
        lo, hi = projection['rva'], projection['rva']+projection['size']
        writers = [[i for i, p in enumerate(profiles) if any(
            w['rva'] < hi and lo < w['rva']+w['width'] for w in p['writes'])] for profiles in (retail, candidate)]
        r, c = writers
        status = ('no-observed-startup-write' if not r and not c else
                  'retail-startup-write-not-observed-in-candidate' if r and not c else
                  'candidate-startup-write-not-observed-in-retail' if c and not r else
                  'observed-effects-match' if all(i in exact_pairs for i in r) and
                  {exact_pairs[i] for i in r} == set(c) else 'effects-unproved')
        if projection['status'] == 'binding-conflict':
            status = 'binding-conflict'
        storage.append(dict(projection_id=projection['id'], candidate_id=projection['candidate_id'],
            unit=projection['unit'], rva=lo, size=projection['size'], retail_initializers=r,
            candidate_initializers=c, status=status, completion='whole-program initialization unproved'))
    matched_bytes = {address for ri in exact_pairs for address in final_bytes(retail[ri])}
    return dict(schema='homm3.data-initialization.v1', retail_sha256=static_report['retail_sha256'],
        tables=tables, candidate_slots=slots, rtti=rtti,
        profiles=retail+candidate, comparisons=comparisons, storage=storage, issues=issues, analysis_issues=analysis_issues,
        summary=dict(table_bytes=sum(t['size'] for t in tables), retail_slots=sum(
            s['target_rva'] is not None for t in tables for s in t['slots']),
            candidate_slots=len(slots), paired_initializers=len(pairs),
            comparison_statuses=dict(Counter(r['status'] for r in comparisons)),
            matched_effect_bytes=len(matched_bytes), storage_statuses=dict(Counter(r['status'] for r in storage)),
            issue_counts=dict(Counter(i['kind'] for i in issues)), rtti=rtti['summary']),
        policy=dict(pairing='independent entry or uniquely shared source-bound storage; order never supplies identity',
                    effects='direct-call closure; exact effects require supported straight-line constant writes',
                    order='within one emitted object; cross-object link order unverified',
                    completion='byte-zero agreement is not dynamic initialization proof'),
        input_sha256=dict(static_report['input_sha256'], **{crt_evidence['path']: crt_evidence['sha256'],
            **{str(root/f'scripts/homm3/analysis/{name}.py'): hashlib.sha256(
                (root/f'scripts/homm3/analysis/{name}.py').read_bytes()).hexdigest()
               for name in ('data_initialization', 'data_effects', 'data_functions', 'compiler_rtti')},
            **{p: hashlib.sha256((root/p).read_bytes()).hexdigest() for p in (
                'config/retail/vtables.tsv', 'scripts/homm3/sema/retail_claims.py',
                'scripts/homm3/vc6/tryblocks.py', 'scripts/homm3/sema/coverage.py')}}), crt_evidence=crt_evidence)


def structures(report):
    """Records eligible for retail extent accounting; never static match credit."""
    result = [dict(rva=t['rva'], size=t['size'], kind='crt-initializer-table',
                   evidence=t['evidence'], owner_rvas=[t['caller_rva']], class_name='') for t in report['tables']]
    roots = {r['id']: r for r in report['rtti']['roots']}
    for row in report['rtti']['structures']:
        owners = sorted({roots[i]['function_rva'] for i in row['root_ids'] if 'function_rva' in roots[i]})
        result.append(dict(rva=row['rva'], size=row['size'], kind=row['kind'], owner_rvas=owners,
                           evidence='bounded VC6 RTTI graph from validated runtime call, catch map or vtable',
                           class_name=row['fields'].get('name', '')))
    return result


def exact(report):
    return (bool(report['tables']) and not report['issues'] and not report['rtti']['issues'] and
            bool(report['comparisons']) and all(r['status'] == 'effects-exact' for r in report['comparisons']))


def export(report, directory):
    for name, rows, default in [
        ('data-initializer-tables', [{k: v for k, v in t.items() if k != 'slots'} for t in report['tables']], ['id', 'rva', 'size']),
        ('data-initializer-slots', [dict(s, table_id=t['id']) for t in report['tables'] for s in t['slots']], ['table_id', 'rva', 'status']),
        ('candidate-initializer-slots', report['candidate_slots'], ['unit', 'status']),
        ('data-initialization', report['profiles'], ['id', 'side', 'root']),
        ('data-initialization-matches', report['comparisons'], ['candidate_id', 'retail_id', 'status']),
        ('data-initialization-storage', report['storage'], ['projection_id', 'rva', 'status']),
        ('data-initialization-issues', report['issues'], ['kind', 'detail']),
        ('data-rtti', report['rtti']['structures'], ['id', 'rva', 'size']),
        ('data-rtti-roots', report['rtti']['roots'], ['id', 'rva', 'kind']),
        ('data-rtti-issues', report['rtti']['issues'], ['kind', 'detail'])]:
        rendered = [{k: '' if v is None else json.dumps(v) if isinstance(v, (list, dict, tuple)) else v
                     for k, v in r.items()} for r in rows]
        tsv.write(directory/f'{name}.tsv', ['# Initialization evidence; unknown effects are not successful initialization.'],
                  list(dict.fromkeys(k for r in rows for k in r)) or default, rendered)
    (directory/'data-initialization-summary.json').write_text(json.dumps(
        {k: report[k] for k in ('schema', 'retail_sha256', 'summary', 'policy', 'analysis_issues', 'input_sha256', 'crt_evidence')}, indent=2)+'\n')
    tsv.write(directory/'data-rtti-runtime.tsv', ['# Independent pinned-runtime root for TypeDescriptor identity.'],
              ['path', 'sha256', 'destructor_rva', 'typeinfo_vtable_rva'],
              [dict(report['rtti']['proof'], typeinfo_vtable_rva=report['rtti']['summary']['typeinfo_vtable_rva'])])


def run(args):
    from homm3.core.common import HOMM3_DIR
    report = generate(HOMM3_DIR, jobs=args.jobs)
    export(report, Path(args.output))
    print(json.dumps(report['summary'], indent=2))
    return int(bool(report['analysis_issues']) or args.require_exact and not exact(report))


def main(argv=None):
    import argparse
    from homm3.core.common import HOMM3_DIR
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HOMM3_DIR/'build/data-initialization')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--require-exact', action='store_true')
    args = parser.parse_args(argv)
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    return run(args)


if __name__ == '__main__':
    raise SystemExit(main())
