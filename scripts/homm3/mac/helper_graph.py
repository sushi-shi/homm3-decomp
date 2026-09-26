"""Join PEF references and resolved source calls, preserving coverage gaps."""
from __future__ import annotations

from bisect import bisect_right
from collections import Counter, defaultdict
import hashlib
import csv
from pathlib import Path
import tomllib

from homm3.mac import addresses, source_graph, tables, target_observations
from homm3.mac.relocations import Address


def address(offset):
    return f'mac:0:0x{offset:x}'


def build(root, index, source, *, complete_source_scope):
    policy = root / 'config/mac/campaign.toml'
    deferred = set(tomllib.loads(policy.read_text()).get('deferred_modules', [])) if policy.exists() else set()
    spans = tables.read_functions(root)
    observations = target_observations.read(root, index.pef, spans)
    starts = sorted(spans)
    claims, _, problems = addresses.scan(root)
    if problems:
        raise ValueError('; '.join(problems))
    owners = defaultdict(list)
    for claim in claims:
        owners[claim.offset].append({'file': claim.path, 'line': claim.line,
            'windows_va': f'0x{claim.windows_va:08x}' if claim.windows_va else None,
            'name': claim.label, 'identity': claim.identity})
    by_mac = defaultdict(set)
    for node in source['nodes'].values():
        for offset, size in node['mac']:
            if spans.get(offset) == size:
                by_mac[offset].add(node['id'])
    prefixes = defaultdict(list)
    for node in source['nodes'].values():
        for path, start, end in node.get('declaration_prefixes', []):
            prefixes[path].append((start, end, node['id']))
    for claim in claims:
        if spans.get(claim.offset) != claim.size or claim.offset in by_mac:
            continue
        anchor = getattr(claim, 'anchor', None)
        if anchor is None:
            continue
        matches = {identity for start, end, identity in prefixes[claim.path]
                   if start <= anchor <= end}
        if len(matches) == 1:
            by_mac[claim.offset].update(matches)
    runtime = {r.offset: r for r in tables.read_runtime(root)}
    indirect = {address(o) for o, r in runtime.items() if r.call_kind == 'indirect_tvector'}

    def containing(at):
        if at.section != 0:
            return None
        pos = bisect_right(starts, at.offset) - 1
        if pos < 0 or at.offset >= starts[pos] + spans[starts[pos]]:
            return None
        return starts[pos]

    def function(offset):
        ids = sorted(by_mac.get(offset, []))
        return {'mac': address(offset), 'size': spans.get(offset),
                'observation': observations.get(offset),
                'runtime_label': {'name': runtime[offset].name, 'owner': runtime[offset].owner,
                                  'evidence': runtime[offset].evidence} if offset in runtime else None,
                'source_ids': ids, 'claims': owners.get(offset, []),
                'deferred': any(Path(c['file']).stem in deferred or
                    Path(c['file']).stem.startswith('rmg_') and 'rmg' in deferred
                    for c in owners.get(offset, [])),
                'name': source['nodes'][ids[0]]['name'] if len(ids) == 1 else
                        runtime[offset].name if offset in runtime else None}

    references = defaultdict(list)
    calls = []
    for at, target, kind in index.branches:
        if target.section != 0:
            continue
        caller = containing(at)
        row = {'site': address(at.offset), 'kind': kind,
               'caller': function(caller) if caller is not None else None,
               'callee': function(target.offset)}
        # Keep local branches in raw reference output, but not as function calls.
        if target.offset in spans or kind == 'linked_branch':
            references[target.offset].append(row)
        if kind == 'linked_branch' or (kind == 'branch' and target.offset in spans
                                        and caller != target.offset):
            calls.append(row)
    indirect_sites = []
    for at in index.indirect_branches:
        owner = containing(at)
        indirect_sites.append({'site': f'mac:{at.section}:0x{at.offset:x}',
            'caller': function(owner) if owner is not None else None,
            'state': 'indirect_destination_unresolved'})
    counts = Counter((r['caller']['mac'] if r['caller'] else None, r['callee']['mac']) for r in calls)
    outgoing = defaultdict(list)
    for edge in source['edges']:
        outgoing[edge['caller']].append(edge)
    path_cache = {}
    nested_cache = {}

    def nested_paths(caller_id, target_ids):
        """Show source wrappers that could explain extra retained Mac calls.

        A direct call is the shortest path to its target, so the ordinary path
        search hides other calls reached through inline header helpers.
        Preserve one lead per first source call without treating it as proof
        that the Mac compiler expanded that helper.
        """
        key = (caller_id, tuple(sorted(target_ids)))
        if key not in nested_cache:
            trails, truncated = [], False
            for edge in outgoing[caller_id]:
                if edge['dispatch'] != 'direct' or edge['callee'] in target_ids:
                    continue
                found, limit = source_graph.paths(source, edge['callee'], target_ids,
                                                  max_depth=7)
                truncated |= limit
                trails.extend([edge, *found[target]] for target in sorted(target_ids)
                              if target in found)
            nested_cache[key] = trails, truncated
        return nested_cache[key]
    queue = []
    for row in calls:
        caller, callee = row['caller'], row['callee']
        # The recovery queue concerns source-owned callers. Raw refs below
        # still include every unpaired/library caller, including tail branches.
        if not caller or not caller['claims']:
            continue
        caller_ids, target_ids = caller['source_ids'], set(callee['source_ids'])
        direct, trails, truncated = [], [], False
        state = 'source_caller_unavailable'
        if len(caller_ids) == 1:
            caller_id = caller_ids[0]
            if not target_ids:
                state = 'source_callee_unavailable'
            else:
                direct = [e for e in outgoing[caller_id] if e['callee'] in target_ids]
                if direct:
                    count = counts[(caller['mac'], callee['mac'])]
                    state = 'direct_count_agrees' if len(direct) == count else 'call_count_difference'
                    if count > len(direct):
                        trails, truncated = nested_paths(caller_id, target_ids)
                    if any(e['dispatch'] != 'direct' for e in direct):
                        state = 'dispatch_requires_review'
                else:
                    if caller_id not in path_cache:
                        path_cache[caller_id] = source_graph.paths(source, caller_id, set(source['nodes']))
                    found, truncated = path_cache[caller_id]
                    trails = [found[t] for t in sorted(target_ids) if t in found]
                    state = 'inlining_path_requires_review' if trails else 'missing_source_call'
        if len(caller_ids) == 1 and len(source['nodes'][caller_ids[0]].get('body_views', [])) > 1:
            state = 'source_body_variant_requires_review'
        if row['kind'] == 'branch':
            state = 'tail_transfer_requires_review'
        if callee['mac'] in indirect:
            state = 'indirect_destination_unresolved'
        queue.append({**row, 'state': state, 'direct_source_sites': direct,
                      'source_paths': trails, 'path_search_truncated': truncated,
                      'mac_call_count': counts[(caller['mac'], callee['mac'])],
                      'source_call_count': len(direct) if len(caller_ids) == 1 and target_ids else None})
    source_queue = []
    for edge in source['edges']:
        caller = source['nodes'][edge['caller']]
        callee = source['nodes'].get(edge['callee'])
        if not callee or edge['dispatch'] != 'direct':
            state = 'source_dispatch_requires_review'
        elif not caller['mac']:
            state = 'source_caller_unpaired'
        elif not callee['mac']:
            state = 'source_callee_unpaired'
        elif any(counts[(address(c[0]), address(t[0]))] for c in caller['mac'] for t in callee['mac']):
            state = 'retained_mac_target_present'
        else:
            state = 'mac_inlining_requires_review'
        source_queue.append({**edge, 'state': state, 'caller_name': caller['name'],
                             'callee_name': callee['name'] if callee else None})
    return {'schema': 1, 'target_sha256': hashlib.sha256(index.pef.data).hexdigest(),
            'source': source, 'source_scope_complete': complete_source_scope,
            'coverage': {'source_units': len(source['units']), 'source_functions': len(source['nodes']),
                         'source_calls': len(source['edges']), 'mac_call_sites': len(calls),
                         'mac_direct_link_sites': sum(r['kind'] == 'linked_branch' for r in calls),
                         'mac_tail_transfer_sites': sum(r['kind'] == 'branch' for r in calls),
                         'caller_queue': len(queue),
                         'observed_targets': len(observations),
                         'deferred_call_sites': sum(r['caller']['deferred'] for r in queue), 'states': dict(Counter(r['state'] for r in queue)),
                         'units_with_errors': sum(bool(v) for v in source['diagnostics'].values()),
                         'source_states': dict(Counter(r['state'] for r in source_queue)),
                         'source_body_variants': sum(len(n.get('body_views', [])) > 1 for n in source['nodes'].values()),
                         'implicit_operation_gaps': len(source['gaps']),
                         'raw_indirect_call_sites': len(indirect_sites)},
            'functions': {address(o): function(o) for o in starts},
            'references': {address(o): r for o, r in references.items()},
            'queue': queue,
            'source_queue': source_queue, 'indirect_calls': indirect_sites,
            'branch_scan_census': dict(getattr(index, 'census', {})),
            'caution': 'Call identities/counts and source paths are generated leads, not recovered-source verdicts. '
                       'A nested path requires evidence of Mac inlining. Missing parses, implicit lifetime operations '
                       'and indirect destinations remain coverage gaps.'}


