"""Compare VC6 exception-frame records (`.xdata$x`) with retail.

The data manifest enrolls each claimed function's retail FuncInfo and unwind
map under compiler-private names (`__ehfuncinfo$<owner>`,
`__ehunwindmap$<owner>`). The candidate object labels the same records with
a counter symbol (`$T<n>`), reachable only through the function: its
associative `.text$x` COMDAT holds the unwind funclets and the registration
stub `mov eax, $T; jmp ___CxxFrameHandler`.

Every byte outside relocation words must equal retail. A relocation into
the record itself (the unwind or try-block map pointer) must name the same
offset in retail. An unwind action names a funclet of the stub's `.text$x`
section: the candidate funclets, in section order, must correspond to the
retail funclets the reviewed census attributes to this parent, in address
order, with the same label always naming the same retail funclet. Any other
referent (catch handlers, type descriptors) stays unresolved unless the
model already knows it. Nothing is masked into an exact verdict.
"""
from __future__ import annotations

from collections import defaultdict
import struct

from homm3.core import msvc_names

FUNCINFO_SIZE = 32
DIR32 = 6


def _funclets(retail_dir):
    from homm3.core.tsv import read
    out = defaultdict(list)
    for row in read(retail_dir / 'funclets.tsv')[2]:
        out[int(row['parent_rva'], 0)].append(int(row['rva'], 0))
    return {parent: sorted(rvas) for parent, rvas in out.items()}


