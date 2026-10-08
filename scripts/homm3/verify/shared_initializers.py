"""Match CRT bodies emitted identically by many units for header statics.

Some header statics have no authored storage owner: for example VC6's
`std::ctype<unsigned short>::id` is initialized in every unit that includes
<xlocale>, under a compiler-private one-byte COMMON guard, with an empty
destructor registered through `_atexit`. The startup comparison needs a
source DATA anchor and therefore cannot select those roots.

Here a retail CRT root is credited only when a current raw object emits a
complete body that agrees byte for byte, including every relocation site.
Named code targets must resolve through the model or through an emitted local
body that itself matches completely. Compiler-private COMMON data can be bound
only when every matching copy agrees on one address, the retail storage is
zero-filled, and no model claim already owns that address. Identical copies
identify the header definition, never the retail unit that emitted them.
"""
from __future__ import annotations

from bisect import bisect_right
from collections import defaultdict
from hashlib import sha256
import struct

from homm3.core import compile_receipt, msvc_names
from homm3.core.tsv import read
from homm3.delink.coffx import Obj
from homm3.verify.source_initializers import verified_roots
from homm3.verify.startup_bodies import Candidate, match
from homm3.core.images import path as _image_path

DIR32, REL32 = 6, 20


def commons(obj):
    """Compiler-private data: {name: size}.

    COMMON symbols carry their size. A header's file-static object (such as
    <iostream>'s `_Ios_init`) is a static symbol in uninitialized storage;
    only its first byte is checked, since candidate placement proves no
    retail extent.
    """
    result, statics = {}, defaultdict(list)
    for index, value, section in obj.iter_symbols():
        storage = obj.buf[obj.symptr + index*18 + 16]
        name = obj.sym_name(index)
        if section == 0 and value and storage == 2:
            result[name] = value
        elif (section > 0 and storage == 3 and not name.startswith('.')
              and obj.section_table[section - 1]['characteristics'] & 0x80):
            statics[section].append((value, name))
    for rows in statics.values():
        for value, name in rows:
            # Candidate placement does not give a retail extent: bind the
            # address only and leave the object's size to its DATA owner.
            result[name] = 1
            FILE_STATIC.add(name)
    return result


#: Names of file statics seen by `commons`; each belongs to one unit only.
FILE_STATIC = set()


def proposals(body, rva, actual, image, private, targets=None):
    """Private-data bindings implied by one retail copy, or None on conflict.

    Unrelocated bytes must agree, and a call to a name the model already
    places must reach that address.
    """
    if len(body.payload) != len(actual):
        return None
    for off, name, kind in body.relocations:
        known = (targets or {}).get(msvc_names.mask(name))
        if kind == REL32 and known:
            addend = struct.unpack_from('<i', body.payload, off)[0]
            destination = rva + off + 4 + struct.unpack_from('<i', actual, off)[0] - addend
            if destination not in known:
                return None
    relocated = set()
    for off, _, _ in body.relocations:
        relocated.update(range(off, off+4))
    if any(a != b for i, (a, b) in enumerate(zip(body.payload, actual))
           if i not in relocated):
        return None
    result = {}
    for off, name, kind in body.relocations:
        if kind == DIR32 and name in private:
            addend = struct.unpack_from('<i', body.payload, off)[0]
            target = struct.unpack_from('<I', actual, off)[0] - image.image_base - addend
            if result.setdefault(name, target) != target:
                return None
    return result


