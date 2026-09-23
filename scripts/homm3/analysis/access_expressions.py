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