def select(report, selector):
    functions = report['functions']
    if selector.startswith('mac:'):
        from homm3.mac.discovery import parse_address
        at = parse_address(selector)
        key = f'mac:{at.section}:0x{at.offset:x}'
        found = [f for f in functions.values() if f['mac'] == key]
    elif selector.startswith('0x'):
        va = f'0x{int(selector, 0):08x}'
        found = [f for f in functions.values() if any(c['windows_va'] == va for c in f['claims'])]
    else:
        found = [f for f in functions.values() if f['name'] == selector
                 or any(c['name'] == selector for c in f['claims'])]
    if len(found) != 1:
        raise ValueError(f'{selector}: expected one exact function identity, found {len(found)}; use its Mac offset or Windows VA')
    return found[0]


def xrefs(report, index, selector):
    target = select(report, selector)
    source = report['source']
    ids = set(target['source_ids'])
    uses = [e for e in source['edges'] if e['callee'] in ids]
    offset = int(target['mac'].rsplit(':', 1)[1], 0)
    raw = index.xrefs(Address(0, offset))
    return {'target': target, 'source_scope_complete': report['source_scope_complete'],
            'source_units': source['units'], 'diagnostics': source['diagnostics'],
            'mac_references': report['references'].get(target['mac'], []),
            'loader_pointers': raw['loader_pointers'], 'toc_uses': raw['toc_uses'],
            'direct_source_uses': [{**e, 'caller_name': source['nodes'][e['caller']]['name']} for e in uses],
            'source_comparisons': [e for e in report['source_queue'] if e['callee'] in ids],
            'caller_comparisons': [r for r in report['queue'] if r['callee']['mac'] == target['mac']],
            'callee_comparisons': [r for r in report['queue'] if r['caller']['mac'] == target['mac']],
            'caution': report['caution']}


