"""Compare source-emitted CRT bodies anchored by ordinary DATA declarations.

The declared objects identify possible retail roots; full raw COFF bytes and
named relocations decide the verdict. Unnamed local cleanup bodies must also
compare completely. No address is inferred for an unknown data reference.
"""
from collections import defaultdict
from dataclasses import dataclass
from hashlib import sha256
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

from homm3.core import compile_receipt, msvc_names
from homm3.core.tsv import read
from homm3.delink.coffx import Obj
from homm3.verify.source_initializers import verified_roots
from homm3.core.images import path as _image_path


@dataclass(frozen=True)
class Body:
    name: str
    payload: bytes
    relocations: tuple
    padding: bytes
    alignment: int


class Candidate:
    def __init__(self, obj):
        self.obj = obj
        self.symbols = defaultdict(list)
        self.cache = {}
        for i, value, section in obj.iter_symbols():
            if section > 0:
                self.symbols[obj.sym_name(i)].append((value, section))

    def body(self, name):
        if name in self.cache:
            return self.cache[name]
        self.cache[name] = None
        if len(self.symbols[name]) != 1:
            return None
        offset, sn = self.symbols[name][0]
        sec = self.obj.section_table[sn-1]
        if offset or not sec['characteristics'] & 0x20:
            return None
        raw = self.obj.section_payload(sn)
        payload = raw.rstrip(b'\x90\xcc')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_32).disasm(payload, 0))
        if (not ins or ins[-1].mnemonic not in ('ret', 'jmp')
                or sum(i.size for i in ins) != len(payload)):
            return None
        relocations = self.obj.typed_relocations(sn)
        covered = set()
        for off, (_, kind) in relocations.items():
            word = set(range(off, off+4))
            if (kind not in (6, 20) or off < 0 or off+4 > len(payload)
                    or covered & word):
                return None
            if kind == 20 and not any(i.mnemonic in ('call', 'jmp')
                    and i.size == 5 and i.address+1 == off for i in ins):
                return None
            covered |= word
        result = Body(name, payload, tuple(sorted((off, name, kind)
                      for off, (name, kind) in relocations.items())),
                      raw[len(payload):], sec['alignment'])
        self.cache[name] = result
        return result

    def roots(self):
        result = []
        for sec in self.obj.section_table:
            if sec['name'] != '.CRT$XCU':
                continue
            payload = self.obj.section_payload(sec['index'])
            for off, (name, kind) in self.obj.typed_relocations(sec['index']).items():
                if (kind == 6 and off % 4 == 0 and off+4 <= len(payload)
                        and payload[off:off+4] == bytes(4) and self.body(name)):
                    result.append(name)
        return result


def match(candidate, name, rva, image, sizes, targets, *, assigned=None, trail=()):
    """Check a whole body and any unpaired local code it references.

    Returns exact dependencies only after every edge and byte verifies. Known
    identities cannot be overridden by a same-shaped function at another VA.
    """
    assigned = {} if assigned is None else assigned
    body = candidate.body(name)
    if not body or rva not in sizes:
        return 'unresolved', [], 'candidate body or reviewed retail extent absent'
    if name in trail or len(trail) >= 32:
        return 'unresolved', [], 'recursive unpaired code graph'
    if name in assigned and assigned[name] != rva:
        return 'mismatch', [], 'one emitted symbol refers to different retail bodies'
    assigned[name] = rva
    if len(body.payload) != sizes[rva]:
        return 'mismatch', [], 'body size differs'
    actual = image.pe.read(rva, sizes[rva])
    if actual is None or len(actual) != sizes[rva]:
        return 'unresolved', [], 'retail body unavailable'
    required = {rva+off for off, _, kind in body.relocations if kind == 6}
    if required != set(image.relocs_in(rva, rva+sizes[rva])):
        return 'mismatch', [], 'relocation sites differ'
    expected = bytearray(body.payload)
    dependencies, unresolved = [], []
    for off, target, kind in body.relocations:
        addend = struct.unpack_from('<i', body.payload, off)[0]
        destination = ((struct.unpack_from('<I', actual, off)[0]-image.image_base-addend)
                       if kind == 6 else
                       rva+off+4+struct.unpack_from('<i', actual, off)[0]-addend)
        known = targets.get(msvc_names.mask(target), set())
        if known:
            if destination not in known:
                return 'mismatch', [], f'named target differs: {target}'
        elif candidate.body(target):
            verdict, children, reason = match(candidate, target, destination,
                image, sizes, targets, assigned=assigned, trail=trail+(name,))
            if verdict == 'mismatch':
                return verdict, [], f'{target}: {reason}'
            if verdict != 'exact':
                unresolved.append(f'{target}: {reason}')
            else:
                dependencies.append((target, destination, sizes[destination]))
                dependencies.extend(children)
        else:
            unresolved.append(f'unpaired reference: {target}')
        # Copied fields are NOT ignored: the identity above must verify, or
        # the entire comparison is explicitly unresolved/mismatching.
        expected[off:off+4] = actual[off:off+4]
    if bytes(expected) != actual:
        return 'mismatch', [], 'instruction bytes differ'
    if unresolved:
        return 'unresolved', [], '; '.join(sorted(set(unresolved)))
    return 'exact', sorted(set(dependencies)), ''


