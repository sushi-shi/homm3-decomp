"""Instantiate proved read-only machine-code summaries at actual call sites."""
from homm3.analysis.access_expressions import Expression as E, substitute
from homm3.analysis import access_bounds

NONVOLATILE = ('ebx', 'esi', 'edi', 'ebp')


def read_only(profile):
    if profile['issues'] or profile['branch_sites'] or profile['back_edges'] or not profile['returns']:
        return False
    if any(r['access'] == 'write' for r in profile['accesses']):
        return False
    for result in profile['returns']:
        if result['stack'] != E.atom('stack') or result['caller_stack_written']:
            return False
        if any(result['registers'].get(reg) != E.atom('entry-register', reg) for reg in NONVOLATILE):
            return False
    return True


def instantiate(profile, arguments, registers, *, memory_clean, caller_bounds):
    if not read_only(profile):
        return None
    def value(expression):
        return substitute(expression, arguments, registers, memory_clean=memory_clean)
    result = profile['returns'][0]['registers'].get('eax')
    if any(r['registers'].get('eax') != result for r in profile['returns']):
        result = None
    accesses = []
    for row in profile['accesses']:
        address = value(row['address'])
        accesses.append(dict(row, address=address,
            bounds=access_bounds.records(access_bounds.for_expression(address, caller_bounds)),
            status='expression-known' if address is not None else 'address-unproved'))
    return dict(status='read-only-summary', value=value(result), accesses=accesses)


class Summaries:
    def __init__(self, provider, normalize_address, *, max_depth=16):
        self.provider, self.normalize = provider, normalize_address
        self.cache, self.active = {}, set()
        self.max_depth = max_depth

    def analyze(self, key):
        from homm3.analysis.data_accesses import analyze
        if key in self.cache:
            return self.cache[key]
        if key in self.active or len(self.active) >= self.max_depth:
            return None
        self.active.add(key)
        try:
            raw, start = self.provider.body(key)
            def call_effect(target, arguments, registers, memory_clean, bounds):
                callee = self.provider.key(target)
                if callee is None:
                    return None
                profile = self.analyze(callee)
                return instantiate(profile, arguments, registers, memory_clean=memory_clean,
                                   caller_bounds=bounds) if profile is not None else None
            result = analyze(raw, start, relocation=self.provider.relocation_resolver(key),
                             normalize_address=self.normalize, call_pop=self.provider.pop_count,
                             call_effect=call_effect)
            self.cache[key] = result
            return result
        finally:
            self.active.remove(key)
