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
import re
import struct
from pathlib import Path

from homm3.compare.canonicalize import normalize_anon_ns_name
from homm3.core import msvc_names
from homm3.core.paths import BUILD, RETAIL
from homm3.core.tsv import read, write


@dataclass(frozen=True)
class Range:
    start: int
    end: int
    category: str
    identity: str
    priority: int = 0


ZLIB_MAP = RETAIL / 'zlib-map.tsv'

#: When one identity arrives through several channels, report the most
#: specific evidence first.
CATEGORY_PRECEDENCE = (
    'source-initializer-exact', 'source-initializer-padding-exact',
    'source-cleanup-exact', 'game', 'library-vendor', 'library-runtime',
    'compiler-generated', 'compiler-metadata', 'import-thunk', 'linker-import',
    'import-structure', 'library-unverified', 'section', 'source-padding-exact',
    'source-padding-aligned', 'patch-residue', 'padding', 'linker-padding',
    'alignment-padding', 'structural')


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
        # One identity claimed through several channels is one folded object.
        category = ('missing' if not owners else 'overlap' if len(identities) > 1
                    else min(categories, key=CATEGORY_PRECEDENCE.index))
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


def _covered(start, end, verified):
    """True when [start, end) lies inside the union of sorted verified ranges."""
    import bisect
    starts = [lo for lo, _hi in verified]
    k = bisect.bisect_right(starts, start) - 1
    while start < end:
        if k < 0 or k >= len(verified) or not verified[k][0] <= start < verified[k][1]:
            return False
        start = verified[k][1]
        k += 1
    return True


def _merged(ranges):
    """Sorted union of (start, end) extents."""
    out = []
    for start, end in sorted(ranges):
        if out and start <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], end))
        else:
            out.append((start, end))
    return out


def model_ranges(model, enrolled, sections, verified_library=(), library_names=None,
                 zlib_units=()):
    """Model claims; library labels inside byte-verified contributions yield.

    A data claim inside a verified library contribution yields only when the
    contribution itself defines that name at that address; any other claim
    there remains and shows as an overlap.
    """
    verified = sorted(verified_library)
    library_names = library_names or {}

    def library_owned(start, end, name):
        return verified and msvc_names.mask(name) in library_names.get(start, ()) \
            and _covered(start, end, verified)
    out = []
    for b in model.functions + model.data:
        if not b.size or not b.channel:
            continue
        if b.channel in ('functions_static_libs', 'functions_zlib') and verified \
                and _covered(b.rva, b.rva+b.size, verified):
            continue
        if b.space != 'text' and library_owned(b.rva, b.rva+b.size, b.name):
            continue
        category = ('library-vendor' if b.channel in (
            'functions_zlib', 'data_static_libs',
            'data_zlib') else 'game')
        if b.channel == 'functions_static_libs':
            category = 'library-unverified'
        out.append(Range(b.rva, b.rva+b.size, category, b.name, 2))
    # A folded literal also emitted by a game object is the game object's
    # copy: game objects precede the vendor objects on the link line.
    game_copies = {(r['rva'], r['name']) for r in enrolled
                   if r.get('object', '').removesuffix('.c') not in zlib_units}
    for r in enrolled:
        if 'gap' in r.get('provenance', ''):
            continue
        start, size = int(r['rva'], 0), int(r['size'], 0)
        if size:
            if library_owned(start, start+size, r['name']):
                continue
            # Zlib objects are vendor contributions, including their literals.
            vendor = r.get('provenance') == 'zlib-source-sizeof' or (
                r.get('object', '').removesuffix('.c') in zlib_units
                and (r['rva'], r['name']) not in game_copies)
            category = 'library-vendor' if vendor else 'game'
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


