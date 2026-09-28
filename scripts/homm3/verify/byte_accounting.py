"""Complete file/image accounting for the Gruntz Model and its manifests.

Coverage is ownership, never a score. Unknown frontier bytes stay missing;
zero payload alone does not establish padding. Placed sections can cover
unnamed bytes, but provisional gap rows cannot credit recovery. Raw candidate
initializers are checked separately, including pointer identities/addends.
"""
from __future__ import annotations

from collections import Counter, defaultdict
from dataclasses import dataclass
import json
import struct
from pathlib import Path

from homm3.core import msvc_names
from homm3.core.paths import BUILD
from homm3.core.tsv import read, write


@dataclass(frozen=True)
class Range:
    start: int
    end: int
    category: str
    identity: str
    priority: int = 0


def partition(size: int, ranges: list[Range]):
    """Disjoint complete [0,size) partition; nested overlaps are retained.

    Equal named folded copies count once. Section topology supplies unnamed
    bytes at lower priority than actual definitions. Structural ranges only
    classify bytes not claimed by game/library contributions.
    """
    events = defaultdict(lambda: [set(), set()])
    unique = sorted(set(ranges), key=lambda r: (r.start, r.end, r.identity))
    for i, r in enumerate(unique):
        if not 0 <= r.start < r.end <= size:
            raise ValueError(f'range outside accounting domain: {r}')
        events[r.start][1].add(i)
        events[r.end][0].add(i)
    events[0]; events[size]
    active, result = set(), []
    points = sorted(events)
    for pos, end in zip(points, points[1:]):
        remove, add = events[pos]
        active.difference_update(remove)
        active.update(add)
        owners = [unique[i] for i in active]
        if owners:
            top = max(r.priority for r in owners)
            owners = [r for r in owners if r.priority == top]
        identities = tuple(sorted({r.identity for r in owners}))
        categories = {r.category for r in owners}
        category = ('missing' if not owners else 'overlap' if len(owners) > 1
                    else next(iter(categories)))
        row = {'start': pos, 'end': end, 'size': end-pos,
               'category': category, 'owners': identities}
        if result and all(result[-1][k] == row[k] for k in ('category', 'owners')):
            result[-1]['end'] = end
            result[-1]['size'] += end-pos
        else:
            result.append(row)
    if sum(r['size'] for r in result) != size:
        raise AssertionError('accounting partition lost bytes')
    return result


def manifest_rows(path=None):
    path = path or BUILD / 'gen/delink_data_manifest.tsv'
    return read(path)[2] if path.is_file() else []


def model_ranges(model, enrolled, sections):
    out = []
    for b in model.functions + model.data:
        if not b.size or not b.channel:
            continue
        category = ('library-vendor' if b.channel in (
            'functions_zlib', 'data_static_libs',
            'data_zlib') else 'game')
        if b.channel == 'functions_static_libs':
            category = 'library-unverified'
        out.append(Range(b.rva, b.rva+b.size, category, b.name, 2))
    for r in enrolled:
        if 'gap' in r.get('provenance', ''):
            continue
        start, size = int(r['rva'], 0), int(r['size'], 0)
        if size:
            category = ('library-vendor' if r.get('provenance') == 'zlib-source-sizeof'
                        else 'game')
            out.append(Range(start, start+size, category, r['name'], 2))
    for s in sections:
        if s['rva'] != '-' and int(s['size'], 0):
            start, size = int(s['rva'], 0), int(s['size'], 0)
            out.append(Range(start, start+size, 'section',
                             f"{s['name']}@{start:x}+{size:x}", 1))
    # The same identity admitted by both source and manifest is one claim.
    return list(set(out))


