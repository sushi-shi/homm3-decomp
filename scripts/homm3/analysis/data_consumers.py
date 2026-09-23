"""Cross-function storage roles and independently typed array dimensions."""
from collections import defaultdict


def scale_changes(retail, candidate):
    """Unique equal factor sets expose changed byte coefficients, not intent.

    Pairing uses the complete symbolic factor set, never instruction ordinal or
    a source variable's suggestive name. Ambiguous repeated shapes stay unpaired.
    """
    def grouped(profile):
        result = defaultdict(list)
        for row in profile['accesses']:
            expression = row['address']
            if expression is None:
                continue
            factors = tuple(f for f, _ in expression.terms)
            result[row['access'], row['width'], factors].append(row)
        return result
    left, right = grouped(retail), grouped(candidate)
    result = []
    for key in left.keys() & right.keys():
        if len(left[key]) != 1 or len(right[key]) != 1:
            continue
        a, b = left[key][0], right[key][0]
        for (factors, x), (_, y) in zip(a['address'].terms, b['address'].terms):
            if not factors or x == y:
                continue
            result.append(dict(retail_site=a['site'], candidate_site=b['site'], access=a['access'],
                               width=a['width'], factors=factors, retail_byte_coefficient=x,
                               candidate_byte_coefficient=y,
                               evidence='same independent symbolic factors; different raw byte-address coefficient',
                               verdict='address-relationship-difference; runtime effect not proved'))
    return sorted(result, key=repr)


def range_changes(retail, candidate):
    def grouped(profile):
        rows = defaultdict(list)
        for row in profile['accesses']:
            if row['address'] is not None:
                rows[row['access'], row['width'], row['address']].append(row)
        return rows
    left, right = grouped(retail), grouped(candidate)
    changes = []
    for key in left.keys() & right.keys():
        if len(left[key]) == len(right[key]) == 1:
            a, b = left[key][0], right[key][0]
            if a['bounds'] != b['bounds']:
                changes.append(dict(retail_site=a['site'], candidate_site=b['site'], access=a['access'],
                    width=a['width'], address=a['address'], retail_bounds=a['bounds'], candidate_bounds=b['bounds'],
                    verdict='different-proved-index-domains; path feasibility and runtime effect not proved'))
    return sorted(changes, key=repr)


def relationships(accesses, comparisons, declared, projections):
    source = defaultdict(list)
    for row in declared['declarations']:
        source[row['rva']].append(dict(id=row['id'], name=row['name'], source=row['source'],
            size=row['size'], shapes=row.get('shapes', []), shape_conflict=row.get('shape_conflict', False)))
    roles, all_roles = defaultdict(set), defaultdict(lambda: defaultdict(set))
    for row in accesses:
        owner = row['extent']['owner_rva']
        if owner is not None:
            roles[row['function_id'], row['access']].add(owner)
            all_roles[owner][row['side'], row['access']].add(row['function_id'])
    differences = []
    for pair in comparisons:
        for mode in ('read', 'write'):
            left, right = (roles[pair[k], mode] for k in ('retail_function_id', 'candidate_function_id'))
            if left == right:
                continue
            differences.append(dict(kind='different-observed-storage', unit=pair['unit'], symbol=pair['symbol'],
                rva=pair['rva'], access=mode, retail_only=sorted(left-right), candidate_only=sorted(right-left),
                complete_local_observation=pair['complete_observation'],
                detail='entry-paired consumer bases differ; unobserved/conditional effects are not proved absent'))
    storage = []
    for owner in sorted(set(all_roles) | {p['rva'] for p in projections}):
        row = dict(rva=owner, declarations=source[owner])
        for side in ('retail', 'candidate'):
            for mode in ('read', 'write'):
                row[side+'_'+mode+'_functions'] = sorted(all_roles[owner][side, mode])
        storage.append(row)
    return storage, differences
