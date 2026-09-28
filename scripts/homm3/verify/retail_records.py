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
    from homm3.sema.coverage import system_claims
    out = Records()
    data = pe.data
    opt = struct.unpack_from('<I', data, 0x3c)[0] + 24
    sections = [dict(rva=s['va'], raw_size=s['rsize'], raw_offset=s['rptr'])
                for s in pe.sections]
    for claim in system_claims(data, sections, opt):
        start, end = claim['rva'], claim['rva'] + claim['size']
        out.add(start, end, 'structural', claim['evidence'], 0)
        if claim['evidence'].startswith('PE import hint/name') and end % 2:
            if pe.read(end, 1) == b'\0':
                out.add(end, end + 1, 'structural',
                        'PE import hint/name even-alignment pad', 0)
    section_alignment, file_alignment = struct.unpack_from('<II', data, opt + 32)
    ordered = sorted(pe.sections, key=lambda s: s['va'])
    for s, following in zip(ordered, ordered[1:] + [None]):
        # The loader's image extent: a section ends at its aligned virtual
        # size, exactly where the next section (or SizeOfImage) begins.
        end = s['va'] + max(s['vsize'], s['rsize'])
        aligned = -(-end // section_alignment) * section_alignment
        limit = following['va'] if following else struct.unpack_from('<I', data, opt + 56)[0]
        if end < aligned == limit:
            out.add(end, aligned, 'structural',
                    f"{s['name']} section-alignment tail", 0)
    for s in pe.sections:
        if s['name'] in ('.rsrc', '.reloc') or s['rsize'] <= s['vsize']:
            continue
        aligned = -(-s['vsize'] // file_alignment) * file_alignment
        tail = data[s['rptr'] + s['vsize']:s['rptr'] + s['rsize']]
        if s['rsize'] == aligned and not any(tail):
            out.add(s['va'] + s['vsize'], s['va'] + s['rsize'], 'structural',
                    f"{s['name']} raw file-alignment tail", 0)
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


def _named(model, name):
    hits = [b.rva for b in model.functions + model.data
            if b.name == name or any(a.name == name for a in b.aliases)]
    return hits[0] if len(set(hits)) == 1 else None


def _pointer_sites_admitted(view, start, words, sites):
    return all(start + 4 * w in sites for w in words)


def metadata(pe, model, sites) -> Records:
    """EH, throw and RTTI records rooted in retail references."""
    view = _View(pe)
    out = Records()
    owner = _owner_names(model)
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
        if not all(w in sites for w in words) or text_lo + offset not in sites:
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

    # --- RTTI -------------------------------------------------------------
    # A COL is rooted by the admitted word preceding its vtable; unreviewed
    # vtables still root their COL when the whole record graph validates.
    roots = sorted(site + 4 for site in sites
                   if view.rdata[0] <= site < view.rdata[1]
                   and view.target(site, view.rdata) is not None)
    for vtable in roots:
        col = view.target(vtable - 4, view.rdata)
        raw = pe.read(col, 20)
        if raw is None or struct.unpack_from('<I', raw)[0]:
            continue
        signature, _offset, _cd, td, chd = struct.unpack('<IIIII', raw)
        if signature or not type_descriptor_ok(td):
            continue
        chd -= view.base
        head = pe.read(chd, 16) if view.rdata[0] <= chd < view.rdata[1] else None
        if head is None:
            continue
        chd_signature, _attributes, bases, array = struct.unpack('<IIII', head)
        array -= view.base
        if (chd_signature or not 1 <= bases <= 64
                or not view.rdata[0] <= array < view.rdata[1] - 4 * bases):
            continue
        descriptors = [view.u32(array + 4 * i) - view.base for i in range(bases)]
        words = [col + 12, col + 16, chd + 12] + [array + 4 * i for i in range(bases)]
        words += [d for d in descriptors]
        if not all(w in sites for w in words):
            out.rejected.append((col, 'RTTI pointer word is not an admitted relocation'))
            continue
        if not all(type_descriptor_ok(view.u32(d)) for d in descriptors):
            continue
        if (vtable not in sites or not view.text[0]
                <= (view.u32(vtable) or 0) - view.base < view.text[1]):
            continue
        def decorated(descriptor):
            end = view.cstring_end(descriptor + 8)
            return pe.read(descriptor + 8, end - descriptor - 9).decode('latin-1')
        cls = decorated(td - view.base)
        out.add(vtable - 4, vtable, 'compiler-metadata',
                f'COL word of vtable {vtable:x} ({cls})', 1)
        out.add(col, col + 20, 'compiler-metadata', f'CompleteObjectLocator {cls}', 1)
        out.add(chd, chd + 16, 'compiler-metadata', f'ClassHierarchyDescriptor {cls}', 1)
        out.add(array, array + 4 * bases, 'compiler-metadata', f'BaseClassArray {cls}', 1)
        type_descriptor(td - view.base, cls)
        for d in descriptors:
            base = view.u32(d) - view.base
            type_descriptor(base, cls)
            out.add(d, d + 24, 'compiler-metadata',
                    f'BaseClassDescriptor {decorated(base)} at {d:x}', 1)
    # Any other admitted reference naming a complete type descriptor
    # (typeid, dynamic_cast and catch operands) roots it too.
    for site in sorted(sites):
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


def fp_constants(pe, sites) -> Records:
    """Read-only x87 constants in `.rdata`, sized by every retail access.

    Each admitted code reference to the address must be a direct x87 load or
    arithmetic operand of one real width. The constant's `__real@` name is
    VC6's spelling of its value, so a COMDAT emitted by candidate or library
    code for the same constant folds onto this claim.
    """
    view = _View(pe)
    text_lo, text_hi = view.text
    widths = {}
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
    out = Records()
    for target, seen in sorted(widths.items()):
        if len(seen) != 1 or None in seen:
            continue
        width = seen.pop()
        raw = pe.read(target, width)
        name = _extended80(raw)
        if not name:
            continue
        out.add(target, target + width, 'compiler-metadata',
                f'__real@{width}@{name}', 1, alignment=width, whole=True)
    return out


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