def compare(model, enrolled, pe, names, base_dir):
    """Results (compare_initializers' shape) for the enrolled EH rows."""
    from homm3.core.paths import RETAIL
    from homm3.delink.coffx import Obj
    from homm3.delink.image import Image
    image = Image(pe)
    funclets = _funclets(RETAIL)
    functions = {}
    for b in model.functions:
        if b.name and b.channel and b.unit:
            functions[(b.unit, msvc_names.mask(b.name))] = b.rva
    pairs = defaultdict(dict)
    for r in enrolled:
        name = r['name']
        for prefix, kind in (('__ehfuncinfo$', 'info'), ('__ehunwindmap$', 'map')):
            if name.startswith(prefix):
                unit = r['object'].removesuffix('.c')
                pairs[(unit, name[len(prefix):])][kind] = r
    objects, results = {}, []
    exact = _exact_functions()
    for (unit, owner), rows in sorted(pairs.items()):
        def emit(verdict, reason='', different=0, unresolved=0, per=None):
            for kind, r in sorted(rows.items()):
                d, u = (per or {}).get(kind, (different, unresolved))
                v = verdict if per is None else (
                    'mismatch' if d else 'unresolved' if u else 'exact')
                results.append(dict(unit=unit, name=r['name'], rva=int(r['rva'], 0),
                                    size=int(r['size'], 0), verdict=v, different=d,
                                    unresolved=u, reason=reason))
        if 'info' not in rows:
            emit('unavailable', 'FuncInfo row not enrolled')
            continue
        path = base_dir / f'{unit}.obj'
        if not path.is_file():
            emit('unavailable', 'candidate object missing')
            continue
        if unit not in objects:
            obj = Obj(path)
            table = defaultdict(list)
            for idx, value, section in obj.iter_symbols():
                table[obj.sym_name(idx)].append((section, value))
            masked = defaultdict(set)
            for name, entries in table.items():
                masked[msvc_names.mask(name)].update(sec for sec, _v in entries if sec > 0)
            table['__masked__'] = masked
            objects[unit] = (obj, table)
        obj, table = objects[unit]
        found = _candidate_record(obj, table, owner)
        if found is None:
            emit('unavailable', 'candidate FuncInfo not found through the function')
            continue
        xsec, offset, textx = found
        start = int(rows['info']['rva'], 0)
        size = FUNCINFO_SIZE + sum(int(r['size'], 0) for k, r in rows.items() if k == 'map')
        payload = obj.section_payload(xsec)[offset:offset + size]
        retail = pe.read(start, size)
        if retail is None or len(payload) != size:
            emit('unavailable', 'candidate record shorter than the retail extent')
            continue
        parent = functions.get((unit, msvc_names.mask(owner)))
        # Rank only the labels this record's actions name: other `.text$x`
        # labels (jump targets inside a funclet) are not funclet starts.
        offsets = dict((name, value) for name, rows in table.items()
                       if name.startswith('$L') for section, value in rows if section == textx)
        actions = sorted({target for site, (target, _typ) in obj.typed_relocations(xsec).items()
                          if offset <= site < offset + size and target in offsets},
                         key=offsets.get)
        label_rank = {name: rank for rank, name in enumerate(actions)}
        retail_funclets = funclets.get(parent, [])
        required = {site - start for site in image.relocs_in(start, start + size)}
        relocated, wrong, unknown, present = set(), set(), set(), set()
        mapping = {}
        for site, (target, typ) in obj.typed_relocations(xsec).items():
            if not offset <= site < offset + size:
                continue
            rel = site - offset
            present.add(rel)
            word = set(range(rel, min(rel + 4, size)))
            relocated |= word
            addend = struct.unpack_from('<i', payload, rel)[0]
            actual = struct.unpack_from('<I', retail, rel)[0] - pe.image_base
            symbol_offset = next((v for sec, v in table.get(target, ()) if sec == xsec), None)
            if typ != DIR32:
                unknown |= word
            elif symbol_offset is not None:
                if actual != start + symbol_offset - offset + addend:
                    wrong |= word
            elif target in label_rank:
                rank = label_rank[target]
                if rank >= len(retail_funclets) or \
                        mapping.setdefault(target, actual) != actual or \
                        retail_funclets[rank] != actual - addend:
                    wrong |= word
            else:
                known = names.get((unit, msvc_names.mask(target))) or \
                    names.get(('', msvc_names.mask(target)), set())
                if len(known) != 1:
                    unknown |= word
                elif actual != next(iter(known)) + addend:
                    wrong |= word
        for rel in required - present:
            wrong.update(range(rel, min(rel + 4, size)))
        for rel in present - required:
            wrong.update(range(rel, min(rel + 4, size)))
        wrong |= {i for i, (a, b) in enumerate(zip(payload, retail))
                  if i not in relocated and a != b}
        per = {'info': (len({i for i in wrong if i < FUNCINFO_SIZE}),
                        len({i for i in unknown if i < FUNCINFO_SIZE}))}
        if 'map' in rows:
            per['map'] = (len({i for i in wrong if i >= FUNCINFO_SIZE}),
                          len({i for i in unknown if i >= FUNCINFO_SIZE}))
        if any(d for d, _u in per.values()) and not exact(unit, owner):
            # The records are emitted from the function body; while that body
            # still differs from retail its unwind layout cannot be compared.
            emit('unavailable', 'owning function is not yet byte-exact',
                 per=None)
            continue
        emit('', per=per)
    return results


def _exact_functions():
    """Predicate over (unit, symbol): the current objdiff report scores 100%."""
    import json
    from homm3.core.paths import BUILD
    path = BUILD / 'objdiff/report.json'
    done = set()
    if path.is_file():
        for unit in json.loads(path.read_text()).get('units', []):
            stem = unit['name'].rsplit('/', 1)[-1]
            for function in unit.get('functions', []):
                if function.get('fuzzy_match_percent') == 100:
                    done.add((stem, msvc_names.mask(function['name'])))
    return lambda unit, owner: (unit, msvc_names.mask(owner)) in done


def _candidate_record(obj, table, owner):
    """(.xdata$x section, record offset, .text$x section) for ``owner``."""
    homes = table['__masked__'].get(msvc_names.mask(owner), set())
    if len(homes) != 1:
        return None
    home = next(iter(homes))
    for sec in obj.section_table:
        if sec['name'] != '.text$x' or sec['assoc'] != home:
            continue
        for site, (target, typ) in obj.typed_relocations(sec['index']).items():
            if typ != DIR32 or not target.startswith('$T'):
                continue
            for section, value in table.get(target, ()):
                if section > 0 and obj.section_table[section - 1]['name'] == '.xdata$x':
                    return section, value, sec['index']
    return None
