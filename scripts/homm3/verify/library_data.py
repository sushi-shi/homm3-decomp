"""Place and compare pinned VC6 runtime-library data against retail.

The retail image has no symbols, but every runtime function named in
``config/retail/runtime-map.tsv`` came from a member of the pinned LIBCMT or
LIBCPMT archive. A witness function first has to reproduce its retail body
(relocation words masked). Each DIR32 relocation in that body then states
where one archive datum lives: the retail word minus the COFF addend is the
symbol's address, and the symbol's section offset gives the base of its whole
section contribution. Placed data sections extend the placement through
their own DIR32 relocations.

A placement is kept only when every derivation agrees. The entire section is
then compared: raw bytes outside relocation words must equal retail, and
each relocation word must equal its referent's known address plus addend.
Unknown referents leave the comparison ``unresolved``; nothing is masked into
an exact verdict. Uninitialized sections and COMMON allocations are sized by
the archive object and must read as zero in retail.

This is library ownership plus byte comparison. It adds no function-score
rows and never edits vendor or archive content.
"""
from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field
import struct

from homm3.compare.canonicalize import CoffObject
from homm3.core.paths import RETAIL, msvc_dir
from homm3.core.tsv import read as read_tsv

LIBRARIES = ('LIBCMT', 'LIBCPMT')
DIR32, REL32 = 6, 20
MEM_EXECUTE = 0x20000000
LNK_COMDAT = 0x00001000
UNINITIALIZED = 0x00000080
INITIALIZED = 0x00000040


@dataclass
class Placement:
    member: str
    section: int
    rva: int
    size: int
    name: str
    storage: str
    symbols: tuple
    witnesses: set = field(default_factory=set)
    verdict: str = ''
    comdat: str = ''
    alignment: int = 0
    different: int = 0
    unresolved: int = 0
    reason: str = ''
    unknown_targets: tuple = ()


def _members(library):
    from homm3.verify.library_data_refs import _named_ar_members
    path = msvc_dir() / 'lib' / f'{library}.LIB'
    out = []
    for name, body in _named_ar_members(path):
        try:
            out.append((f'{library}:{name.rstrip("/")}', CoffObject(body)))
        except (ValueError, struct.error):
            continue
    return out


def _storage(section):
    flags = section.characteristics
    if flags & MEM_EXECUTE:
        return 'text'
    if flags & UNINITIALIZED:
        return 'bss'
    if flags & INITIALIZED:
        return 'data' if flags & 0x80000000 else 'rdata'
    return None