def library_ranges(pe, model):
    """Byte-verified static-library contributions (`homm3.verify.library_code`)."""
    from homm3.retail_labels.censuses import functions
    from homm3.verify import library_code
    names = defaultdict(set)
    for b in model.functions + model.data:
        for entry in (b, *b.aliases):
            if entry.name and entry.channel and entry.channel != 'functions_static_libs':
                names[msvc_names.mask(entry.name)].add(b.rva)
    zlib_units = {r['unit'] for r in read(library_code.ZLIB_MAP)[2]}
    game = library_code.GameComdats(pe, [r['rva'] for r in functions()],
                                    exclude=zlib_units)
    from homm3.delink.image import Image
    evidence = read(RETAIL / 'reloc-evidence.tsv')[2]
    referenced = {int(r['value'], 16) - pe.image_base for r in evidence
                  if r['disposition'].startswith('kept')}
    verdicts, data = library_code.verify(pe, names=names, game_comdats=game,
                                         reloc_sites=Image(pe).reloc_sites,
                                         referenced=referenced)
    ranges = [Range(lo, hi, category, identity, 2)
              for lo, hi, category, identity in library_code.ranges(verdicts, pe)]
    defined = defaultdict(set)
    for v in verdicts:
        if v.verdict == 'exact' and v.row.kind != 'alias':
            for rva, name in v.symbols:
                defined[rva].add(msvc_names.mask(name))
    return ranges, library_code.summary(verdicts, data, game), defined


def linker_ranges(pe, dynamic, startup_sets, library, library_names, enrolled,
                  model_claims=()):
    """Linker-produced import tables, game `.CRT$XCU` slots, fill and tails.

    `startup_sets` are match lists shaped like `startup_initializers`
    (rva, unit, symbol, source): every exact CRT body a slot may point to.
    """
    from homm3.verify import linker_structures as linker
    records = linker.import_records(pe, referenced=linker.referenced_slots())
    claims = [Range(r.start, r.start+r.size, r.category, f'import {r.kind} {r.name}', 2)
              for r in records]
    bodies = {}
    for row in dynamic['matches']:
        owner = row['owner']
        bodies[row['rva']] = (f"{owner['source']}:{owner['name']}", owner['name'], None, None)
    for matches in startup_sets:
        for row in matches:
            bodies[row['rva']] = (f"{row['source']}:{row['symbol']}", None,
                                  row.get('unit'), row.get('symbol'))
    covered = [(r.start, r.end) for r in library]
    slots, slot_findings = linker.crt_slots(pe, bodies, library_names, covered)
    claims += [Range(a, b, 'source-initializer-exact', identity, 2) for a, b, identity in slots]
    # LINK aligns the start of the .data group that follows the merged .CRT
    # group to the group's largest section alignment (a VC6 link of one
    # 4-aligned .data object with LIBCMT starts .data 16-aligned after
    # ___xt_z). The largest verified library .data alignment supplies it.
    from homm3.verify import library_code
    runtime = library_code.load_libraries(zlib_units=())
    group_alignment = max((runtime.section(r.library, r.member, r.section).align
                           for r in library_code.read_inventory()
                           if r.kind == 'data' and r.library in library_code.RUNTIME_LIBRARIES
                           and runtime.objects[(r.library, r.member)]
                           .sections[r.section - 1].name == '.data'), default=0)

    def section_alignment(_address):
        return group_alignment
    claims += [Range(a, b, 'linker-padding', identity, 2)
               for a, b, identity in linker.crt_group_fill(pe, library_names, section_alignment)]
    commons = [(r.start, r.end) for r in library if 'COMMON ' in r.identity]
    data_claims = [(r.start, r.end) for r in model_claims]
    claims += [Range(a, b, 'linker-padding', identity, 2)
               for a, b, identity in linker.common_zone_fill(pe, commons, data_claims)]
    file_tails, image_tails = linker.data_alignment_tails(pe)
    tails = ([Range(a, b, 'structural', identity, -1) for a, b, identity in file_tails],
             [Range(a, b, 'structural', identity, -1) for a, b, identity in image_tails])
    report = dict(imports=linker.summary(records), crt_slots=len(slots),
                  crt_findings=slot_findings)
    return claims, tails, report


CODE_EXTENT_CATEGORIES = {'patch-residue'}


