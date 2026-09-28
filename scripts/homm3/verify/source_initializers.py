"""Match CRT initializer bodies emitted by ordinary header definitions.

Each match binds the source object's store and every named call. No relocation
is merely masked. Identical header copies establish the declaration's identity,
not which retail translation unit emitted it. Only reviewed CRT roots are used.
"""
from collections import defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM

from homm3.core import compile_receipt
from homm3.core.tsv import read
from homm3.delink.coffx import Obj


@dataclass(frozen=True)
class Owner:
    source: str
    line: int
    name: str
    symbol: str
    size: int
    type: str


@dataclass(frozen=True)
class Pattern:
    owner: Owner
    body: bytes
    relocations: tuple
    padding: bytes = b''
    alignment: int = 1


def declarations(project, source, header):
    import clang.cindex as cx
    from homm3.core.compiler_profile import Profiles
    tu = cx.Index.create().parse(
        str(source), args=Profiles(project).for_source(source),
        options=cx.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    errors = [str(d) for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
    if errors:
        return {}, errors
    owners = {}
    for node in tu.cursor.get_children():
        if (node.kind == cx.CursorKind.VAR_DECL and node.location.file
                and Path(node.location.file.name).resolve() == header
                and node.storage_class == cx.StorageClass.STATIC
                and node.is_definition() and node.type.get_size() == 4):
            owner = Owner(header.relative_to(project.root).as_posix(),
                          node.location.line, node.spelling, node.mangled_name,
                          node.type.get_size(), node.type.spelling)
            owners[owner.symbol] = owner
    return owners, []


def candidate_patterns(obj, owners):
    symbols = defaultdict(list)
    for i, value, section in obj.iter_symbols():
        if section > 0:
            symbols[obj.sym_name(i)].append((value, section))
    dis = Cs(CS_ARCH_X86, CS_MODE_32)
    dis.detail = True
    result = set()
    for sec in obj.section_table:
        if sec['name'] != '.CRT$XCU':
            continue
        slots = obj.section_payload(sec['index'])
        for slot, (symbol, kind) in obj.typed_relocations(sec['index']).items():
            if (kind != 6 or slot % 4 or slot + 4 > len(slots)
                    or slots[slot:slot+4] != bytes(4) or len(symbols[symbol]) != 1):
                continue
            start, sn = symbols[symbol][0]
            if start or not obj.section_table[sn-1]['characteristics'] & 0x20:
                continue
            raw = obj.section_payload(sn)
            body = raw.rstrip(b'\x90\xcc')
            ins = list(dis.disasm(body, 0))
            if (not ins or ins[-1].mnemonic != 'ret'
                    or sum(i.size for i in ins) != len(body)):
                continue
            rel = obj.typed_relocations(sn)
            stores = [(off, name) for off, (name, k) in rel.items()
                      if name in owners and k == 6]
            if len(stores) != 1:
                continue
            store, name = stores[0]
            owner = owners[name]
            if len(symbols[name]) != 1:
                continue
            value, data_section = symbols[name][0]
            data = obj.section_table[data_section-1]
            if (not data['characteristics'] & 0x80 or value + owner.size > data['size']
                    or any(value < v < value+owner.size for hits in symbols.values()
                           for v, s in hits if s == data_section)):
                continue
            valid = True
            reloc_bytes = set()
            for off, (target, k) in rel.items():
                covered = set(range(off, off+4))
                if (off < 0 or off+4 > len(body) or covered & reloc_bytes
                        or body[off:off+4] != bytes(4)):
                    valid = False; break
                reloc_bytes |= covered
                if k == 20:
                    valid &= any(i.mnemonic == 'call' and i.size == 5
                                 and i.address + 1 == off for i in ins)
                elif k == 6 and off == store:
                    valid &= any(i.mnemonic == 'mov' and i.operands
                                 and i.operands[0].type == X86_OP_MEM
                                 and i.operands[0].size == owner.size
                                 and not i.operands[0].mem.base
                                 and not i.operands[0].mem.index
                                 and i.address+i.disp_offset == off
                                 and i.disp_size == 4 for i in ins)
                else:
                    valid = False
            if valid:
                result.add(Pattern(owner, body, tuple(sorted(
                    (off, name, k) for off, (name, k) in rel.items())),
                    raw[len(body):], obj.section_table[sn-1]['alignment']))
    return result


def match_body(pattern, rva, body, image, targets):
    """Return its bound destination only for a complete byte/relocation match."""
    if len(body) != len(pattern.body):
        return None
    required = {site-rva for site in image.relocs_in(rva, rva+len(body))}
    if required != {off for off, _, k in pattern.relocations if k == 6}:
        return None
    expected = bytearray(pattern.body)
    destination = None
    for off, name, kind in pattern.relocations:
        if kind == 20:
            actual = rva+off+4+struct.unpack_from('<i', body, off)[0]
            if actual not in targets.get(name, ()):
                return None
            struct.pack_into('<I', expected, off, (actual-rva-off-4) & 0xffffffff)
        elif kind == 6 and name == pattern.owner.symbol:
            destination = struct.unpack_from('<I', body, off)[0] - image.image_base
            if destination % pattern.owner.size or not any(
                    s['name'] == '.data' and s['va'] <= destination
                    and destination + pattern.owner.size <= s['va']+s['vsize']
                    for s in image.pe.sections):
                return None
            if image.payload(destination, pattern.owner.size) != bytes(pattern.owner.size):
                return None
            struct.pack_into('<I', expected, off, destination + image.image_base)
        else:
            return None
    return destination if expected == body else None


def matched_padding(pattern, rva, image, starts):
    start = rva + len(pattern.body)
    end = start + len(pattern.padding)
    if (not pattern.padding or rva % pattern.alignment or end % pattern.alignment
            or end not in starts or any(start <= p < end for p in starts)
            or image.relocs_in(start, end)
            or image.pe.read(start, end-start) != pattern.padding):
        return 0
    return end-start


def verified_roots(pe, rows):
    """Check the entire admitted slot sequence against one actual CRT table."""
    ordered = sorted((int(r['slot'], 0), int(r['rva'], 0)) for r in rows)
    if len(ordered) < 4 or [s for s, _ in ordered] != list(range(len(ordered))):
        return []
    table = b''.join(struct.pack('<I', pe.image_base+r) for _, r in ordered)
    start = pe.data.find(table)
    if start < 0 or pe.data.find(table, start+1) >= 0:
        return []
    if not any(s['name'] in ('.data', '.rdata')
               and s['rptr'] <= start < start+len(table) <= s['rptr']+s['rsize']
               for s in pe.sections):
        return []
    return [r for _, r in ordered]


def compare(project, pe, model, *, header='include/terrain.h', witness='iconwdgt'):
    """One ordinary compiled TU can witness repeated copies of a header body.

    Scope is deliberately explicit. Unsupported constructors, stores and
    unknown callees leave unclaimed bytes; they cannot produce partial credit.
    """
    from homm3.delink.image import Image
    from homm3.core.cc_wrap import scan_header_deps
    from homm3.retail_labels.censuses import functions
    result = dict(matches=[], gaps=[], header=header, witness=witness)
    root = project.root
    unit = next((u for u in project.manifest['unit'] if u['unit'] == witness), None)
    if not unit:
        result['gaps'].append('witness unit not admitted'); return result
    source, header = root / unit['source'], (root / header).resolve()
    objpath = root / f'build/objdiff/base/{witness}.obj'
    required = [source, header, root / 'config/units.toml', root / 'config/project.toml',
                *scan_header_deps(source, project.toolchain / 'include', *project.includes),
                *[p for p in (project.toolchain / 'bin').iterdir()
                  if p.is_file() and p.suffix.lower() in ('.exe', '.dll')]]
    inputs = compile_receipt.current(objpath, flags=project.manifest['flags'][unit['flags']],
                                     required=required)
    if inputs is None:
        result['gaps'].append('witness object missing or stale; rebuild it'); return result
    object_hash = compile_receipt.digest(objpath)
    owners, errors = declarations(project, source, header)
    if errors:
        result['gaps'].extend(errors); return result
    patterns = candidate_patterns(Obj(objpath), owners)
    if not patterns:
        result['gaps'].append('no supported source initializer emitted'); return result
    roots = verified_roots(pe, read(root / 'config/retail/init-thunks.tsv')[2])
    if not roots:
        result['gaps'].append('retail CRT table does not verify'); return result
    sizes = {row['rva']: row['size'] for row in functions()}
    targets = defaultdict(set)
    for b in model.functions:
        if b.name and b.channel:
            targets[b.name].add(b.rva)
    image = Image(pe)
    rows = []
    for rva in roots:
        size = sizes.get(rva, 0)
        body = pe.read(rva, size) if size else None
        if not body:
            continue
        hits = [(p, d) for p in patterns
                if (d := match_body(p, rva, body, image, targets)) is not None]
        if len(hits) != 1:
            if hits:
                result['gaps'].append(f'ambiguous source owner at {rva:#x}')
            continue
        pattern, destination = hits[0]
        if any(b.rva < destination+pattern.owner.size and destination < b.rva+b.size
               for b in model.data if b.size and b.channel):
            result['gaps'].append(f'conflicting storage claim at {destination:#x}')
        rows.append(dict(rva=rva, size=size, destination=destination,
                         owner=asdict(pattern.owner), witness=witness, verdict='exact',
                         padding_size=matched_padding(pattern, rva, image, sizes),
                         retail_unit=None))
    destinations = defaultdict(list)
    for row in rows:
        destinations[row['destination']].append(row)
    for destination, copies in sorted(destinations.items()):
        if len(copies) != 1:
            result['gaps'].append(f'multiple initializers write {destination:#x}')
        else:
            result['matches'].extend(copies)
    # A concurrent edit/compile must invalidate this report as well.
    if compile_receipt.current(objpath, flags=project.manifest['flags'][unit['flags']],
                               required=inputs) != inputs or compile_receipt.digest(objpath) != object_hash:
        result['matches'] = []
        result['gaps'].append('witness inputs changed during comparison')
    result['object_sha256'] = object_hash
    return result