def analyse(pe, model=None, *, libraries=LIBRARIES):
    from homm3.retail_labels.censuses import functions
    extents = {r['rva']: r['size'] for r in functions()}
    runtime = {}
    for row in read_tsv(RETAIL / 'runtime-map.tsv')[2]:
        runtime.setdefault(row['name'], set()).add(int(row['rva'], 16))
    objects = dict(m for library in libraries for m in _members(library))
    # External definitions by name, per kind.
    code, data, common = defaultdict(list), defaultdict(list), defaultdict(int)
    for key, obj in objects.items():
        for sym in obj.symbols.values():
            if sym.storage_class != 2:
                continue
            if sym.section == 0 and sym.value:
                common[sym.name] = max(common[sym.name], sym.value)
            elif sym.section > 0:
                kind = _storage(obj.sections[sym.section - 1])
                (code if kind == 'text' else data)[sym.name].append((key, sym))

    def comdat_shape(key, sym):
        obj = objects[key]
        section = obj.sections[sym.section - 1]
        if not section.characteristics & LNK_COMDAT:
            return None
        relocations = tuple(sorted((r.site, r.typ, obj.symbols[r.symbol_index].name)
                                   for r in obj.relocations if r.section == sym.section))
        return sym.value, obj.section_bytes(section), relocations

    def definition(name):
        """One archive definition, folding identical COMDAT copies."""
        defs = data.get(name, [])
        if len(defs) == 1:
            return defs[0]
        shapes = {comdat_shape(key, sym) for key, sym in defs}
        if defs and None not in shapes and len(shapes) == 1:
            return min(defs, key=lambda d: d[0])
        return None

    def word(rva):
        raw = pe.read(rva, 4)
        return None if raw is None else struct.unpack('<I', raw)[0]

    placements: dict[tuple, dict[int, set]] = defaultdict(lambda: defaultdict(set))
    commons: dict[str, dict[int, set]] = defaultdict(lambda: defaultdict(set))
    witnesses = []

    def place_symbol(key, obj, sym, address, why):
        """Record where ``sym`` (as seen from member ``key``) lives."""
        if sym.section > 0:
            if _storage(obj.sections[sym.section - 1]) in ('data', 'rdata', 'bss'):
                placements[(key, sym.section)][address - sym.value].add(why)
            return
        found = definition(sym.name)
        if found:
            other_key, other = found
            placements[(other_key, other.section)][address - other.value].add(why)
        elif not data.get(sym.name) and sym.name in common:
            commons[sym.name][address].add(why)

    def body_matches(key, sym, rva, extent=None):
        """The member's code for ``sym`` reproduces retail at ``rva``."""
        obj = objects[key]
        section = obj.sections[sym.section - 1]
        if extent is None:
            following = [s.value for s in obj.symbols.values()
                         if s.section == sym.section and s.value > sym.value
                         and s.storage_class in (2, 3) and not s.aux_count]
            extent = min(following, default=section.raw_size) - sym.value
        if extent <= 0 or sym.value + extent > section.raw_size:
            return False
        body = pe.read(rva, extent)
        if body is None:
            return False
        raw = obj.section_bytes(section)[sym.value:sym.value + extent]
        masked = set()
        for rel in obj.relocations:
            if rel.section != sym.section or not sym.value <= rel.site < sym.value + extent:
                continue
            if rel.typ not in (DIR32, REL32) or rel.site + 4 > sym.value + extent:
                return False
            masked.update(range(rel.site - sym.value, rel.site - sym.value + 4))
        return all(a == b for i, (a, b) in enumerate(zip(raw, body)) if i not in masked)

    def code_definitions(key, obj, sym):
        """Candidate (member, symbol) code a relocation names.

        Identical COMDAT copies from several members are all candidates;
        the caller still requires one of them to reproduce retail.
        """
        if sym.section > 0:
            return [(key, sym)]
        if sym.storage_class == 105 and sym.aux_count and not code.get(sym.name):
            # Weak external: its auxiliary record names the default symbol.
            default = struct.unpack_from('<I', obj.data, sym.offset + 18)[0]
            target = obj.symbols.get(default)
            return code_definitions(key, obj, target) if target is not None else []
        return list(code.get(sym.name, []))

    # 1. witnesses: archive function bodies that reproduce retail.
    for name, rvas in sorted(runtime.items()):
        for rva in rvas:
            extent = extents.get(rva, 0)
            body = pe.read(rva, extent) if extent else None
            if not body:
                continue
            hits = []
            for key, sym in code.get(name, ()):
                obj = objects[key]
                section = obj.sections[sym.section - 1]
                if sym.value + extent > section.raw_size:
                    continue
                raw = obj.section_bytes(section)[sym.value:sym.value + extent]
                masked, ok = set(), True
                for rel in obj.relocations:
                    if rel.section != sym.section or not sym.value <= rel.site < sym.value + extent:
                        continue
                    if rel.typ not in (DIR32, REL32) or rel.site + 4 > sym.value + extent:
                        ok = False
                        break
                    masked.update(range(rel.site - sym.value, rel.site - sym.value + 4))
                if ok and all(a == b for i, (a, b) in enumerate(zip(raw, body))
                              if i not in masked):
                    hits.append((key, sym))
            if len(hits) != 1:
                continue
            key, sym = hits[0]
            obj = objects[key]
            witnesses.append((rva, name, key))
            payload = obj.section_bytes(obj.sections[sym.section - 1])
            for rel in obj.relocations:
                if (rel.section != sym.section or rel.typ != DIR32
                        or not sym.value <= rel.site < sym.value + extent):
                    continue
                addend = struct.unpack_from('<i', payload, rel.site)[0]
                value = word(rva + rel.site - sym.value)
                if value is None:
                    continue
                target = obj.symbols[rel.symbol_index]
                place_symbol(key, obj, target, (value - addend - pe.image_base) & 0xffffffff,
                             f'{name}+0x{rel.site - sym.value:x}')

    # 2. fixpoint through placed data sections' own pointers.
    done = set()
    while True:
        pending = [(k, s, b) for (k, s), bases in list(placements.items())
                   if len(bases) == 1 for b in bases if (k, s, b) not in done]
        if not pending:
            break
        for key, secnum, base in pending:
            done.add((key, secnum, base))
            obj = objects[key]
            section = obj.sections[secnum - 1]
            if _storage(section) == 'bss':
                continue
            payload = obj.section_bytes(section)
            for rel in obj.relocations:
                if rel.section != secnum or rel.typ != DIR32:
                    continue
                addend = struct.unpack_from('<i', payload, rel.site)[0]
                value = word(base + rel.site)
                if value is None:
                    continue
                target = obj.symbols[rel.symbol_index]
                place_symbol(key, obj, target, (value - addend - pe.image_base) & 0xffffffff,
                             f'{key}{section.name}+0x{rel.site:x}')

    # 3. compare every uniquely placed section.
    function_addresses = {}
    for rva, name, key in witnesses:
        function_addresses.setdefault(name, set()).add(rva)
    for name, rvas in runtime.items():
        function_addresses.setdefault(name, set()).update(rvas)
    # The linker keeps the first COMDAT copy; game objects precede the
    # archives, so an instantiation the game also emits lives at the model's
    # claimed address for that exact name.
    model_code = defaultdict(set)
    for b in (model.functions if model is not None else ()):
        for entry in (b, *b.aliases):
            if entry.name and entry.channel:
                model_code[entry.name].add(b.rva)
    for name, rvas in model_code.items():
        function_addresses.setdefault(name, set()).update(rvas)
    results = []
    located = {}
    #: code names whose archive body reproduces retail at a referenced address
    verified_code = defaultdict(set)
    for (key, secnum), bases in placements.items():
        if len(bases) == 1:
            located[(key, secnum)] = next(iter(bases))
    common_at = {name: next(iter(a)) for name, a in commons.items() if len(a) == 1}
    # A placed COMDAT copy locates its external symbol for every member.
    comdat_at = defaultdict(set)
    for (key, secnum), base in located.items():
        obj = objects[key]
        if obj.sections[secnum - 1].characteristics & LNK_COMDAT:
            for sym in obj.symbols.values():
                if sym.section == secnum and sym.storage_class == 2:
                    comdat_at[sym.name].add(base + sym.value)

    def address_of(key, obj, sym):
        if sym.section > 0:
            section = obj.sections[sym.section - 1]
            if _storage(section) == 'text':
                rvas = function_addresses.get(sym.name, set()) if sym.storage_class == 2 else set()
                return next(iter(rvas)) if len(rvas) == 1 else None
            base = located.get((key, sym.section))
            if base is None and len(comdat_at.get(sym.name, ())) == 1:
                return next(iter(comdat_at[sym.name]))
            return None if base is None else base + sym.value
        rvas = function_addresses.get(sym.name, set())
        if not rvas and sym.storage_class == 105 and sym.aux_count:
            default = obj.symbols.get(struct.unpack_from('<I', obj.data, sym.offset + 18)[0])
            if default is not None:
                rvas = function_addresses.get(default.name, set())
        if len(rvas) == 1:
            return next(iter(rvas))
        found = definition(sym.name)
        if found:
            other_key, other = found
            base = located.get((other_key, other.section))
            if base is None and len(comdat_at.get(sym.name, ())) == 1:
                return next(iter(comdat_at[sym.name]))
            return None if base is None else base + other.value
        if not data.get(sym.name) and sym.name in common_at:
            return common_at[sym.name]
        return None

    for (key, secnum), bases in sorted(placements.items(), key=lambda i: str(i[0])):
        obj = objects[key]
        section = obj.sections[secnum - 1]
        names = tuple(sorted(s.name for s in obj.symbols.values()
                             if s.section == secnum and s.storage_class in (2, 3)
                             and not s.name.startswith('.') and not s.aux_count))
        base = next(iter(bases)) if len(bases) == 1 else min(bases)
        item = Placement(key, secnum, base, section.raw_size, section.name,
                         _storage(section), names,
                         {w for b in bases.values() for w in b})
        align_code = (section.characteristics >> 20) & 0xF
        item.alignment = 1 << (align_code - 1) if align_code else 16
        if section.characteristics & LNK_COMDAT:
            # The COMDAT's own external symbol is its link identity; the
            # linker keeps one copy however many members emit it.
            external = sorted(s.name for s in obj.symbols.values()
                              if s.section == secnum and s.storage_class == 2)
            item.comdat = external[0] if len(external) == 1 else ''
        if len(bases) != 1:
            item.verdict, item.reason = 'withheld', 'conflicting placements'
            results.append(item)
            continue
        retail = pe.read(base, section.raw_size)
        if retail is None:
            item.verdict, item.reason = 'withheld', 'placement outside the image'
            results.append(item)
            continue
        payload = obj.section_bytes(section)
        relocated, wrong, unknown = set(), set(), set()
        for rel in obj.relocations:
            if rel.section != secnum:
                continue
            span = set(range(rel.site, rel.site + 4))
            relocated |= span
            if rel.typ != DIR32:
                unknown |= span
                continue
            addend = struct.unpack_from('<i', payload, rel.site)[0]
            referent = obj.symbols[rel.symbol_index]
            target = address_of(key, obj, referent)
            if target is None:
                # A code referent is resolved only when the member's own
                # body reproduces retail at the address this word names.
                claimed = (struct.unpack_from('<I', retail, rel.site)[0]
                           - addend - pe.image_base) & 0xffffffff
                if any(_storage(objects[k].sections[c.section - 1]) == 'text'
                       and body_matches(k, c, claimed)
                       for k, c in code_definitions(key, obj, referent)):
                    target = claimed
                    verified_code[referent.name].add(claimed)
            if target is None:
                unknown |= span
                item.unknown_targets += (referent.name,)
                continue
            if struct.unpack_from('<I', retail, rel.site)[0] != (target + addend + pe.image_base) & 0xffffffff:
                wrong |= span
        wrong |= {i for i, (a, b) in enumerate(zip(payload, retail))
                  if i not in relocated and a != b}
        item.different, item.unresolved = len(wrong), len(unknown)
        item.verdict = 'mismatch' if wrong else 'unresolved' if unknown else 'exact'
        results.append(item)
    for name, addresses in sorted(commons.items()):
        size = common[name]
        if len(addresses) != 1 or not size:
            results.append(Placement('COMMON', 0, min(addresses), size, name, 'bss', (name,),
                                     {w for a in addresses.values() for w in a},
                                     'withheld', reason='conflicting placements'))
            continue
        rva = next(iter(addresses))
        payload = pe.read(rva, size)
        wrong = sum(bool(b) for b in payload) if payload is not None else size
        results.append(Placement('COMMON', 0, rva, size, name, 'bss', (name,),
                                 set(next(iter(addresses.values()))),
                                 'mismatch' if wrong else 'exact', wrong))
    for rva, name, _key in witnesses:
        verified_code[name].add(rva)
    analyse.verified_code = {name: next(iter(rvas)) for name, rvas in verified_code.items()
                             if len(rvas) == 1}
    return results, witnesses


def ranges(results):
    from homm3.verify.byte_accounting import Range
    out = []
    for r in results:
        if r.verdict in ('exact', 'unresolved') and r.size:
            identity = (r.comdat or (f'{r.member}{r.name}' if r.member != 'COMMON'
                                     else r.name))
            out.append(Range(r.rva, r.rva + r.size, 'library-vendor', identity, 2))
    return out


def explain(pe, rva):
    """Diagnostic: unresolved relocation targets of the placement at ``rva``."""
    results, _ = analyse(pe)
    return [r for r in results if r.rva == rva]