def reviewed_code_extents(pe, model, path=None):
    """Reviewed `.text` extents that are neither a function nor its padding.

    `patch-residue`: bytes of an original body that a binary patch cut off
    behind a claimed entry. The row must begin exactly where that claimed
    function's reviewed extent ends, end at the next reviewed function start,
    and no admitted relocation may point into it (the bytes are unreachable).
    """
    from homm3.core.paths import RETAIL
    from homm3.retail_labels.censuses import functions
    path = path or RETAIL / 'code-extents.tsv'
    if not path.is_file():
        return []
    rows = [r for r in read(path)[2] if r['category'] != 'padding']
    starts = sorted(r['rva'] for r in functions())
    claimed_ends = {b.rva + b.size for b in model.functions if b.channel and b.size}
    targets = {int(r['value'], 16) - pe.image_base
               for r in read(RETAIL / 'reloc-evidence.tsv')[2]
               if r['value'].startswith('0x')}
    text = pe.section('.text')
    out = []
    for r in rows:
        rva, size = int(r['rva'], 0), int(r['size'], 0)
        end = rva + size
        following = next((s for s in starts if s > rva), None)
        if (r['category'] not in CODE_EXTENT_CATEGORIES or not r['evidence']
                or not text['va'] <= rva < end <= text['va'] + text['vsize']
                or rva not in claimed_ends or following != end
                or any(rva <= t < end for t in targets)):
            raise ValueError(f'code extent does not verify: {r}')
        out.append(dict(rva=rva, size=size, category=r['category'],
                        evidence=r['evidence']))
    return out


def linker_fill_ranges(*comparisons):
    """LINK's INT3 fill; one physical gap is credited once."""
    unique = {}
    for comparison in comparisons:
        for row in comparison['matches']:
            unique.setdefault((row['start'], row['end']), row)
    return [Range(start, end, 'linker-padding', f"{row['owner']}@{row['rva']:x}", 2)
            for (start, end), row in sorted(unique.items())]


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


