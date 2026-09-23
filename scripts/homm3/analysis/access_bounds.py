"""Conservative signed-32 interval facts from actual x86 comparisons.

Unsigned predicates are converted to signed intervals before taking a hull.
We refine only an unmodified scalar atom: solving affine comparisons without a
separate no-wrap proof would turn modular arithmetic into invalid array bounds.
"""
from homm3.analysis.access_expressions import Expression as E

MIN, MAX = -0x80000000, 0x7fffffff
RELATIONS = {'je': 'eq', 'jne': 'ne', 'jl': 'lt', 'jle': 'le', 'jg': 'gt', 'jge': 'ge',
             'jb': 'ult', 'jbe': 'ule', 'ja': 'ugt', 'jae': 'uge', 'js': 'negative', 'jns': 'nonnegative'}
INVERSE = {'eq': 'ne', 'ne': 'eq', 'lt': 'ge', 'le': 'gt', 'gt': 'le', 'ge': 'lt',
           'ult': 'uge', 'ule': 'ugt', 'ugt': 'ule', 'uge': 'ult',
           'negative': 'nonnegative', 'nonnegative': 'negative'}
SWAP = {'eq': 'eq', 'ne': 'ne', 'lt': 'gt', 'le': 'ge', 'gt': 'lt', 'ge': 'le',
        'ult': 'ugt', 'ule': 'uge', 'ugt': 'ult', 'uge': 'ule'}


def intrinsic(atom):
    if atom[0] in ('argument', 'load') and atom[2 if atom[0] == 'argument' else 1] in (1, 2):
        width = atom[2 if atom[0] == 'argument' else 1]
        return 0, (1 << (width*8))-1
    if atom[0] == 'movzx':
        return 0, (1 << (atom[1]*8))-1
    if atom[0] == 'movsx':
        return -(1 << (atom[1]*8-1)), (1 << (atom[1]*8-1))-1
    if atom[0] == 'mask':
        return 0, atom[1]
    return MIN, MAX


def ranges(relation, value):
    if relation == 'negative':
        return [(MIN, -1)]
    if relation == 'nonnegative':
        return [(0, MAX)]
    unsigned = relation.startswith('u')
    if unsigned:
        relation, value = relation[1:], value & 0xffffffff
    low, high = (0, 0xffffffff) if unsigned else (MIN, MAX)
    spans = {'eq': [(value, value)], 'ne': [(low, value-1), (value+1, high)],
             'lt': [(low, value-1)], 'le': [(low, value)],
             'gt': [(value+1, high)], 'ge': [(value, high)]}[relation]
    result = []
    for a, b in spans:
        a, b = max(a, low), min(b, high)
        if a > b:
            continue
        if not unsigned:
            result.append((a, b))
        else:
            if a <= MAX:
                result.append((a, min(b, MAX)))
            if b > MAX:
                result.append((max(a, MAX+1)-0x100000000, b-0x100000000))
    return result


def refine(bounds, predicate, mnemonic, taken):
    """Return refined facts, or None for a provably impossible edge."""
    relation = RELATIONS.get(mnemonic)
    if predicate is None or relation is None:
        return dict(bounds)
    kind, left, right = predicate
    if relation in ('negative', 'nonnegative') and kind != 'test':
        return dict(bounds)  # JS after CMP tests a subtraction, possibly wrapped.
    if left is None or right is None:
        return dict(bounds)
    if not taken:
        relation = INVERSE[relation]
    number = right.number()
    if number is None and left.number() is not None and relation in SWAP:
        left, right, relation = right, left, SWAP[relation]
        number = right.number()
    if number is None:
        return dict(bounds)
    allowed = ranges(relation, number)
    constant = left.number()
    if constant is not None:
        return dict(bounds) if any(a <= constant <= b for a, b in allowed) else None
    if len(left.terms) != 1 or left.terms[0][1] != 1 or len(left.terms[0][0]) != 1:
        return dict(bounds)
    atom = left.terms[0][0][0]
    if atom[0] in ('stack', 'storage'):
        return dict(bounds)
    lo, hi = bounds.get(atom, intrinsic(atom))
    spans = [(max(a, lo), min(b, hi)) for a, b in allowed if max(a, lo) <= min(b, hi)]
    if not spans:
        return None
    result = dict(bounds)
    result[atom] = min(a for a, _ in spans), max(b for _, b in spans)
    return result


def for_expression(expression, explicit):
    if expression is None:
        return {}
    return {atom: explicit.get(atom, intrinsic(atom)) for factors, _ in expression.terms for atom in factors
            if atom[0] not in ('stack', 'storage')}


def records(bounds):
    return [dict(atom=atom, bounds=value) for atom, value in sorted(bounds.items(), key=lambda item: repr(item[0]))]
