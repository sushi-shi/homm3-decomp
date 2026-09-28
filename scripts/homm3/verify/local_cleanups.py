"""Compare compiler-emitted code that claimed source functions reference.

A claimed function can take the address of code the compiler emitted for it
without a source-level name: the `$E` destructor registered with `_atexit`
for a function-local static, or the element constructor/destructor handed to
the vector iterators. Such a body is credited only when the claimed parent's
own current object references it at the corresponding retail site, and the
whole emitted body, its relocation sites and every named or local callee
match the retail bytes completely.

Sites correspond in two independent ways. When the parent's emitted body has
the retail length and exactly the retail absolute-relocation sites, each
site pairs with the same offset. Otherwise only `push OFFSET callback; call
_atexit` registrations pair, in emission order, and only when both sides have
the same number. Anything else leaves the referenced bytes unclaimed.
"""
from __future__ import annotations

from collections import defaultdict
import re
import struct

from homm3.core import msvc_names
from homm3.verify import source_padding
from homm3.verify.startup_bodies import Candidate, bindings, match

DIR32, REL32 = 6, 20
PUSH, CALL = 0x68, 0xe8


def _pushed_to_atexit(code, is_atexit):
    """[(operand offset, pushed value)] reaching `call _atexit` as its argument.

    The callback push may be separated from the call by instructions that do
    not touch the stack (VC6 schedules the guard store between them).
    """
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    found, pending = [], None
    for address, size, mnemonic, operands in disassembler.disasm_lite(code, 0):
        if mnemonic == 'push':
            pending = ((address + 1, struct.unpack_from('<I', code, address + 1)[0])
                       if code[address] == PUSH and size == 5 else None)
        elif mnemonic == 'call':
            if pending and code[address] == CALL and is_atexit(address):
                found.append(pending)
            pending = None
        elif mnemonic in ('pop', 'pushal', 'popal', 'ret', 'leave') or 'esp' in operands:
            pending = None
    return found


def registrations(body, relocations, atexit):
    """[(site, symbol)] of callbacks pushed for `call _atexit` in emitted code."""
    def is_atexit(address):
        call = relocations.get(address + 1)
        return call is not None and call[1] == REL32 and msvc_names.mask(call[0]) == atexit
    return [(site, relocations[site][0]) for site, _ in _pushed_to_atexit(body, is_atexit)
            if relocations.get(site, ('', 0))[1] == DIR32]


def retail_registrations(actual, rva, atexit_rva, image_base):
    """[(site offset, callback rva)] of retail callbacks passed to _atexit."""
    def is_atexit(address):
        return rva + address + 5 + struct.unpack_from('<i', actual, address + 1)[0] == atexit_rva
    return [(site, value - image_base) for site, value in _pushed_to_atexit(actual, is_atexit)]


def runtime_names():
    """{masked symbol: {rva}} from reviewed runtime placements, if present."""
    from homm3.core.paths import RETAIL
    from homm3.core.tsv import read
    path = RETAIL / 'runtime-contributions.tsv'
    names = defaultdict(set)
    if path.is_file():
        for row in read(path)[2]:
            symbol = row.get('symbol', '-')
            # Compiler-private ordinals (`$E24`) name nothing outside their
            # own archive member and would collide with emitted local code.
            if symbol != '-' and not re.fullmatch(r'_?\$[A-Z][0-9]+', symbol):
                names[msvc_names.mask(symbol)].add(int(row['rva'], 0))
    return names


def compare(project, pe, model, enrolled=(), objects=None):
    from homm3.delink.image import Image
    from homm3.retail_labels.censuses import functions
    image = Image(pe)
    sizes = {row['rva']: row['size'] for row in functions()}
    objects = objects or source_padding.Objects(project)
    targets = defaultdict(set)
    for b in model.functions + model.data:
        for entry in (b, *b.aliases):
            if entry.name and entry.channel:
                targets[msvc_names.mask(entry.name)].add(b.rva)
    claimed = {b.rva for b in model.functions if b.channel}
    atexit = msvc_names.mask('_atexit')
    atexit_rva = next(iter(targets.get(atexit, ())), None)
    result = dict(matches=[], dependencies=[], gaps=[])
    candidates = {}
    seen = {}
    runtime = runtime_names()
    for b in sorted(model.functions, key=lambda b: b.rva):
        if b.channel not in ('src', 'src_compgen', 'src_dyninit') or not b.unit:
            continue
        loaded = objects.get(b.unit)
        if loaded is None:
            continue
        found = source_padding.section_of(loaded, b.name)
        if not found:
            continue
        obj = loaded[0]
        number = found[0]
        raw = obj.section_payload(number) or b''
        relocations = obj.typed_relocations(number)
        actual = pe.read(b.rva, b.size)
        if actual is None:
            continue
        # Emitted functions only: each starts its own code section. Code
        # labels (jump-table targets) share their parent's section.
        local = {name for name, hits in loaded[1].items()
                 if not name.startswith(('$L', '.'))
                 and any(value == 0 and section > 0
                         and obj.section_table[section - 1]['characteristics']
                         & source_padding.CODE for value, section in hits)}
        pairs = []
        required = {site - b.rva for site in image.relocs_in(b.rva, b.rva + b.size)}
        emitted = {site for site, (_, kind) in relocations.items() if kind == DIR32}
        if (len(raw) >= b.size and raw[b.size:].strip(b'\x90') == b''
                and emitted == required):
            for site, (symbol, kind) in relocations.items():
                if kind == DIR32 and msvc_names.mask(symbol) in local:
                    addend = struct.unpack_from('<i', raw, site)[0]
                    pairs.append((symbol, struct.unpack_from('<I', actual, site)[0]
                                  - image.image_base - addend))
        elif atexit_rva is not None:
            mine = registrations(raw, relocations, atexit)
            theirs = retail_registrations(actual, b.rva, atexit_rva, image.image_base)
            if len(mine) == len(theirs):
                pairs = [(symbol, callback) for (_, symbol), (_, callback)
                         in zip(mine, theirs) if msvc_names.mask(symbol) in local]
        if not pairs:
            continue
        if b.unit not in candidates:
            candidate = Candidate(obj)
            # Same-unit names, including VC6's spelling of anonymous scopes.
            unit_targets = bindings(model, enrolled, b.unit, list(candidate.symbols))[0]
            for name, rvas in runtime.items():
                unit_targets.setdefault(name, set()).update(rvas)
            candidates[b.unit] = (candidate, unit_targets)
        candidate, unit_targets = candidates[b.unit]
        for symbol, callback in pairs:
            if callback in claimed or msvc_names.mask(symbol) in targets:
                continue
            verdict, children, reason = match(candidate, symbol, callback, image,
                                              sizes, unit_targets)
            row = dict(unit=b.unit, symbol=symbol, rva=callback,
                       size=sizes.get(callback, 0), parent=b.name, verdict=verdict,
                       reason=reason)
            previous = seen.get(callback)
            if previous is not None:
                continue
            seen[callback] = row
            if verdict != 'exact':
                result['gaps'].append(row)
                continue
            result['matches'].append(row)
            for child, address, size in children:
                if address not in claimed:
                    result['dependencies'].append(dict(unit=b.unit, symbol=child,
                        rva=address, size=size, root=callback, verdict='exact'))
    fresh = objects.fresh()
    result['matches'] = [r for r in result['matches'] if r['unit'] in fresh]
    roots = {r['rva'] for r in result['matches']}
    result['dependencies'] = [r for r in result['dependencies']
                              if r['unit'] in fresh and r['root'] in roots
                              and r['rva'] not in roots]
    return result
