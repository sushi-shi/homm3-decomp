"""Bounded polynomial expressions in x86 *byte* addresses.

Terms carry input/storage identities, never register-allocation identities.
Unsupported operations return unknown instead of silently dropping a factor.
All operations model 32-bit arithmetic; bounded ranges need a separate no-wrap
proof before they can establish an allocation's extent.
"""
from dataclasses import dataclass


def signed(value):
    value &= 0xffffffff
    return value if value < 0x80000000 else value-0x100000000


@dataclass(frozen=True)
class Expression:
    terms: tuple

    @classmethod
    def make(cls, terms):
        grouped = {}
        for factors, coefficient in terms:
            factors = tuple(sorted(factors, key=repr))
            grouped[factors] = signed(grouped.get(factors, 0)+coefficient)
        terms = tuple(sorted(((f, c) for f, c in grouped.items() if c), key=repr))
        if len(terms) > 64 or any(len(f) > 4 for f, _ in terms):
            return None
        return cls(terms)

    @classmethod
    def constant(cls, value):
        return cls.make([((), value)])

    @classmethod
    def atom(cls, *identity):
        return cls.make([((tuple(identity),), 1)])

    def number(self):
        if not self.terms:
            return 0
        if len(self.terms) == 1 and self.terms[0][0] == ():
            return self.terms[0][1]
        return None

    def plus(self, other):
        return self.make((*self.terms, *other.terms)) if other is not None else None

    def times(self, other):
        if other is None:
            return None
        return self.make((a+b, c*d) for a, c in self.terms for b, d in other.terms)

    def offset(self, value):
        return self.plus(self.constant(value))

    def stack_offset(self):
        nonconstant = [(f, c) for f, c in self.terms if f]
        if nonconstant == [((("stack",),), 1)]:
            return sum(c for f, c in self.terms if not f)
        return None


ZERO = Expression.constant(0)


def substitute(expression, arguments, registers, *, memory_clean=True, namespace=None):
    """Instantiate a callee expression without identifying unrelated inputs.

    Argument values are snapshots in the caller and are returned unchanged.
    A callee memory load is a fresh read; it cannot reuse the caller's initial
    memory identity after a mutation. Unsupported/local atoms stay unknown.
    """
    if expression is None:
        return None

    def atom_value(atom):
        kind = atom[0]
        if kind == 'argument':
            offset, width = atom[1:]
            if offset % 4 or offset//4 >= len(arguments) or width not in (1, 2, 4):
                return None
            value = arguments[offset//4]
            if value is None or width == 4:
                return value
            number = value.number()
            return Expression.constant(number & ((1 << (width*8))-1)) if number is not None else Expression.atom('mask', (1 << (width*8))-1, value.terms)
        if kind == 'entry-register':
            return registers.get(atom[1])
        if kind == 'storage':
            return Expression.atom(*atom)
        if kind in ('load', 'mask', 'movsx', 'movzx'):
            inner = substitute(Expression(atom[2]), arguments, registers,
                               memory_clean=memory_clean, namespace=namespace)
            if inner is None or kind == 'load' and not memory_clean:
                return None
            return Expression.atom(kind, atom[1], inner.terms)
        if kind == 'loop-iteration' and namespace is not None:
            return Expression.atom('context-loop', namespace, *atom[1:])
        return None

    result = ZERO
    for factors, coefficient in expression.terms:
        term = Expression.constant(coefficient)
        for atom in factors:
            term = term.times(atom_value(atom)) if term is not None else None
        result = result.plus(term) if result is not None else None
    return result


def affine_range(expression, bounds):
    """Inclusive mathematical range; reject nonlinear and 32-bit wrapping sums.

    bounds maps individual atoms to inclusive (low, high). An unknown bound,
    scaled pointer or nonlinear term is not a finite extent proof.
    """
    if expression is None:
        return None
    lo = hi = 0
    for factors, coefficient in expression.terms:
        if not factors:
            low = high = 1
        elif len(factors) == 1 and factors[0] in bounds:
            low, high = bounds[factors[0]]
        else:
            return None
        a, b = coefficient*low, coefficient*high
        lo += min(a, b)
        hi += max(a, b)
        if lo < -0x80000000 or hi > 0x7fffffff:
            return None
    return lo, hi