def write_queues(report, output):
    """Compact worker inputs alongside the full graph; regenerate after edits."""
    for suffix, fields, rows in (
        ('calls', ['caller', 'callee', 'site', 'state', 'mac_count', 'source_count', 'deferred',
                   'callee_name', 'callee_operation', 'callee_category'],
         ({'caller': r['caller']['mac'], 'callee': r['callee']['mac'], 'site': r['site'],
           'state': r['state'], 'mac_count': r['mac_call_count'], 'source_count': r['source_call_count'],
           'deferred': r['caller']['deferred'],
           'callee_name': r['callee']['name'] or '',
           'callee_operation': (r['callee'].get('observation') or {}).get('operation', ''),
           'callee_category': (r['callee'].get('observation') or {}).get('category', '')}
          for r in report['queue'])),
        ('source', ['caller', 'callee', 'file', 'line', 'state', 'expression'],
         ({'caller': r['caller_name'], 'callee': r['callee_name'], 'file': r['location']['file'],
           'line': r['location']['line'], 'state': r['state'], 'expression': r['expression']}
          for r in report['source_queue'])),
    ):
        target = output.with_name(output.stem + '-' + suffix + '.tsv')
        temporary = target.with_suffix('.tmp')
        with temporary.open('w', newline='') as stream:
            writer = csv.DictWriter(stream, fields, delimiter='\t')
            writer.writeheader()
            writer.writerows(rows)
        temporary.replace(target)
