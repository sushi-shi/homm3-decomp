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
    'compiler-generated', 'compiler-metadata', 'library-unverified', 'section',
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


def compiler_ranges(pe, model):
    """Attribute EH code to its authored parent, without claiming gap bytes.

    The parent's decoded registration push and FuncInfo identify the owner;
    the reviewed funclet census must independently agree and supply each
    cleanup body's own extent. This establishes ownership, not byte matching.
    """
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    from homm3.core.paths import RETAIL
    from homm3.delink import eh_band
    from homm3.retail_labels import censuses

    parents = {b.rva: (b.name, b.unit, b.size) for b in model.functions
               if b.channel in ('src', 'src_compgen', 'src_dyninit')
               and b.name and b.unit and b.size}
    sizes = {r['rva']: r['size'] for r in censuses.functions()}
    reviewed = {(int(r['rva'], 0), int(r['parent_rva'], 0))
                for r in read(RETAIL / 'funclets.tsv')[2]}
    text = pe.section('.text')
    lo, hi = text['va'], text['va'] + text['vsize']
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
    verdicts, data = library_code.verify(pe, names=names, game_comdats=game,
                                         reloc_sites=Image(pe).reloc_sites)
    ranges = [Range(lo, hi, category, identity, 2)
              for lo, hi, category, identity in library_code.ranges(verdicts, pe)]
    defined = defaultdict(set)
    for v in verdicts:
        if v.verdict == 'exact' and v.row.kind != 'alias':
            for rva, name in v.symbols:
                defined[rva].add(msvc_names.mask(name))
    return ranges, library_code.summary(verdicts, data, game), defined


def account(pe, model, enrolled, sections, *, initializers=(), library=(),
            library_names=None):
    data = pe.data
    opt = struct.unpack_from('<I', data, 0x3c)[0] + 24
    image_size, header_size = struct.unpack_from('<II', data, opt+56)
    verified = [(r.start, r.end) for r in library]
    zlib_units = {r['unit'] for r in read(ZLIB_MAP)[2]} if ZLIB_MAP.is_file() else set()
    claims = (model_ranges(model, enrolled, sections, verified, library_names, zlib_units)
              + compiler_ranges(pe, model)
              + list(initializers) + list(library))
    file_ranges = [Range(0, header_size, 'structural', 'PE headers', -1)]
    image_ranges = [Range(0, header_size, 'structural', 'PE headers', -1)]
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
    # Alignment gaps, overlay and unclaimed zero fill remain explicit missing
    # ranges. No inferred library frontier or zero-content padding exemptions.
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
    model = model or resolve()
    enrolled = manifest_rows()
    section_path = BUILD / 'gen/delink_data_section_manifest.tsv'
    sections = read(section_path)[2] if section_path.is_file() else []
    pe = image()
    project = Project(HOMM3_DIR)
    dynamic = compare(project, pe, model)
    startup = compare_startup(project, pe, model, enrolled)
    library, library_report, library_names = library_ranges(pe, model)
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
    claims = (initializer_ranges(dynamic) + startup_ranges(startup) + records.ranges
              + compiler_padding)
    domains = account(pe, model, enrolled, sections, initializers=claims,
                      library=library, library_names=library_names)
    padding = retail_records.alignment_padding(pe, domains['image'], records, sections)
    if padding:
        domains = account(pe, model, enrolled, sections, initializers=claims + padding,
                          library=library, library_names=library_names)
    comparisons = compare_initializers(model, enrolled, pe, library_names=library_names)
    doc = {'schema': 1, 'domains': domains, 'initializers': comparisons,
           'source_initializers': dynamic,
           'startup_initializers': startup,
           'library_code': library_report,
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