def compare(project, pe, model, excluded=(), library_names=None):
    """Return exact shared bodies, their dependencies and COMMON bindings."""
    from homm3.core.cc_wrap import scan_header_deps
    from homm3.delink.image import Image
    from homm3.retail_labels.censuses import functions
    result = dict(matches=[], dependencies=[], bindings=[], gaps=[])
    roots = verified_roots(pe, read(project.root / _image_path('config/retail/init-thunks.tsv'))[2])
    if not roots:
        result['gaps'].append('retail CRT table does not verify'); return result
    image = Image(pe)
    sizes = {row['rva']: row['size'] for row in functions()}
    claimed = {b.rva for b in model.functions if b.channel}
    open_roots = [rva for rva in roots if rva not in excluded and rva not in claimed
                  and rva in sizes]
    targets = defaultdict(set)
    for b in model.functions + model.data:
        for entry in (b, *b.aliases):
            if entry.name and entry.channel:
                targets[msvc_names.mask(entry.name)].add(b.rva)
    from homm3.verify.local_cleanups import runtime_names
    for name, rvas in runtime_names(library_names).items():
        targets[name].update(rvas)
    owned = [(b.rva, b.rva + max(b.size, 1)) for b in model.data if b.channel]
    compiler = [p for p in (project.toolchain / 'bin').iterdir()
                if p.is_file() and p.suffix.lower() in ('.exe', '.dll')]
    hashes, receipts, witnesses = {}, {}, []
    for unit in project.manifest['unit']:
        stem, source = unit['unit'], project.root / unit['source']
        path = project.root / f'build/objdiff/base/{stem}.obj'
        if not path.is_file():
            continue
        obj = Obj(path)
        candidate = Candidate(obj)
        private = commons(obj)
        names = [n for n in candidate.roots()
                 if any(k == DIR32 and t in private
                        for _, t, k in candidate.body(n).relocations)]
        if not names:
            continue
        required = [source, project.root / _image_path('config/units.toml'),
                    project.root / 'config/project.toml', *compiler,
                    *scan_header_deps(source, project.toolchain / 'include', *project.includes)]
        flags = project.manifest['flags'][unit['flags']]
        inputs = compile_receipt.current(path, flags=flags, required=required, hashes=hashes)
        if inputs is None or compile_receipt.digest(path) != sha256(obj.buf).hexdigest():
            result['gaps'].append(f'{stem}: source object missing or stale'); continue
        receipts[stem] = (path, flags, inputs, sha256(obj.buf).hexdigest())
        witnesses.extend((stem, candidate, private, name) for name in names)

    # One pattern per distinct emitted body, with every unit that emits it.
    patterns = defaultdict(list)
    for stem, candidate, private, name in witnesses:
        body = candidate.body(name)
        patterns[(body.payload, body.relocations)].append((stem, candidate, private, name))

    # Identical bytes can carry different compiler-private names (e.g. the
    # num_get and num_put facet guards). Retail keeps each object's code
    # contiguous, so such a tie is broken only by the unit that owns the
    # nearest preceding claimed source function; otherwise it stays open.
    owners = sorted((b.rva, b.unit) for b in model.functions
                    if b.channel in ('src', 'src_compgen', 'src_dyninit') and b.unit)
    starts = [rva for rva, _ in owners]

    def enclosing(rva):
        index = bisect_right(starts, rva) - 1
        return owners[index][1] if index >= 0 else None

    proposed = defaultdict(set)
    hits = defaultdict(list)
    for rva in open_roots:
        actual = pe.read(rva, sizes[rva])
        found = []
        unit = enclosing(rva)
        for emitters in patterns.values():
            stem, candidate, private, name = emitters[0]
            binding = proposals(candidate.body(name), rva, actual, image, private, targets)
            if binding is None:
                continue
            # A file static identifies storage of its own unit only.
            if any(symbol in FILE_STATIC for symbol in binding):
                emitters = tuple(e for e in emitters if e[0] == unit)
                if not emitters:
                    continue
            found.append((emitters, binding))
        if len({tuple(sorted(b)) for _, b in found}) > 1:
            found = [(tuple(e for e in emitters if e[0] == unit), b)
                     for emitters, b in found]
            found = [(emitters, b) for emitters, b in found if emitters]
            if len({tuple(sorted(b)) for _, b in found}) != 1:
                result['gaps'].append(f'{rva:#x}: identical bodies name different '
                                      'private data and no enclosing unit decides')
                continue
        for emitters, binding in found:
            stem, candidate, private, name = emitters[0]
            hits[rva].append((stem, candidate, private, name, binding))
            for symbol, target in binding.items():
                proposed[symbol].add(target)

    bound = {}
    claimants = defaultdict(set)
    for symbol, addresses in proposed.items():
        for address in addresses:
            claimants[address].add(symbol)
    for symbol, addresses in sorted(proposed.items()):
        if len(addresses) != 1:
            result['gaps'].append(f'{symbol}: copies disagree on its address'); continue
        address = next(iter(addresses))
        if len(claimants[address]) != 1:
            result['gaps'].append(f'{symbol}: {address:#x} is also proposed for '
                                  + ', '.join(sorted(claimants[address] - {symbol})))
            continue
        size = max(private.get(symbol, 0) for emitters in patterns.values()
                   for _, _, private, _ in emitters)
        if any(lo < address + size and address < hi for lo, hi in owned):
            result['gaps'].append(f'{symbol}: {address:#x} already has a model owner'); continue
        if (image.sec_name(address) != '.data' or image.off(address) is not None
                or image.payload(address, size) != bytes(size)):
            result['gaps'].append(f'{symbol}: {address:#x} is not zero-filled storage'); continue
        bound[symbol] = (address, size)
    for symbol, (address, size) in sorted(bound.items(), key=lambda x: x[1]):
        result['bindings'].append(dict(symbol=symbol, rva=address, size=size))

    for rva in open_roots:
        exact = []
        for stem, candidate, private, name, binding in hits.get(rva, ()):
            if any(symbol not in bound for symbol in binding):
                continue
            local = defaultdict(set, targets)
            for symbol, (address, _) in bound.items():
                local[msvc_names.mask(symbol)].add(address)
            verdict, children, reason = match(candidate, name, rva, image, sizes, local)
            if verdict == 'exact':
                exact.append((stem, name, children))
        if len(exact) != 1:
            if exact:
                result['gaps'].append(f'ambiguous shared initializer at {rva:#x}')
            continue
        stem, name, children = exact[0]
        result['matches'].append(dict(unit=stem, symbol=name, rva=rva, size=sizes[rva],
                                      source=f'{stem} (identical header copy)'))
        for child, address, size in children:
            result['dependencies'].append(dict(unit=stem, symbol=child, rva=address,
                                               size=size, root=rva, verdict='exact'))

    fresh, final = set(), {}
    for stem, (path, flags, inputs, digest) in receipts.items():
        if (compile_receipt.current(path, flags=flags, required=inputs, hashes=final) == inputs
                and compile_receipt.digest(path) == digest):
            fresh.add(stem)
        else:
            result['gaps'].append(f'{stem}: source inputs changed during comparison')
    result['matches'] = [r for r in result['matches'] if r['unit'] in fresh]
    kept = {r['rva'] for r in result['matches']}
    result['dependencies'] = [r for r in result['dependencies']
                              if r['unit'] in fresh and r['root'] in kept]
    return result