def bindings(model, enrolled, unit, emitted):
    """Use the existing model and same-TU names, including anonymous data."""
    from homm3.retail_labels.source import vc6_function_name
    names, local, owners = defaultdict(set), defaultdict(set), {}
    for b in model.functions + model.data:
        for entry in (b, *b.aliases):
            if entry.name and entry.channel:
                names[msvc_names.mask(entry.name)].add(b.rva)
                if entry.unit == unit:
                    local[msvc_names.mask(entry.name)].add(b.rva)
    for b in model.data:
        if b.channel == 'src' and b.size and b.unit == unit:
            owners[msvc_names.mask(b.name)] = b
    for row in enrolled:
        if 'gap' in row.get('provenance', ''):
            continue
        name, rva = msvc_names.mask(row['name']), int(row['rva'], 0)
        names[name].add(rva)
        if row['object'].removesuffix('.c') == unit:
            local[name].add(rva)
    names.update(local)
    for spelling in list(local):
        if '@?A0x' not in spelling:
            continue
        actual = vc6_function_name(spelling, emitted, unit)
        if actual:
            key = msvc_names.mask(actual)
            names[key].update(local[spelling])
            if spelling in owners:
                owners[key] = owners[spelling]
    return names, owners


def source_anchors(body, owners):
    found = {owners[msvc_names.mask(name)].rva: owners[msvc_names.mask(name)]
             for _, name, kind in body.relocations
             if kind == 6 and msvc_names.mask(name) in owners}
    return [found[rva] for rva in sorted(found)]


def paired_roots(owners, roots, references):
    # At least one reference into every declared owner must be present. This
    # only selects candidates: all addends/targets/bytes still have to match.
    return [rva for rva in roots if all(any(b.rva <= target < b.rva+b.size
                for target in references[rva]) for b in owners)]


def compare(project, pe, model, enrolled):
    from homm3.core.cc_wrap import scan_header_deps
    from homm3.delink.image import Image
    from homm3.retail_labels.censuses import functions
    result = dict(matches=[], comparisons=[], dependencies=[], gaps=[])
    roots = verified_roots(pe, read(project.root / _image_path('config/retail/init-thunks.tsv'))[2])
    if not roots:
        result['gaps'].append('retail CRT table does not verify'); return result
    image = Image(pe)
    sizes = {row['rva']: row['size'] for row in functions()}
    references = {rva:{image.u32(site)-image.image_base
                  for site in image.relocs_in(rva, rva+sizes.get(rva, 0))} for rva in roots}
    hashes, pending, inputs_by_unit = {}, [], {}
    compiler_files = [p for p in (project.toolchain / 'bin').iterdir()
                      if p.is_file() and p.suffix.lower() in ('.exe', '.dll')]
    for unit in project.manifest['unit']:
        stem, source = unit['unit'], project.root / unit['source']
        path = project.root / f'build/objdiff/base/{stem}.obj'
        if not path.is_file():
            continue
        obj = Obj(path)
        candidate = Candidate(obj)
        emitted = list(candidate.symbols)
        targets, owners = bindings(model, enrolled, stem, emitted)
        selected = [(name, source_anchors(candidate.body(name), owners))
                    for name in candidate.roots()]
        selected = [(name, anchors) for name, anchors in selected if anchors]
        if not selected:
            continue
        required = [source, project.root / _image_path('config/units.toml'),
                    project.root / 'config/project.toml', *compiler_files,
                    *scan_header_deps(source, project.toolchain / 'include', *project.includes)]
        inputs = compile_receipt.current(path, flags=project.manifest['flags'][unit['flags']],
                                         required=required, hashes=hashes)
        if inputs is None or compile_receipt.digest(path) != sha256(obj.buf).hexdigest():
            result['gaps'].append(f'{stem}: source object missing or stale'); continue
        inputs_by_unit[stem] = (path, unit['flags'], inputs, sha256(obj.buf).hexdigest())
        for name, anchors in selected:
            options = paired_roots(anchors, roots, references)
            hits, attempts = [], []
            for rva in options:
                verdict, children, reason = match(candidate, name, rva, image, sizes, targets)
                attempts.append(dict(rva=rva, verdict=verdict, reason=reason))
                if verdict == 'exact':
                    hits.append((rva, children))
            summary = dict(unit=stem, symbol=name, source=unit['source'],
                owners=[dict(name=b.name, rva=b.rva, size=b.size) for b in anchors],
                verdict='exact' if len(hits) == 1 else
                        attempts[0]['verdict'] if len(attempts) == 1 else 'unavailable',
                candidates=attempts)
            result['comparisons'].append(summary)
            if len(hits) != 1:
                continue
            rva, children = hits[0]
            pending.append(dict(rva=rva, size=sizes[rva], **summary))
            for child, address, size in children:
                result['dependencies'].append(dict(unit=stem, symbol=child,
                    rva=address, size=size, root=rva, verdict='exact'))
    # Recheck receipt content at the end, including headers shared across TUs.
    fresh, final_hashes = set(), {}
    for stem, (path, flags, inputs, digest) in inputs_by_unit.items():
        if (compile_receipt.current(path, flags=project.manifest['flags'][flags],
                                   required=inputs, hashes=final_hashes) == inputs
                and compile_receipt.digest(path) == digest):
            fresh.add(stem)
        else:
            result['gaps'].append(f'{stem}: source inputs changed during comparison')
    for row in result['comparisons']:
        if row['unit'] not in fresh:
            row['verdict'] = 'unavailable'
            row['candidates'] = []
    by_address = defaultdict(list)
    for row in pending:
        if row['unit'] in fresh:
            by_address[row['rva']].append(row)
    for rva, rows in sorted(by_address.items()):
        if len(rows) != 1:
            result['gaps'].append(f'ambiguous emitted initializer at {rva:#x}')
        else:
            result['matches'].extend(rows)
    admitted = {(r['unit'], r['rva']) for r in result['matches']}
    result['dependencies'] = [r for r in result['dependencies']
                             if (r['unit'], r['root']) in admitted]
    return result