def verified_eh_groups(pe, model):
    """EH groups whose authored parent pushes the decoded registration stub."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    from homm3.delink import eh_band

    parents = {b.rva: (b.name, b.unit, b.size) for b in model.functions
               if b.channel in ('src', 'src_compgen', 'src_dyninit')
               and b.name and b.unit and b.size}
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    out = []
    for group in eh_band.groups(pe.path, parents, contiguous=False):
        if group.owner_rva not in parents:
            continue
        name, unit, size = parents[group.owner_rva]
        body = pe.read(group.owner_rva, size)
        expected = b'\x68' + struct.pack('<I', pe.image_base + group.stub)
        if body is None or not any(
                body[address-group.owner_rva:address-group.owner_rva+length] == expected
                for address, length, _, _ in disassembler.disasm_lite(body, group.owner_rva)):
            continue
        out.append((group, name, unit))
    return out


def compiler_ranges(pe, model, groups=None):
    """Attribute EH code to its authored parent, without claiming gap bytes.

    The parent's decoded registration push and FuncInfo identify the owner;
    the reviewed funclet census must independently agree and supply each
    cleanup body's own extent. This establishes ownership, not byte matching.
    """
    from homm3.core.paths import RETAIL
    from homm3.delink import eh_band
    from homm3.retail_labels import censuses

    sizes = {r['rva']: r['size'] for r in censuses.functions()}
    reviewed = {(int(r['rva'], 0), int(r['parent_rva'], 0))
                for r in read(RETAIL / 'funclets.tsv')[2]}
    text = pe.section('.text')
    lo, hi = text['va'], text['va'] + text['vsize']
    out = []
    for group, name, unit in (verified_eh_groups(pe, model) if groups is None else groups):
        if lo <= group.stub < group.stub + eh_band.STUB_SIZE <= hi:
            out.append(Range(group.stub, group.stub + eh_band.STUB_SIZE,
                             'compiler-generated', f'{unit}:{eh_band.registration_symbol(name)}', 1))
        for address in group.funclets:
            size = sizes.get(address, 0)
            if ((address, group.owner_rva) in reviewed and size
                    and lo <= address < address+size <= hi):
                out.append(Range(address, address+size, 'compiler-generated',
                                 f'{unit}:EH cleanup for {name}@{address:x}', 1))
    return out


def initializer_ranges(comparison):
    ranges = []
    for row in comparison['matches']:
        owner = row['owner']
        name = f"{owner['source']}:{owner['name']}@{row['destination']:x}"
        ranges.append(Range(row['rva'], row['rva']+row['size'],
                            'source-initializer-exact', name, 2))
        if row.get('padding_size'):
            end = row['rva'] + row['size']
            ranges.append(Range(end, end+row['padding_size'],
                                'source-initializer-padding-exact', name, 2))
        ranges.append(Range(row['destination'], row['destination']+owner['size'],
                            'game', name, 2))
    return ranges


def startup_ranges(comparison):
    """Credit checked code only; its DATA owners already have typed extents."""
    ranges = []
    for row in comparison['matches']:
        ranges.append(Range(row['rva'], row['rva']+row['size'],
                            'source-initializer-exact',
                            f"{row['source']}:CRT@{row['rva']:x}", 3))
    roots = {r['rva'] for r in comparison['matches']}
    # A cleanup referenced by several initializers is still one physical body.
    for rva, size in sorted({(r['rva'], r['size']) for r in comparison['dependencies']}):
        if rva not in roots:
            ranges.append(Range(rva, rva+size, 'source-cleanup-exact',
                                f'source-emitted cleanup@{rva:x}', 3))
    return ranges


def cleanup_ranges(comparison):
    """Code a claimed parent references; each body is counted once."""
    rows = {(r['rva'], r['size']): r for r in
            comparison['matches'] + comparison['dependencies']}
    return [Range(rva, rva+size, 'source-cleanup-exact',
                  f"{row['unit']}:{row['symbol']}@{rva:x}", 3)
            for (rva, size), row in sorted(rows.items())]


def padding_ranges(comparison, category, aligned=None):
    """Compiler/linker alignment bytes proven through the next boundary.

    Rows whose emitted section length differs from retail are `aligned`,
    not byte-exact; they get their own category when one is given.
    """
    return [Range(row['start'], row['end'],
                  aligned if aligned and row.get('kind') == 'aligned' else category,
                  f"{row['owner']}@{row['rva']:x}", 2)
            for row in comparison['matches']]


def code_alignment_tails(pe):
    """Header-proven alignment tails of executable sections.

    The section header states VirtualSize; LINK rounds the raw data up to
    FileAlignment and the next section starts at the SectionAlignment
    boundary. Only a tail shorter than that alignment, reaching exactly the
    next raw/virtual start and filled with the linker's zero fill, counts.
    Data sections are reported separately and are not classified here.
    """
    data = pe.data
    header = struct.unpack_from('<I', data, 0x3c)[0]
    count = struct.unpack_from('<H', data, header + 6)[0]
    optional = struct.unpack_from('<H', data, header + 20)[0]
    section_alignment, file_alignment = struct.unpack_from('<II', data, header + 24 + 32)
    image_size = struct.unpack_from('<I', data, header + 24 + 56)[0]
    rows = []
    for index in range(count):
        base = header + 24 + optional + index * 40
        vsize, va, rsize, rptr = struct.unpack_from('<IIII', data, base + 8)
        characteristics = struct.unpack_from('<I', data, base + 36)[0]
        rows.append((va, vsize, rsize, rptr, characteristics,
                     data[base:base + 8].rstrip(b'\0').decode('latin-1')))
    file_ranges, image_ranges = [], []

    def zero_runs(lo, hi, rebase):
        # Linker fill is zero; any other byte stays missing for review.
        start = None
        for offset in range(lo, hi + 1):
            if offset < hi and data[offset] == 0:
                start = offset if start is None else start
            elif start is not None:
                yield start + rebase, offset + rebase
                start = None

    for i, (va, vsize, rsize, rptr, characteristics, name) in enumerate(rows):
        if not characteristics & 0x20:
            continue
        following = rows[i + 1] if i + 1 < len(rows) else None
        virtual_end = -(-vsize // section_alignment) * section_alignment
        raw_end = -(-vsize // file_alignment) * file_alignment
        raw_tail = (rsize == raw_end and rsize > vsize
                    and (following[3] if following else len(data)) == rptr + rsize)
        if raw_tail:
            for a, b in zero_runs(rptr + vsize, rptr + rsize, 0):
                file_ranges.append(Range(a, b, 'structural',
                                         f'{name} FileAlignment tail', -1))
        if (virtual_end > vsize and rsize <= virtual_end
                and (following[0] if following else image_size) == va + virtual_end):
            # File-backed tail bytes load as-is; the rest is loader zero fill.
            mapped = rptr + min(rsize, virtual_end) if rsize > vsize else rptr + vsize
            if rsize > vsize and not raw_tail:
                continue
            for a, b in zero_runs(rptr + vsize, mapped, va - rptr):
                image_ranges.append(Range(a, b, 'structural',
                                          f'{name} SectionAlignment tail', -1))
            if va + mapped - rptr < va + virtual_end:
                image_ranges.append(Range(va + mapped - rptr, va + virtual_end,
                                          'structural', f'{name} SectionAlignment tail', -1))
    return file_ranges, image_ranges


def account(pe, model, enrolled, sections, *, initializers=(), groups=None):
    data = pe.data
    opt = struct.unpack_from('<I', data, 0x3c)[0] + 24
    image_size, header_size = struct.unpack_from('<II', data, opt+56)
    claims = (model_ranges(model, enrolled, sections) + compiler_ranges(pe, model, groups)
              + list(initializers))
    file_ranges = [Range(0, header_size, 'structural', 'PE headers', -1)]
    image_ranges = [Range(0, header_size, 'structural', 'PE headers', -1)]
    for claim in claims:
        if not any(s['va'] <= claim.start < claim.end <=
                   s['va']+max(s['vsize'], s['rsize']) for s in pe.sections):
            raise ValueError(f'claim crosses retail section boundary: {claim}')
        image_ranges.append(claim)
    for s in pe.sections:
        lo, raw, virtual = s['va'], s['rsize'], s['vsize']
        # Only independently known PE metadata is classified as structural.
        if s['name'] in ('.rsrc', '.reloc'):
            image_ranges.append(Range(lo, lo+max(raw, virtual), 'structural',
                                      s['name'], -1))
            if raw:
                file_ranges.append(Range(s['rptr'], s['rptr']+raw,
                                         'structural', s['name'], -1))
        for r in claims:
            a, b = max(r.start, lo), min(r.end, lo+raw)
            if a < b:
                file_ranges.append(Range(s['rptr']+a-lo, s['rptr']+b-lo,
                                         r.category, r.identity, r.priority))
    # Code-section alignment tails are proven by the section headers.
    # Other alignment gaps, overlay and unclaimed zero fill remain explicit
    # missing ranges. No inferred library frontier or zero-content padding.
    tails = code_alignment_tails(pe)
    file_ranges += tails[0]
    image_ranges += tails[1]
    return {'file': partition(len(data), file_ranges),
            'image': partition(image_size, image_ranges)}


def compare_initializers(model, enrolled, pe, base_dir=None):
    """Raw COFF versus retail, without normalized payloads or guessed extents."""
    from homm3.delink.coffx import Obj
    from homm3.delink.image import Image
    from homm3.retail_labels.source import vc6_function_name
    base_dir = base_dir or BUILD / 'objdiff/base'
    names = defaultdict(set)
    for b in model.functions + model.data:
        for entry in (b, *b.aliases):
            if entry.name and entry.channel:
                names[('', msvc_names.mask(entry.name))].add(b.rva)
                if entry.unit:
                    names[(entry.unit, msvc_names.mask(entry.name))].add(b.rva)
    for r in enrolled:
        if 'gap' not in r.get('provenance', ''):
            unit = r['object'].removesuffix('.c')
            names[(unit, msvc_names.mask(r['name']))].add(int(r['rva'], 0))
            names[('', msvc_names.mask(r['name']))].add(int(r['rva'], 0))
    image = Image(pe)
    objects, members, results = {}, {}, []
    for r in enrolled:
        if 'gap' in r.get('provenance', ''):
            continue
        unit = r['object'].removesuffix('.c')
        start, size = int(r['rva'], 0), int(r['size'], 0)
        result = dict(unit=unit, name=r['name'], rva=start, size=size,
                      verdict='unavailable', different=0, unresolved=0, reason='')
        path = base_dir / f'{unit}.obj'
        if not path.is_file():
            result['reason'] = 'candidate object missing'
            results.append(result); continue
        if unit not in objects:
            objects[unit] = Obj(path)
            members[unit] = defaultdict(list)
            emitted_names = []
            for idx, value, section in objects[unit].iter_symbols():
                name = objects[unit].sym_name(idx)
                members[unit][msvc_names.mask(name)].append((value, section))
                if section > 0 or section == 0 and value:
                    emitted_names.append(name)
            # Clang hashes anonymous namespaces; VC6 encodes their source
            # filename. Reuse the strict module/signature bridge used for
            # functions. These aliases apply only inside this object's TU.
            for owner, spelling in list(names):
                if owner != unit or '@?A0x' not in spelling:
                    continue
                emitted = vc6_function_name(spelling, emitted_names, unit)
                if emitted is not None:
                    key = msvc_names.mask(emitted)
                    names[(unit, key)].update(names[(unit, spelling)])
                    members[unit][spelling] = members[unit][key]
        obj = objects[unit]
        # Manifest ordinals address the reconstructed TARGET topology; they
        # are compressed and are not candidate COFF section numbers.
        definitions = members[unit].get(msvc_names.mask(r['name']), [])
        hits = [(value, section) for value, section in definitions if section > 0]
        commons = [value for value, section in definitions if section == 0 and value]
        if not hits and len(commons) == 1 and r['storage'] == 'bss':
            payload = pe.read(start, size)
            if commons[0] != size or payload is None:
                result.update(verdict='mismatch', reason='COMMON extent differs')
            else:
                wrong = sum(bool(byte) for byte in payload)
                result.update(verdict='mismatch' if wrong else 'exact', different=wrong)
            results.append(result); continue
        if len(hits) != 1:
            result['reason'] = 'candidate definition absent or ambiguous'
            results.append(result); continue
        offset, sn = hits[0]
        sec = obj.section_table[sn-1]
        if offset + size > sec['size']:
            result['reason'] = 'candidate extent too short'
            result['verdict'] = 'mismatch'
            result['different'] = max(0, offset+size-sec['size'])
            results.append(result); continue
        raw = obj.section_payload(sn)
        payload = raw[offset:offset+size] if raw else bytes(size)
        retail = pe.read(start, size)
        if retail is None or len(payload) != size:
            result['reason'] = 'incomplete raw extent'
            results.append(result); continue
        relocated, wrong, unknown = set(), set(), set()
        required = {site-start for site in image.relocs_in(start, start+size)}
        present = set()
        for off, (name, typ) in obj.typed_relocations(sn).items():
            if not offset <= off < offset+size:
                continue
            relative = off-offset
            present.add(relative)
            word = set(range(relative, min(relative+4, size)))
            relocated |= word
            if relative+4 > size or typ != 6:
                unknown |= word; continue
            addend = struct.unpack_from('<i', payload, relative)[0]
            key = msvc_names.mask(name)
            targets = names.get((unit, key)) or names.get(('', key), set())
            if len(targets) != 1:
                unknown |= word; continue
            expected = (next(iter(targets))+pe.image_base+addend) & 0xffffffff
            actual = struct.unpack_from('<I', retail, relative)[0]
            if relative not in required or expected != actual:
                wrong |= word
        for relative in required-present:
            wrong.update(range(relative, min(relative+4, size)))
        wrong |= {i for i, (a, b) in enumerate(zip(payload, retail))
                  if i not in relocated and a != b}
        result.update(different=len(wrong), unresolved=len(unknown),
                      verdict='mismatch' if wrong else 'unresolved' if unknown else 'exact')
        results.append(result)
    return results


def report(model=None):
    from homm3.model import resolve
    from homm3.core.pe import image
    from homm3.core.common import HOMM3_DIR
    from homm3.core.project import Project
    from homm3.verify.source_initializers import compare
    from homm3.verify.startup_bodies import compare as compare_startup
    from homm3.verify import source_padding
    from homm3.verify.shared_initializers import compare as compare_shared
    from homm3.verify.local_cleanups import compare as compare_cleanups
    from homm3.verify.source_padding import compare as compare_padding
    model = model or resolve()
    enrolled = manifest_rows()
    section_path = BUILD / 'gen/delink_data_section_manifest.tsv'
    sections = read(section_path)[2] if section_path.is_file() else []
    pe = image()
    project = Project(HOMM3_DIR)
    dynamic = compare(project, pe, model)
    startup = compare_startup(project, pe, model, enrolled)
    shared = compare_shared(project, pe, model, excluded={
        row['rva'] for row in startup['matches'] + dynamic['matches']})
    # A local cleanup that is already a claimed source function keeps its owner.
    claimed_code = {b.rva for b in model.functions if b.channel}
    shared_credit = dict(matches=shared['matches'], dependencies=[
        row for row in shared['dependencies'] if row['rva'] not in claimed_code])
    objects = source_padding.Objects(project)
    cleanups = compare_cleanups(project, pe, model, enrolled, objects)
    functions = source_padding.function_entries(model)
    padding = compare_padding(project, pe, functions, objects)
    # A cleanup body that is also a claimed source function is checked once.
    claimed = {entry.rva for entry in functions}
    startup_padding = compare_padding(project, pe, [
        source_padding.Entry(row['unit'], row['symbol'], row['rva'], row['size'],
                             f"{row['unit']}:{row['symbol']}")
        for row in (startup['matches'] + startup['dependencies']
                    + shared['matches'] + shared_credit['dependencies']
                    + cleanups['matches'] + cleanups['dependencies'])
        if row['rva'] not in claimed], objects)
    groups = verified_eh_groups(pe, model)
    eh_padding = source_padding.compare_eh(project, pe, groups, objects)
    domains = account(pe, model, enrolled, sections, groups=groups,
                      initializers=initializer_ranges(dynamic)+startup_ranges(startup)
                      + startup_ranges(shared_credit) + cleanup_ranges(cleanups)
                      + padding_ranges(padding, 'source-padding-exact', 'source-padding-aligned')
                      + padding_ranges(startup_padding, 'source-initializer-padding-exact',
                                       'source-padding-aligned')
                      + padding_ranges(eh_padding, 'linker-padding'))
    comparisons = compare_initializers(model, enrolled, pe)
    doc = {'schema': 1, 'domains': domains, 'initializers': comparisons,
           'source_initializers': dynamic,
           'startup_initializers': startup,
           'shared_initializers': shared,
           'local_cleanups': cleanups,
           'source_padding': dict(functions=padding, startup=startup_padding,
                                  eh_contributions=eh_padding),
           'model_violations': model.violations, 'totals': {}}
    for domain, rows in domains.items():
        totals = Counter()
        for row in rows:
            totals[row['category']] += row['size']
        doc['totals'][domain] = dict(totals)
        write(BUILD / f'gen/data_coverage_{domain}.tsv',
              ['# GENERATED from the Gruntz model; complete byte accounting.'],
              ['start', 'end', 'size', 'category', 'owners'],
              [[hex(r['start']), hex(r['end']), r['size'], r['category'],
                ';'.join(r['owners'])] for r in rows])
    write(BUILD / 'gen/data_initializer_comparison.tsv',
          ['# GENERATED: raw VC6 initializer bytes and pointer identities.'],
          ['unit', 'name', 'rva', 'size', 'verdict', 'different', 'unresolved', 'reason'],
          comparisons)
    output = BUILD / 'gen/data_coverage.json'
    output.write_text(json.dumps(doc, indent=2)+'\n')
    return doc
