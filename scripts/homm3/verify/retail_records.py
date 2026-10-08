"""Retail-proven structural records and link alignment for byte accounting.

These claims are OWNERSHIP, never a candidate match. Every extent is stated
by the retail record itself, and every pointer word the extent contains must
be an admitted relocation site. Nothing is sized by the distance to a label.

* PE import structures are bounded by the import directory (the same reader
  as ``homm3 sema coverage``). A hint/name record's single trailing pad byte
  is the PE format's even-alignment rule, credited only when it is zero.
  Raw bytes between a section's virtual size and its raw size are the file
  alignment tail only when the raw size is exactly the aligned virtual size.
* MSVC exception metadata is rooted by a registration stub,
  ``mov eax, <FuncInfo>; jmp ___CxxFrameHandler``. The FuncInfo's own counts
  size its unwind map, try-block map and catch-handler arrays. A group is one
  claim only when its records follow VC6's ``.xdata$x`` emission layout.
* Throw records are rooted by a code word naming a ThrowInfo; its catchable
  type array and catchable types follow their own counts.
* RTTI is rooted by a vtable's preceding complete-object-locator word; the
  hierarchy, base-class array and base-class descriptors follow their counts.
* Type descriptors are reached from those records only. Their extent is the
  type_info vtable word, the spare word and the NUL-terminated decorated name.

``alignment_padding`` then credits a zero gap only when the following claim
is a whole contribution whose required alignment places it exactly at the
aligned end of a preceding whole contribution. A zero gap with any other
explanation stays missing.
"""
from __future__ import annotations

from dataclasses import dataclass, field
import bisect
import struct

FUNCINFO_MAGIC = 0x19930520
#: cl's `.xdata$x` FuncInfo record (the candidate objects emit 32 bytes and
#: place the unwind map directly after it), then 8-byte aligned tables.
FUNCINFO_SIZE = 32
XDATA_ALIGNMENT = 8
#: Candidate COFF emits `??_R0` type descriptors in 8-byte aligned COMDATs.
TYPE_DESCRIPTOR_ALIGNMENT = 8
STUB_SIZE = 10


@dataclass
class Records:
    """Claims plus the alignment facts ``alignment_padding`` may use."""
    ranges: list = field(default_factory=list)
    #: start rva -> required alignment of a whole contribution starting there
    starts: dict = field(default_factory=dict)
    #: end rvas of whole contributions
    ends: set = field(default_factory=set)
    rejected: list = field(default_factory=list)

    def add(self, start, end, category, identity, priority, *, alignment=None,
            whole=False):
        from homm3.verify.byte_accounting import Range
        self.ranges.append(Range(start, end, category, identity, priority))
        if whole:
            if alignment:
                self.starts[start] = max(alignment, self.starts.get(start, 1))
            self.ends.add(end)


class _View:
    def __init__(self, pe):
        self.pe = pe
        self.base = pe.image_base
        self.sections = pe.sections
        text = pe.section('.text')
        self.text = (text['va'], text['va'] + text['vsize'])
        rdata, data = pe.section('.rdata'), pe.section('.data')
        self.rdata = (rdata['va'], rdata['va'] + rdata['vsize'])
        self.data = (data['va'], data['va'] + data['rsize'])

    def u32(self, rva):
        raw = self.pe.read(rva, 4)
        return None if raw is None else struct.unpack('<I', raw)[0]

    def target(self, rva, region):
        value = self.u32(rva)
        if value is None:
            return None
        value -= self.base
        return value if region[0] <= value < region[1] else None

    def cstring_end(self, rva, limit=4096):
        raw = self.pe.read(rva, 1)
        for n in range(limit):
            raw = self.pe.read(rva + n, 1)
            if raw is None:
                return None
            if raw == b'\0':
                return rva + n + 1
        return None