def account(pe, model, enrolled, sections, *, initializers=(), groups=None, library=(),
            library_names=None, linker=(), tails=((), ()), padding=(), stale=None):
    data = pe.data
    opt = struct.unpack_from('<I', data, 0x3c)[0] + 24
    image_size, header_size = struct.unpack_from('<II', data, opt+56)
    verified = [(r.start, r.end) for r in library]
    zlib_units = {r['unit'] for r in read(ZLIB_MAP)[2]} if ZLIB_MAP.is_file() else set()
    claims = (model_ranges(model, enrolled, sections, verified, library_names, zlib_units)
              + compiler_ranges(pe, model, groups) + list(linker)
              + list(initializers) + list(library))
    # An exact comparison supersedes the reviewed padding row beneath it;
    # any other overlap is an error (`homm3.verify.padding.supersede`).
    from homm3.verify.padding import supersede
    kept, superseded = supersede(padding, claims)
    if stale is not None:
        stale += superseded
    claims += kept
    file_ranges = [Range(0, header_size, 'structural', 'PE headers', -1)] + list(tails[0])
    image_ranges = [Range(0, header_size, 'structural', 'PE headers', -1)] + list(tails[1])
    for claim in claims:
        # Only PE-structural claims (section-alignment tails) lie between
        # sections; every other claim stays inside one retail section.
        if claim.category != 'structural' and not any(
                s['va'] <= claim.start < claim.end <=
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


_ANON = re.compile(r'\?A0x[0-9A-Fa-f]+')


def _guard_owners(root):
    """{guard claim name: owner static} from source DATA_COMPGEN_GUARD rows."""
    head = re.compile(r'\bDATA_COMPGEN_GUARD\s*\(\s*0x[0-9a-fA-F]+\s*,\s*(\w+)\s*,\s*(\w+)\s*\)')
    data = re.compile(r'\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)')
    out = {}
    for path in sorted(Path(root, 'src').rglob('*.cpp')):
        unit = path.stem
        text = path.read_text(encoding='latin-1')
        for m in head.finditer(text):
            # The owner's own DATA annotation follows its guard (a STATIC_DTOR
            # row may sit between them) and names the owner's retail address.
            follow = data.search(text, m.end(), m.end() + 300)
            address = int(follow.group(1), 16) - 0x400000 if follow else None
            out[f'__h3cg${unit}$static_init_guard${m.group(1)}'] = (m.group(2), address)
    return out


def bridge_data_name(name, emitted, guard_owners, owner_names=None):
    """The candidate spelling of a claimed datum whose model name differs.

    Only unique, identity-preserving spellings are accepted: the anonymous
    namespace hash (which encodes the compiling path), VC6's `_name` form
    for statics in an anonymous namespace, a reference's cv letter, and a
    source guard's `$S` counter symbol in its owner static's scope.
    """
    owner_names = owner_names or {}

    def unique(keys):
        keys = [k for k in keys if emitted.get(k)]
        return keys[0] if len(keys) == 1 else None

    # Only a TYPE's anonymous namespace may be normalized: the variable's own
    # scope must still come from the strict module bridge.
    if not re.match(r'^_?\?\w+@\?A0x', name):
        anon = _ANON.sub('?A0x#', name)
        found = unique([k for k in emitted if _ANON.sub('?A0x#', k) == anon and k != name])
        if found:
            return found
    m = re.match(r'^\?(\w+)@\?A0x[0-9A-Fa-f]+@@3', name)
    if m and emitted.get('_' + m.group(1)):
        return '_' + m.group(1)
    if name.startswith('?') and '@@3AA' in name and name.endswith('B'):
        found = unique([name[:-1] + 'A'])
        if found:
            return found
    owner, address = guard_owners.get(name, (None, None))
    if owner:
        scopes = {k[len(f'_?{owner}@'):].split('@4', 1)[0] for k in emitted
                  if k.startswith(f'_?{owner}@?')}
        if len(scopes) > 1 and address is not None:
            named = {k[len(f'_?{owner}@'):].split('@4', 1)[0]
                     for k in owner_names.get(address, ()) if k.startswith(f'_?{owner}@?')}
            scopes &= named
        if len(scopes) == 1:
            scope = scopes.pop()
            return unique([k for k in emitted if k.startswith('_?$S')
                           and k.split('@', 1)[1].split('@4', 1)[0] == scope])
    return None


def folded_body(obj, name, rva, pe):
    """True when ``obj``'s code for ``name`` reproduces retail at ``rva``.

    The body runs to the section's next defined symbol; relocation words are
    masked, every other byte must agree, and at least eight bytes compare.
    """
    cache = obj.__dict__.setdefault('_folded_index', {})
    if not cache:
        for idx, value, section in obj.iter_symbols():
            if section > 0 and obj.section_table[section - 1]['characteristics'] & 0x20000000:
                cache.setdefault(obj.sym_name(idx), []).append((section, value))
    hits = cache.get(name, [])
    if len(hits) != 1:
        return False
    section, value = hits[0]
    starts = obj.__dict__.setdefault('_folded_starts', {})
    if section not in starts:
        starts[section] = sorted({v for rows in cache.values() for s2, v in rows
                                  if s2 == section})
    later = [v for v in starts[section] if v > value]
    end = later[0] if later else obj.section_table[section - 1]['size']
    body = obj.section_payload(section)[value:end]
    retail = pe.read(rva, len(body))
    if len(body) < 8 or retail is None:
        return False
    masked = set()
    for site in obj.typed_relocations(section):
        if value <= site < end:
            masked.update(range(site - value, site - value + 4))
    return all(a == b for i, (a, b) in enumerate(zip(body, retail)) if i not in masked)


def compare_initializers(model, enrolled, pe, base_dir=None, library_names=None):
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
    code_index = {}

    def defining_objects(symbol):
        """Other candidate objects defining ``symbol`` as code."""
        if not code_index:
            for path in sorted(base_dir.glob('*.obj')):
                other = Obj(path)
                for idx, value, section in other.iter_symbols():
                    if section > 0 and other.section_table[section - 1]['characteristics'] & 0x20000000:
                        code_index.setdefault(other.sym_name(idx), []).append(path)
            code_index['__loaded__'] = {}
        loaded = code_index['__loaded__']
        out = []
        for path in code_index.get(symbol, [])[:4]:
            if path not in loaded:
                loaded[path] = Obj(path)
            out.append(loaded[path])
        return out
    from homm3.core.common import HOMM3_DIR
    guard_owners = _guard_owners(HOMM3_DIR)
    anon_names = defaultdict(set)
    for (owner, key), rvas in list(names.items()):
        if '?A0x' in key:
            anon_names[_ANON.sub('?A0x#', key)].update(rvas)
            # VC6 spells a static inside an anonymous namespace `_name`.
            plain = re.match(r'^\?(\w+)@\?A0x[0-9A-Fa-f]+@@3', key)
            if plain and owner:
                names[(owner, '_' + plain.group(1))].update(rvas)
    # Verified runtime-library contributions name their own public symbols.
    for rva, symbols in (library_names or {}).items():
        for symbol in symbols:
            names.setdefault(('', symbol), set()).add(rva)
    owner_names = defaultdict(set)
    for b in model.data:
        for entry in (b, *b.aliases):
            if entry.name:
                owner_names[b.rva].add(msvc_names.mask(entry.name))
    from homm3.verify import eh_records
    results += eh_records.compare(model, enrolled, pe, names, base_dir)
    for r in enrolled:
        if 'gap' in r.get('provenance', '') or r.get('provenance') == 'retail-EH-funcinfo':
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
                canonical = normalize_anon_ns_name(name, unit)
                if canonical != name:
                    # A reviewed anonymous namespace: the model spells the
                    # retail scope, the object the compiling path.
                    members[unit][msvc_names.mask(canonical)].append((value, section))
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
        if not definitions:
            bridged = bridge_data_name(r['name'], members[unit], guard_owners, owner_names)
            if bridged:
                definitions = members[unit][bridged]
        hits = [(value, section) for value, section in definitions if section > 0]
        commons = [value for value, section in definitions if section == 0 and value]
        result['storage'] = r['storage']
        if not hits and len(commons) == 1 and r['storage'] == 'bss':
            from homm3.verify.library_code import common_alignment
            payload = pe.read(start, size)
            result.update(definition=commons[0], extent=(
                'ok' if commons[0] == size else 'beyond-candidate-definition'
                if commons[0] < size else 'candidate-larger-than-extent'),
                aligned=not start % common_alignment(commons[0]))
            if commons[0] != size or payload is None:
                result.update(verdict='mismatch', reason='COMMON extent differs')
            else:
                wrong = {i for i, byte in enumerate(payload) if byte}
                result.update(verdict='mismatch' if wrong else 'exact',
                              different=len(wrong), span=size)
                if wrong:
                    result['wrong'] = [[a, b, bytes(b - a).hex(), payload[a:b][:64].hex()]
                                       for a, b in _runs(wrong)]
            results.append(result); continue
        if len(hits) != 1:
            result['reason'] = 'candidate definition absent or ambiguous'
            results.append(result); continue
        offset, sn = hits[0]
        sec = obj.section_table[sn-1]
        # The extent is compared with the candidate definition itself: its
        # own bytes run to the next datum of its section (or the section end).
        following = [v for v, n, _scl in obj.section_members(sn) if v > offset]
        from homm3.verify.game_bytes import alignment_bound, definition_extent
        extent, definition = definition_extent(
            size, offset, sec['size'], min(following) if following else None,
            bool(sec['characteristics'] & 0x1000), sec['alignment'],
            obj.section_payload(sn))
        # The declared type's alignment divides both the member's offset in
        # its aligned section and its size; `verify_game` judges placement.
        result.update(extent=extent, definition=definition,
                      alignment_bound=alignment_bound(offset, size, sec['alignment']))
        # Only the candidate definition's own bytes can reproduce the claim;
        # bytes past it belong to a neighbour (or to no candidate datum).
        span = min(size, definition) if extent == 'beyond-candidate-definition' else size
        if offset + span > sec['size']:
            result['reason'] = 'candidate extent too short'
            result['verdict'] = 'mismatch'
            result['different'] = max(0, offset+span-sec['size'])
            results.append(result); continue
        raw = obj.section_payload(sn)
        payload = raw[offset:offset+span] if raw else bytes(span)
        retail = pe.read(start, span)
        if retail is None or len(payload) != span:
            result['reason'] = 'incomplete raw extent'
            results.append(result); continue
        relocated, wrong, unknown = set(), set(), set()
        required = {site-start for site in image.relocs_in(start, start+span)}
        present, pointers = set(), []
        for off, (name, typ) in obj.typed_relocations(sn).items():
            if not offset <= off < offset+span:
                continue
            relative = off-offset
            present.add(relative)
            word = set(range(relative, min(relative+4, span)))
            relocated |= word
            if relative+4 > span or typ != 6:
                unknown |= word; continue
            addend = struct.unpack_from('<i', payload, relative)[0]
            key = msvc_names.mask(name)
            targets = names.get((unit, key)) or names.get(('', key), set())
            if not targets and '?%' in name:
                canonical = msvc_names.mask(normalize_anon_ns_name(name, unit))
                targets = names.get((unit, canonical)) or names.get(('', canonical), set())
            if not targets and '?A0x' in key:
                # The anonymous-namespace hash encodes the compiling path.
                targets = anon_names.get(_ANON.sub('?A0x#', key), set())
            default = obj.weak_default(name)
            if not targets and default:
                # A weak external binds to its aux default when nothing
                # defines it: VC6's vector deleting destructor names ??_G.
                key = msvc_names.mask(default)
                targets = names.get((unit, key)) or names.get(('', key), set())
            if len(targets) != 1:
                # An identical-code-folded referent: the word names retail
                # code that this object's own definition of the symbol (or of
                # its weak default) reproduces byte for byte.
                actual = struct.unpack_from('<I', retail, relative)[0]
                claimed = (actual - pe.image_base - addend) & 0xffffffff
                if relative in required and any(
                        folded_body(other, candidate, claimed, pe)
                        for candidate in (name, default) if candidate
                        for other in [obj, *defining_objects(candidate)]):
                    continue
                unknown |= word; continue
            expected = (next(iter(targets))+pe.image_base+addend) & 0xffffffff
            actual = struct.unpack_from('<I', retail, relative)[0]
            if relative not in required or expected != actual:
                wrong |= word
                pointers.append(dict(offset=relative, symbol=name, addend=addend,
                                     expected=expected, actual=actual,
                                     retail_relocated=relative in required))
        for relative in required-present:
            wrong.update(range(relative, min(relative+4, span)))
            pointers.append(dict(offset=relative, symbol=None, expected=None,
                                 actual=struct.unpack_from('<I', retail, relative)[0]
                                 if relative + 4 <= span else None,
                                 retail_relocated=True))
        wrong |= {i for i, (a, b) in enumerate(zip(payload, retail))
                  if i not in relocated and a != b}
        result.update(different=len(wrong), unresolved=len(unknown), span=span,
                      verdict='mismatch' if wrong else 'unresolved' if unknown else 'exact')
        if wrong:
            result['wrong'] = [[a, b, payload[a:b][:64].hex(), retail[a:b][:64].hex()]
                               for a, b in _runs(wrong)]
            result['pointers'] = pointers
        if unknown:
            result['unknown'] = [[a, b] for a, b in _runs(unknown)]
        if span < size:
            # The claim is not reproduced past its definition.
            result.update(verdict='mismatch', reason='extent beyond candidate definition')
        results.append(result)
    return results


def _runs(offsets):
    """Sorted [start, end) runs of a set of integers."""
    out = []
    for i in sorted(offsets):
        if out and out[-1][1] == i:
            out[-1][1] = i + 1
        else:
            out.append([i, i + 1])
    return out


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
    from homm3.verify import import_thunks
    from homm3.core.paths import RETAIL
    from homm3.verify.source_padding import compare as compare_padding
    model = model or resolve()
    enrolled = manifest_rows()
    section_path = BUILD / 'gen/delink_data_section_manifest.tsv'
    sections = read(section_path)[2] if section_path.is_file() else []
    pe = image()
    project = Project(HOMM3_DIR)
    dynamic = compare(project, pe, model)
    startup = compare_startup(project, pe, model, enrolled)
    # Byte-verified library sections name their symbols at their own offsets.
    library, library_report, library_names = library_ranges(pe, model)
    shared = compare_shared(project, pe, model, excluded={
        row['rva'] for row in startup['matches'] + dynamic['matches']},
        library_names=library_names)
    # A local cleanup that is already a claimed source function keeps its owner.
    claimed_code = {b.rva for b in model.functions if b.channel}
    shared_credit = dict(matches=shared['matches'], dependencies=[
        row for row in shared['dependencies'] if row['rva'] not in claimed_code])
    objects = source_padding.Objects(project)
    cleanups = compare_cleanups(project, pe, model, enrolled, objects, library_names)
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
    from homm3.retail_labels.censuses import functions as census
    reviewed = census()
    claimed_code = {b.rva for b in model.functions if b.channel}
    runtime_rows = RETAIL / 'runtime-contributions.tsv'
    if runtime_rows.is_file():
        claimed_code |= {int(r['rva'], 0) for r in read(runtime_rows)[2]}
    thunks = import_thunks.compare(project, pe, {r['rva']: r['size'] for r in reviewed},
                                   claimed_code)
    ends = ({r['rva'] + r['size'] for r in reviewed}
            | {group.stub + 10 for group, _, _ in groups})
    before = source_padding.compare_before(project, pe, model, groups, ends, objects)
    code_extents = reviewed_code_extents(pe, model)
    data_claims = [Range(b.rva, b.rva+b.size, 'game', b.name) for b in model.data
                   if b.size and b.channel]
    # Shared-header initializer bodies are CRT slot targets as well.
    linker, tails, linker_report = linker_ranges(pe, dynamic,
                                                 [startup['matches'], shared['matches']],
                                                 library, library_names, enrolled,
                                                 data_claims)
    # Retail-proven records: compiler metadata, FP constants, COMDAT data,
    # member padding and literals (`homm3.verify.retail_records`).
    from homm3.delink.image import Image
    from homm3.verify import retail_records
    sites = set(Image(pe).reloc_sites)
    records = retail_records.pe_structures(pe)
    found = retail_records.metadata(pe, model, sites)
    constants = retail_records.fp_constants(pe, sites, retail_records.library_code(model))
    comdats = retail_records.comdat_contributions(enrolled)
    members, compiler_padding = retail_records.ordinary_members(pe, enrolled, model=model)
    literals = retail_records.source_literals(HOMM3_DIR, pe)
    pushed = retail_records.referenced_literals(
        pe, sites, [(r.start, r.end) for x in (found, constants) for r in x.ranges])
    for extra in (found, constants, comdats, members, literals, pushed):
        records.ranges += extra.ranges
        records.starts.update(extra.starts)
        records.ends |= extra.ends
    # Byte-verified library sections and the verified import tables already
    # own their bytes; a retail-record reading inside one (an IAT slot, a
    # library FP constant, printable bytes inside library RTTI) adds nothing.
    verified = _merged([(r.start, r.end) for r in library]
                       + [(r.start, r.end) for r in linker
                          if r.category in ('linker-import', 'import-structure')])
    records.ranges = [r for r in records.ranges
                      if not _covered(r.start, r.end, verified)]
    # Padding is credited only through reviewed rows (`homm3.verify.padding`).
    # The inference passes below are generators: their ranges become review
    # proposals and never claim bytes on their own.
    function_padding = (padding_ranges(padding, 'source-padding-exact', 'source-padding-aligned')
                        + padding_ranges(startup_padding, 'source-initializer-padding-exact',
                                         'source-padding-aligned'))
    inferred = ([r for r in function_padding if r.category == 'source-padding-aligned']
                + linker_fill_ranges(eh_padding, before) + compiler_padding
                + [r for r in linker if r.category == 'linker-padding']
                + [Range(r.start, r.end, 'linker-padding', r.identity, 2) for r in library
                   if r.identity.startswith('link fill before')])
    library = [r for r in library if not r.identity.startswith('link fill before')]
    linker = [r for r in linker if r.category != 'linker-padding']
    from homm3.verify import padding as reviewed_padding
    claims = (initializer_ranges(dynamic) + startup_ranges(startup)
              + startup_ranges(shared_credit) + cleanup_ranges(cleanups)
              + [r for r in function_padding if r.category != 'source-padding-aligned']
              + [Range(r['rva'], r['rva'] + r['size'], 'import-thunk',
                       f"{r['dll']}!{r['imported']}", 2) for r in thunks['matches']]
              + [Range(r['rva'], r['rva'] + r['size'], r['category'],
                       f"reviewed {r['category']}@{r['rva']:x}", 2)
                 for r in code_extents]
              + records.ranges)
    stale = []
    arguments = dict(groups=groups, library=library, library_names=library_names,
                     linker=linker, tails=tails, padding=reviewed_padding.claims(pe),
                     stale=stale)
    domains = account(pe, model, enrolled, sections, initializers=claims, **arguments)
    # Link-alignment zero fill is proposed only where every other pass left a gap.
    inferred += retail_records.alignment_padding(pe, domains['image'], records, sections)
    # A verified import thunk is the address a call or table slot naming the
    # imported symbol links to (a vendor DLL's C++ export keeps its decorated
    # name in the import directory).
    referent_names = defaultdict(set, {rva: set(names) for rva, names in library_names.items()})
    for r in thunks['matches']:
        if isinstance(r['imported'], str):
            referent_names[r['rva']].add(r['imported'])
    comparisons = compare_initializers(model, enrolled, pe, library_names=referent_names)
    from homm3.verify.game_bytes import (destination_comparisons, header_compilands,
                                         verify_game)
    destinations = destination_comparisons(dynamic, pe)
    markers = {name: rva for rva, ns in library_names.items() for name in ns}
    compilands = (header_compilands(pe, dynamic.get('matches', ()), markers['___xc_a'],
                                    markers['___xc_z'])
                  if '___xc_a' in markers and '___xc_z' in markers else [])
    domains, verification = verify_game(pe, model, domains, comparisons + destinations,
                                        inferred=inferred, compilands=compilands)
    verification['destinations'] = destinations
    # Rows an exact game verdict superseded must see that verdict now.
    reviewed_padding.confirm(stale, domains['image'])
    sources = {int(r['rva'], 0): Path(r['source']).name for r in reviewed_padding.rows()}
    for row in stale:
        row['source'] = sources.get(int(row['rva'], 16), '')
    verification['padding']['stale'] = dict(
        rows=stale, count=len(stale), bytes=sum(r['covered_bytes'] for r in stale))
    reviewed_padding.write_proposals(verification['padding']['proposals'])
    doc = {'schema': 1, 'domains': domains, 'initializers': comparisons,
           'game_verification': verification, 'padding': verification.pop('padding'),
           'source_initializers': dynamic,
           'startup_initializers': startup,
           'library_code': library_report,
           'linker_structures': linker_report,
           'shared_initializers': shared,
           'local_cleanups': cleanups,
           'source_padding': dict(functions=padding, startup=startup_padding,
                                  eh_contributions=eh_padding, linker_before=before),
           'import_thunks': thunks,
           'code_extents': code_extents,
           'model_violations': model.violations, 'totals': {}}
    for domain, rows in domains.items():
        totals = Counter()
        for row in rows:
            totals[row['category']] += row['size']
        doc['totals'][domain] = dict(totals)
        write(BUILD / f'gen/data_coverage_{domain}.tsv',
              ['# GENERATED from the Gruntz model; complete byte accounting.'],
              ['start', 'end', 'size', 'category', 'owners', 'reason'],
              [[hex(r['start']), hex(r['end']), r['size'], r['category'],
                ';'.join(r['owners']), r.get('reason', '')] for r in rows])
    write(BUILD / 'gen/data_initializer_comparison.tsv',
          ['# GENERATED: raw VC6 initializer bytes and pointer identities.'],
          ['unit', 'name', 'rva', 'size', 'verdict', 'different', 'unresolved', 'reason'],
          comparisons)
    output = BUILD / 'gen/data_coverage.json'
    output.write_text(json.dumps(doc, indent=2)+'\n')
    return doc
