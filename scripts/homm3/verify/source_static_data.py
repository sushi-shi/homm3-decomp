"""Identify SDK tree statics through complete, source-claimed function bodies.

The ordinary map/set instantiation emits its own named four-byte COMDATs.
An independently claimed function can identify their retail addresses only
when its entire code, relocation sites, addends and other named references
agree. These bindings are regenerated from fresh objects, never hand ledgers.
"""
from collections import defaultdict
from hashlib import sha256
import struct

from homm3.core import compile_receipt, msvc_names
from homm3.delink import coffx
from homm3.verify.startup_bodies import Candidate, bindings, match


def definitions(obj):
    """Only complete zero-initialized SDK tree COMDAT definitions qualify."""
    result = set()
    for sec in obj.section_table:
        members = obj.defined_symbols(sec['index'])
        if (sec['name'] != '.bss' or sec['size'] != 4 or sec['comdat'] != 2
                or sec['characteristics'] & 0x200010a0 != 0x1080
                or sec['alignment'] != 4 or len(members) != 1
                or members[0][0] != 0 or obj.typed_relocations(sec['index'])
                or obj.section_payload(sec['index']) not in (b'', bytes(4))):
            continue
        name = members[0][1]
        if name.startswith(('?_Nil@?$_Tree@', '?_Nilrefs@?$_Tree@')):
            result.add(name)
    return result


def infer(model, candidates, image):
    """Propose a binding only after a whole named source function verifies."""
    sizes = {b.rva: b.size for b in model.functions}
    occupied = [b for b in model.functions + model.data if b.channel and b.size]
    proposals = defaultdict(set)
    complete = []
    regions = image.pe.data_regions()
    for unit, (candidate, eligible) in sorted(candidates.items()):
        targets, _ = bindings(model, [], unit, list(candidate.symbols))
        for binding in model.functions:
            if binding.channel not in ('src', 'src_compgen') or binding.unit != unit:
                continue
            body = candidate.body(binding.name)
            if body is None or len(body.payload) != binding.size:
                continue
            raw = image.pe.read(binding.rva, binding.size)
            if raw is None or len(raw) != binding.size:
                continue
            pending = defaultdict(set)
            unsupported_addend = False
            for offset, name, kind in body.relocations:
                if kind != 6 or name not in eligible or targets.get(msvc_names.mask(name)):
                    continue
                addend = struct.unpack_from('<i', body.payload, offset)[0]
                if addend:
                    unsupported_addend = True
                address = struct.unpack_from('<I', raw, offset)[0] - image.image_base - addend
                pending[msvc_names.mask(name)].add(address)
            if unsupported_addend or not pending or any(len(v) != 1 for v in pending.values()):
                continue
            pending = {n: next(iter(v)) for n, v in pending.items()}
            if len(set(pending.values())) != len(pending):
                continue
            if any(address % 4 or not any(lo <= address and address+4 <= hi
                    for space, (lo, hi) in regions.items()
                    if space in ('data', 'bss'))
                    or image.payload(address, 4) != bytes(4)
                    or list(image.relocs_in(address, address+4))
                    or any(b.rva < address+4 and address < b.rva+b.size for b in occupied)
                    for address in pending.values()):
                continue
            trial = dict(targets)
            trial.update({msvc_names.mask(n): {a} for n, a in pending.items()})
            if match(candidate, binding.name, binding.rva, image, sizes, trial)[0] != 'exact':
                continue
            for name, address in pending.items():
                proposals[name].add(address)
            complete.append((unit, binding.rva, pending))
    # A conflicting witness invalidates the identity; never pick one address.
    invalid = {n for n, v in proposals.items() if len(v) != 1}
    by_address = defaultdict(set)
    for name, addresses in proposals.items():
        for address in addresses:
            by_address[address].add(name)
    invalid.update(n for names in by_address.values() if len(names) != 1 for n in names)
    witnesses = defaultdict(set)
    for unit, rva, pending in complete:
        # A conflicting target invalidates that entire body's proof, including
        # its other apparently unique data references.
        if invalid.intersection(pending):
            continue
        for name, address in pending.items():
            witnesses[(name, address)].add((unit, rva))
    return [(name, address, tuple(sorted(proofs)))
            for (name, address), proofs in sorted(witnesses.items())]


def recover(model, project, base_dir):
    """Use source/header/compiler receipts and recheck every contributing input."""
    from homm3.core.cc_wrap import scan_header_deps
    from homm3.delink.image import retail
    from homm3.model import Binding

    if not base_dir.is_dir() or not (project.toolchain / 'bin').is_dir():
        return []
    units = {u['unit']: u for u in project.manifest['unit']}
    candidates, witnesses, hashes, headers = {}, {}, {}, {}
    compiler = [p for p in (project.toolchain / 'bin').iterdir()
                if p.is_file() and p.suffix.lower() in ('.exe', '.dll')]
    for unit, obj in coffx.objects(base_dir):
        eligible = definitions(obj)
        row = units.get(unit)
        if not eligible or row is None:
            continue
        path = base_dir / f'{unit}.obj'
        source = project.root / row['source']
        required = [source, project.root / 'config/units.toml',
                    project.root / 'config/project.toml', *compiler,
                    *scan_header_deps(source, project.toolchain / 'include',
                                     *project.includes, cache=headers)]
        expected = compile_receipt.current(path,
            flags=project.manifest['flags'][row['flags']], required=required, hashes=hashes)
        if expected is None or compile_receipt.digest(path) != sha256(obj.buf).hexdigest():
            continue
        candidates[unit] = (Candidate(obj), eligible)
        witnesses[unit] = (path, expected, sha256(obj.buf).hexdigest())
    proved = infer(model, candidates, retail())
    if not proved:
        return []
    # Recheck unsuccessful witnesses too: a changed body could introduce a
    # conflicting address and invalidate the uniqueness established above.
    contributing = set(witnesses)
    inputs = {}
    for unit in contributing:
        for path, digest in witnesses[unit][1].items():
            if path in inputs and inputs[path] != digest:
                return []
            inputs[path] = digest
    try:
        if compile_receipt.snapshot(inputs) != inputs or any(
                compile_receipt.digest(witnesses[u][0]) != witnesses[u][2]
                for u in contributing):
            return []
    except OSError:
        return []
    regions = retail().pe.data_regions()
    return [Binding(address, 4, '', next(space for space, (lo, hi) in regions.items()
                    if lo <= address and address+4 <= hi), name, proofs[0][0],
                    'data_compgen', (), tuple(sorted({u for u, _ in proofs})))
            for name, address, proofs in proved]