def pe_structures(pe) -> Records:
    """The import address table, bounded by the import directory.

    The directory itself, its lookup and hint/name records and the section
    alignment tails are verified by the library code verifier; only the IAT
    slots and terminators, which game code reads through, are claimed here.
    """
    from homm3.sema.coverage import system_claims
    out = Records()
    data = pe.data
    opt = struct.unpack_from('<I', data, 0x3c)[0] + 24
    sections = [dict(rva=s['va'], raw_size=s['rsize'], raw_offset=s['rptr'])
                for s in pe.sections]
    for claim in system_claims(data, sections, opt):
        if claim['evidence'] in ('PE IAT slot', 'PE IAT terminator'):
            out.add(claim['rva'], claim['rva'] + claim['size'], 'structural',
                    claim['evidence'], 0)
    return out


def _owner_names(model):
    functions = sorted((b.rva, b.size, b.name) for b in model.functions if b.size)
    starts = [f[0] for f in functions]

    def owner(rva):
        i = bisect.bisect_right(starts, rva) - 1
        if i >= 0 and rva < functions[i][0] + functions[i][1]:
            return functions[i][2] or f'fn_{functions[i][0]:x}'
        return None
    return owner


def library_code(model):
    """Predicate: an address inside a runtime-library function.

    Library-owned data belongs to the library verifier; the records here are
    rooted only in game code.
    """
    spans = sorted((b.rva, b.rva + b.size) for b in model.functions
                   if b.channel in ('functions_static_libs',) and b.size)
    starts = [lo for lo, _hi in spans]

    def inside(rva):
        i = bisect.bisect_right(starts, rva) - 1
        return i >= 0 and rva < spans[i][1]
    return inside


def _named(model, name):
    hits = [b.rva for b in model.functions + model.data
            if b.name == name or any(a.name == name for a in b.aliases)]
    return hits[0] if len(set(hits)) == 1 else None


def _pointer_sites_admitted(view, start, words, sites):
    return all(start + 4 * w in sites for w in words)


def metadata(pe, model, sites) -> Records:
    """EH, throw and type-descriptor records rooted in game code."""
    view = _View(pe)
    out = Records()
    owner = _owner_names(model)
    library = library_code(model)
    handler = _named(model, '___CxxFrameHandler')
    type_info_vtable = _named(model, '??_7type_info@@6B@')
    if handler is None or type_info_vtable is None:
        out.rejected.append(('-', 'frame handler or type_info vtable is not uniquely named'))
        return out
    text_lo, text_hi = view.text
    code = pe.read(text_lo, text_hi - text_lo)
    type_descriptors = {}

    def type_descriptor(rva, root):
        if rva is None:
            return False
        if rva in type_descriptors:
            return True
        if not (view.data[0] <= rva < view.data[1]):
            return False
        if (view.u32(rva) != type_info_vtable + view.base or view.u32(rva + 4) != 0
                or pe.read(rva + 8, 1) != b'.' or rva not in sites):
            return False
        end = view.cstring_end(rva + 8)
        if end is None:
            return False
        type_descriptors[rva] = end
        out.add(rva, end, 'compiler-metadata', f'type descriptor {pe.read(rva + 8, end - rva - 9).decode("latin-1")}',
                1, alignment=TYPE_DESCRIPTOR_ALIGNMENT, whole=True)
        return True

    # --- exception frames -------------------------------------------------
    pushes = {}
    for offset in range(len(code) - 5):
        if code[offset] == 0x68:
            value = struct.unpack_from('<I', code, offset + 1)[0] - view.base
            if text_lo <= value < text_hi:
                pushes.setdefault(value, []).append(text_lo + offset)
    for offset in range(len(code) - STUB_SIZE):
        if code[offset] != 0xB8 or code[offset + 5] != 0xE9:
            continue
        stub = text_lo + offset
        if stub + STUB_SIZE + struct.unpack_from('<i', code, offset + 6)[0] != handler:
            continue
        funcinfo = struct.unpack_from('<I', code, offset + 1)[0] - view.base
        header = pe.read(funcinfo, FUNCINFO_SIZE)
        if header is None or struct.unpack_from('<I', header)[0] != FUNCINFO_MAGIC:
            out.rejected.append((stub, 'stub does not load a FuncInfo'))
            continue
        _, states, unwind, tries, trymap, nip, ip = struct.unpack_from('<IiIiIiI', header)
        if not pushes.get(stub) or any(library(site) for site in pushes[stub]):
            continue
        parents = {owner(site) for site in pushes.get(stub, ())} - {None}
        name = parents.pop() if len(parents) == 1 else f'stub@{stub:x}'
        layout, pointers = [(funcinfo, FUNCINFO_SIZE)], []
        if states < 0 or tries < 0 or nip or ip:
            out.rejected.append((funcinfo, 'FuncInfo counts are unsupported'))
            continue
        if states:
            pointers.append(funcinfo + 8)
            layout.append((unwind - view.base, 8 * states))
            pointers += [unwind - view.base + 8 * i + 4 for i in range(states)
                         if view.u32(unwind - view.base + 8 * i + 4)]
        if tries:
            pointers.append(funcinfo + 16)
            layout.append((trymap - view.base, 20 * tries))
            for i in range(tries):
                block = trymap - view.base + 20 * i
                catches = view.u32(block + 12)
                handlers = view.u32(block + 16) - view.base
                pointers.append(block + 16)
                layout.append((handlers, 16 * catches))
                for c in range(catches):
                    entry = handlers + 16 * c
                    pointers.append(entry + 12)
                    if view.u32(entry + 4):
                        pointers.append(entry + 4)
                        if not type_descriptor(view.u32(entry + 4) - view.base,
                                               f'catch in {name}'):
                            out.rejected.append((entry, 'catch type is not a type descriptor'))
        if not all(p in sites for p in pointers):
            out.rejected.append((funcinfo, 'FuncInfo pointer word is not an admitted relocation'))
            continue
        # VC6 emits one `.xdata$x` contribution: FuncInfo, unwind map, try
        # map, handler arrays, each table 8-byte aligned after the previous.
        cursor, packed = funcinfo, True
        for start, size in layout:
            if start != -(-cursor // XDATA_ALIGNMENT) * XDATA_ALIGNMENT:
                packed = False
            cursor = start + size
        if packed:
            out.add(funcinfo, cursor, 'compiler-metadata', f'EH data for {name}', 1,
                    alignment=XDATA_ALIGNMENT, whole=True)
        else:
            for start, size in layout:
                out.add(start, start + size, 'compiler-metadata',
                        f'EH data for {name}', 1)

    # --- throw records ----------------------------------------------------
    def throw_info(rva):
        raw = pe.read(rva, 16)
        if raw is None or not (view.rdata[0] <= rva < view.rdata[1]):
            return None
        attributes, unwind, forward, array = struct.unpack('<IIII', raw)
        if attributes > 3 or forward:
            return None
        if unwind and not text_lo <= unwind - view.base < text_hi:
            return None
        array -= view.base
        count = view.u32(array)
        if count is None or not 1 <= count <= 64:
            return None
        types = [view.u32(array + 4 + 4 * i) for i in range(count)]
        if any(t is None for t in types):
            return None
        types = [t - view.base for t in types]
        if any(type_descriptor_ok(view.u32(t + 4)) is False for t in types):
            return None
        return array, count, types

    def type_descriptor_ok(value):
        if value is None:
            return False
        rva = value - view.base
        return (view.data[0] <= rva < view.data[1]
                and view.u32(rva) == type_info_vtable + view.base)

    seen_throw = set()
    for offset in range(0, len(code) - 4):
        value = struct.unpack_from('<I', code, offset)[0] - view.base
        if value in seen_throw or not view.rdata[0] <= value < view.rdata[1]:
            continue
        found = throw_info(value)
        if not found:
            continue
        array, count, types = found
        words = [value + 12] + ([value + 4] if view.u32(value + 4) else [])
        words += [array + 4 + 4 * i for i in range(count)]
        words += [t + 4 for t in types] + [t + 24 for t in types if view.u32(t + 24)]
        if (not all(w in sites for w in words) or text_lo + offset not in sites
                or library(text_lo + offset)):
            continue
        seen_throw.add(value)
        name = f'throw {value:x}'
        out.add(value, value + 16, 'compiler-metadata', f'ThrowInfo {name}', 1,
                alignment=XDATA_ALIGNMENT, whole=True)
        out.add(array, array + 4 + 4 * count, 'compiler-metadata',
                f'CatchableTypeArray {name}', 1, alignment=XDATA_ALIGNMENT, whole=True)
        for t in types:
            out.add(t, t + 28, 'compiler-metadata', f'CatchableType {name}', 1,
                    alignment=XDATA_ALIGNMENT, whole=True)
            type_descriptor(view.u32(t + 4) - view.base, name)

    # RTTI graphs exist only for runtime-library classes (the game is built
    # without /GR); they are library data and are not claimed here.
    # Any other admitted reference naming a complete type descriptor
    # (typeid, dynamic_cast and catch operands) roots it too.
    for site in sorted(sites):
        if not text_lo <= site < text_hi or library(site):
            continue
        target = view.u32(site)
        if target is not None and view.data[0] <= target - view.base < view.data[1]:
            type_descriptor(target - view.base, f'reference at {site:x}')
    return out


def _extended80(raw: bytes) -> str:
    """VC6's `__real@` spelling: the constant as an x87 80-bit value."""
    import math
    value = struct.unpack('<d' if len(raw) == 8 else '<f', raw)[0]
    if value == 0 or not math.isfinite(value):
        sign = 0x8000 if math.copysign(1, value) < 0 else 0
        return f'{sign:04x}' + '0' * 16 if value == 0 else ''
    mantissa, exponent = math.frexp(abs(value))
    bits = int(mantissa * (1 << 64))
    sign = 0x8000 if value < 0 else 0
    return f'{sign | (exponent - 1 + 16383):04x}{bits:016x}'


#: x87 memory-operand widths for `op modrm(mod=00, rm=101) disp32`, keyed by
#: (opcode, reg): the operand is a floating-point constant of that size.
FPU_REAL = {**{(0xD8, r): 4 for r in range(8)}, **{(0xDC, r): 8 for r in range(8)},
            (0xD9, 0): 4, (0xDD, 0): 8}


def fp_constants(pe, sites, library=lambda rva: False) -> Records:
    """Read-only x87 constants in `.rdata`, sized by every retail access.

    Each admitted code reference to the address must be a direct x87 load or
    arithmetic operand of one real width. The constant's `__real@` name is
    VC6's spelling of its value, so a COMDAT emitted by candidate or library
    code for the same constant folds onto this claim.
    """
    view = _View(pe)
    text_lo, text_hi = view.text
    widths, game_users = {}, set()
    for site in sites:
        if not text_lo + 2 <= site < text_hi:
            continue
        target = view.target(site, view.rdata)
        if target is None:
            continue
        head = pe.read(site - 2, 2)
        width = None
        if head[1] & 0xC7 == 0x05:
            width = FPU_REAL.get((head[0], (head[1] >> 3) & 7))
        widths.setdefault(target, set()).add(width)
        if not library(site):
            # Game objects precede the libraries on the link line, so a
            # constant any game function loads is the game's COMDAT.
            game_users.add(target)
    out = Records()
    for target, seen in sorted(widths.items()):
        if len(seen) != 1 or None in seen or target not in game_users:
            continue
        width = seen.pop()
        raw = pe.read(target, width)
        name = _extended80(raw)
        if not name:
            continue
        out.add(target, target + width, 'compiler-metadata',
                f'__real@{width}@{name}', 1, alignment=width, whole=True)
    return out


_COMPGEN_LITERAL = None


def _c_string(body: str) -> bytes | None:
    """Bytes of one C string-literal body (VC6 narrow, Latin-1 source)."""
    out, i = bytearray(), 0
    simple = {'n': 10, 't': 9, 'r': 13, '0': 0, '\\': 92, '"': 34, "'": 39,
              'a': 7, 'b': 8, 'f': 12, 'v': 11, '?': 63}
    while i < len(body):
        c = body[i]
        if c != '\\':
            out += c.encode('latin-1')
            i += 1
            continue
        i += 1
        if i >= len(body):
            return None
        c = body[i]
        if c == 'x':
            j = i + 1
            while j < len(body) and body[j] in '0123456789abcdefABCDEF':
                j += 1
            out.append(int(body[i + 1:j], 16) & 0xff)
            i = j
        elif c in '01234567':
            j = i
            while j < len(body) and j < i + 3 and body[j] in '01234567':
                j += 1
            out.append(int(body[i:j], 8) & 0xff)
            i = j
        elif c in simple:
            out.append(simple[c])
            i += 1
        else:
            return None
    return bytes(out)


def source_literals(root, pe) -> Records:
    """String literals a source DATA_COMPGEN places at a retail address.

    The annotation is the reviewed address claim; the literal's own bytes
    plus its NUL must equal retail there. Nothing else is inferred: a value
    that is not a plain (possibly concatenated) narrow string literal, or
    whose bytes differ, claims nothing.
    """
    import re
    from pathlib import Path
    head = re.compile(r'\bDATA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(\w+)\s*,\s*')
    literal = re.compile(r'"((?:[^"\\\n]|\\.)*)"\s*')
    out = Records()
    seen = {}
    from homm3.core import images
    for path in sorted([*Path(root, 'src').rglob('*.cpp'), *Path(root, 'include').rglob('*.h')]):
        if images.foreign(path, root):
            continue
        text = path.read_text(encoding='latin-1')
        for m in head.finditer(text):
            at, parts = m.end(), []
            while True:
                lm = literal.match(text, at)
                if not lm:
                    break
                parts.append(lm.group(1))
                at = lm.end()
            if not parts or not text.startswith(')', at):
                continue
            data = b''.join(_c_string(p) or b'\xff\xff' for p in parts)
            if any(_c_string(p) is None for p in parts):
                continue
            rva = int(m.group(1), 16) - pe.image_base
            if pe.read(rva, len(data) + 1) != data + b'\0':
                continue
            seen.setdefault((rva, len(data) + 1), m.group(2))
    data = pe.section('.data')
    for (rva, size), name in sorted(seen.items()):
        # Pooled literals in .data are 4-aligned `??_C@` COMDATs in every
        # candidate object; elsewhere no contribution shape is assumed.
        pooled = data['va'] <= rva < data['va'] + data['vsize']
        out.add(rva, rva + size, 'game', f'source literal {name}', 1,
                alignment=4 if pooled else None, whole=pooled)
    return out


def referenced_literals(pe, sites, taken=()) -> Records:
    """C string literals that retail pushes or stores by address.

    A `push imm32` operand, or an initialized pointer word, that is an
    admitted relocation naming a 4-aligned run of printable bytes and its NUL
    is a string argument: the literal's extent is its own text plus the
    terminator. Game objects emit these as 4-aligned `??_C@` COMDATs; the
    claim sizes the object, not its owner.
    """
    printable = set(range(0x20, 0x7f)) | {9, 10, 13}
    view = _View(pe)
    text_lo, text_hi = view.text
    out = Records()
    seen = set()
    for site in sorted(sites):
        in_code = text_lo + 1 <= site < text_hi
        if in_code and pe.read(site - 1, 1) != b'\x68':
            # `mov edi, imm32` feeding an inline strlen (`repne scasb`)
            # names a string just as a pushed argument does.
            opcode, tail = pe.read(site - 1, 1), pe.read(site + 4, 12) or b''
            if opcode != b'\xbf' or b'\xf2\xae' not in tail:
                continue
        if not in_code and not (view.rdata[0] <= site < view.rdata[1]
                                or view.data[0] <= site < view.data[1]):
            continue
        value = view.u32(site)
        if value is None:
            continue
        target = value - view.base
        if target in seen or target % 4 or not view.data[0] <= target < view.data[1]:
            continue
        end = view.cstring_end(target, 512)
        if end is None or end - target < 2 or (not in_code and end - target > 64):
            continue
        body = pe.read(target, end - target - 1)
        if any(b not in printable for b in body):
            continue
        if any(lo < end and target < hi for lo, hi in taken):
            continue            # another retail record already sizes it
        seen.add(target)
        out.add(target, end, 'game', f'pushed literal {body[:24].decode("latin-1")!r}',
                1, alignment=4, whole=True)
    return out


def comdat_contributions(enrolled, base_dir=None) -> Records:
    """Whole COMDAT contributions among enrolled rows, with COFF alignment.

    A row is one whole contribution when its candidate definition is the only
    external symbol of a COMDAT section, at offset 0, and the row's extent is
    that section's size. The section's COFF alignment is then the linker's
    placement requirement.
    """
    from pathlib import Path
    from homm3.core import msvc_names
    from homm3.core.paths import BUILD
    from homm3.delink.coffx import Obj
    base_dir = Path(base_dir or BUILD / 'objdiff/base')
    out, objects = Records(), {}
    for r in enrolled:
        if 'gap' in r.get('provenance', ''):
            continue
        unit = r['object'].removesuffix('.c')
        if unit not in objects:
            path = base_dir / f'{unit}.obj'
            obj = Obj(path) if path.is_file() else None
            index = {}
            for sec in (obj.section_table if obj else ()):
                if sec['characteristics'] & 0x1000:
                    members = obj.defined_symbols(sec['index'])
                    if len(members) == 1 and members[0][0] == 0:
                        index[msvc_names.mask(members[0][1])] = sec
            objects[unit] = index
        sec = objects[unit].get(msvc_names.mask(r['name']))
        start, size = int(r['rva'], 0), int(r['size'], 0)
        if sec is not None and sec['size'] == size and not start % sec['alignment']:
            out.starts[start] = max(sec['alignment'], out.starts.get(start, 1))
            out.ends.add(start + size)
    return out


def ordinary_members(pe, enrolled, base_dir=None, model=None):
    """Compiler padding inside, and contribution edges of, ordinary sections.

    Source data defined in one candidate `.data`/`.rdata`/`.bss` section keeps
    the compiler's own member layout. Where two members that are adjacent in
    the candidate section sit at the same distance in retail, the bytes
    between them are the compiler's padding: claimed when the candidate
    emitted zeros there (always, for uninitialized storage) and retail agrees.
    A member at offset 0 starts the section contribution (with the section's
    COFF alignment); a member ending at the section size ends it.
    """
    from pathlib import Path
    from homm3.core import msvc_names
    from homm3.core.paths import BUILD
    from homm3.delink.coffx import Obj
    from homm3.verify.byte_accounting import Range
    base_dir = Path(base_dir or BUILD / 'objdiff/base')
    from homm3.core.common import HOMM3_DIR
    from homm3.verify.byte_accounting import _guard_owners, bridge_data_name
    guard_owners = _guard_owners(HOMM3_DIR)
    owner_names = {}
    for b in (model.data if model is not None else ()):
        for entry in (b, *b.aliases):
            if entry.name:
                owner_names.setdefault(b.rva, set()).add(msvc_names.mask(entry.name))
    out, pads = Records(), []
    by_unit = {}
    claimed = [(int(r['rva'], 0), int(r['rva'], 0) + int(r['size'], 0),
                r['object'].removesuffix('.c'), r['name']) for r in enrolled
               if 'gap' not in r.get('provenance', '') and int(r['size'], 0)]
    for r in enrolled:
        if r.get('provenance') == 'src-DATA-sizeof':
            by_unit.setdefault(r['object'].removesuffix('.c'), []).append(r)
    for unit, rows in sorted(by_unit.items()):
        path = base_dir / f'{unit}.obj'
        if not path.is_file():
            continue
        obj = Obj(path)
        where = {}
        for idx, value, secnum in obj.iter_symbols():
            if secnum > 0:
                where.setdefault(msvc_names.mask(obj.sym_name(idx)), []).append((secnum, value))
        sections = {}
        for r in rows:
            hits = where.get(msvc_names.mask(r['name']), [])
            if not hits:
                # Guards and path-spelled statics reach their candidate
                # symbol through the comparison's identity bridges.
                bridged = bridge_data_name(msvc_names.mask(r['name']), where,
                                           guard_owners, owner_names)
                hits = where.get(bridged, []) if bridged else []
            if len(hits) != 1:
                continue
            secnum, offset = hits[0]
            sec = obj.section_table[secnum - 1]
            if sec['characteristics'] & 0x1000 or sec['name'] not in ('.data', '.rdata', '.bss'):
                continue
            sections.setdefault(secnum, []).append(
                (offset, int(r['size'], 0), int(r['rva'], 0), r['name']))
        for secnum, members in sections.items():
            sec = obj.section_table[secnum - 1]
            payload = obj.section_payload(secnum)
            # A whole ordinary section is one contiguous link contribution.
            # When every datum it defines is claimed, no foreign claim lies
            # between them and the retail band is exactly the section size,
            # the zero bytes left inside the band are that section's padding.
            names = {n for _v, n, _scl in obj.section_members(secnum)}
            placed = {n for _o, _z, _r, n in members}
            lo = min(r for _o, _z, r, _n in members)
            hi = max(r + z for _o, z, r, _n in members)
            foreign = [x for x in claimed if x[0] < hi and lo < x[1]
                       and (x[2], x[3]) not in {(unit, n) for n in placed}]
            complete = {msvc_names.mask(n) for n in names} <= {
                msvc_names.mask(n) for n in placed}
            if complete and hi - lo == sec['size'] and not foreign:
                out.starts[lo] = max(sec['alignment'], out.starts.get(lo, 1))
                out.ends.add(hi)
                cursor = lo
                for _o, z, r, n in sorted(members, key=lambda m: m[2]):
                    if r > cursor:
                        retail = pe.read(cursor, r - cursor)
                        if retail is not None and not any(retail):
                            pads.append(Range(cursor, r, 'alignment-padding',
                                              f'{unit} compiler padding at 0x{cursor:x}', 0))
                    cursor = max(cursor, r + z)
            others = sorted(value for value_name in obj.section_members(secnum)
                            for value in [value_name[0]])
            members.sort()
            for offset, size, rva, _name in members:
                if offset == 0 and not rva % sec['alignment']:
                    out.starts[rva] = max(sec['alignment'], out.starts.get(rva, 1))
                if offset + size == sec['size']:
                    out.ends.add(rva + size)
            for (o1, z1, r1, n1), (o2, _z2, r2, n2) in zip(members, members[1:]):
                gap = o2 - (o1 + z1)
                between = [v for v in others if o1 < v < o2]
                if gap <= 0 or between or r2 - r1 != o2 - o1:
                    continue
                candidate = payload[o1 + z1:o2] if payload else bytes(gap)
                retail = pe.read(r1 + z1, gap)
                if retail is None or any(candidate) or retail != candidate:
                    continue
                pads.append(Range(r1 + z1, r2, 'alignment-padding',
                                  f'{unit} compiler padding at 0x{r1 + z1:x}', 0))
    return out, pads


def alignment_padding(pe, rows, records: Records, sections=()):
    """Zero gaps that link alignment alone explains, as extra claims.

    ``rows`` is an image partition. The gap must be missing, zero, follow a
    whole contribution and end at the next whole contribution whose required
    alignment first admits that address.
    """
    from homm3.verify.byte_accounting import Range
    starts, ends = dict(records.starts), set(records.ends)
    for s in sections:
        if s['rva'] == '-' or not int(s['size'], 0):
            continue
        start, size = int(s['rva'], 0), int(s['size'], 0)
        starts[start] = max(int(s['alignment'], 0), starts.get(start, 1))
        ends.add(start + size)
    executable = [(s['va'], s['va'] + s['vsize']) for s in pe.sections
                  if s['name'] == '.text']
    out = []
    for row in rows:
        if row['category'] != 'missing':
            continue
        start, end = row['start'], row['end']
        if any(lo <= start < hi for lo, hi in executable):
            continue
        alignment = starts.get(end)
        if (not alignment or start not in ends or end - start >= alignment
                or end % alignment or -(-start // alignment) * alignment != end):
            continue
        payload = pe.read(start, end - start)
        if payload is None or any(payload):
            continue
        out.append(Range(start, end, 'alignment-padding',
                         f'link alignment {alignment} before 0x{end:x}', 0))
    return out
