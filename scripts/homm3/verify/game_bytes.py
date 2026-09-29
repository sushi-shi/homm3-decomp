"""Prove that every game-claimed byte is reproduced by its source definition.

`byte_accounting` first files every source-claimed byte as `game`. This pass
replaces that ownership with verdicts, in the image and file domains alike:

    game-code-exact        a claimed function whose CURRENT compiled bytes equal
                           retail (CUR 100% in the objdiff report)
    game-code-unverified   any other claimed game function byte
    game-data-exact        a data byte the claim's own candidate definition
                           emits at that address with the retail value
    game-bss-exact         zero-initialized storage whose candidate definition
                           has the retail size, is placed at a compatible
                           alignment and is zero in the image
    game-data-mismatch     a byte the candidate definition emits with a
                           different value (pointers: a different referent or
                           addend), byte-precise
    game-data-pending-function
                           exception records (FuncInfo, unwind map) of a
                           claimed function that is not yet byte-exact: the
                           function's score tracks them, and they compare as
                           soon as it matches; not a finish-line category
    game-data-unverified   every other game-claimed data byte, with a reason

Padding is accounted only through reviewed rows (`homm3.verify.padding`);
this pass also turns the padding generators' ranges into review proposals.

Invariant: a retail data byte is exact only if a compiled candidate
definition emits that byte at that address. Retail-side extents, labels,
slot sizes and zero-ness never cover a byte on their own.

Every run in a finish-line category (missing, game-data-unverified,
game-data-mismatch) is written to
build/gen/data_worklist.{json,tsv} with the neighbouring claims and a
diagnosis; `homm3 verify data-worklist` lists it. Pending-function runs are
listed there too, and per function in build/gen/pending_function_records.json
(`homm3 status functions` shows them).
"""
from __future__ import annotations

import bisect
from collections import defaultdict
from dataclasses import dataclass
import json
import re
from pathlib import Path

from homm3.core import msvc_names
from homm3.core.paths import BUILD

GAME_CODE_EXACT, GAME_CODE_UNVERIFIED = 'game-code-exact', 'game-code-unverified'
GAME_DATA_EXACT, GAME_BSS_EXACT = 'game-data-exact', 'game-bss-exact'
GAME_DATA_MISMATCH, GAME_DATA_UNVERIFIED = 'game-data-mismatch', 'game-data-unverified'
GAME_DATA_PENDING_FUNCTION = 'game-data-pending-function'
PADDING_PROVISIONAL = 'padding-provisional'
GAME_CATEGORIES = (GAME_CODE_EXACT, GAME_CODE_UNVERIFIED, GAME_DATA_EXACT,
                   GAME_BSS_EXACT, GAME_DATA_MISMATCH, GAME_DATA_UNVERIFIED,
                   GAME_DATA_PENDING_FUNCTION)
#: Categories that must reach zero (post-link residue excepted).
FINISH_LINE = ('missing', GAME_DATA_UNVERIFIED, GAME_DATA_MISMATCH)
#: Worklist categories: the finish line plus records pending their function.
WORKLIST_CATEGORIES = FINISH_LINE + (GAME_DATA_PENDING_FUNCTION,)
#: EH record prefixes (eh_records): the owning function follows the prefix.
EH_RECORD_PREFIXES = ('__ehfuncinfo$', '__ehunwindmap$')
#: eh_records' reason when the owner's body still differs from retail.
PENDING_FUNCTION_REASON = 'owning function is not yet byte-exact'
PENDING = BUILD / 'gen/pending_function_records.json'
#: Accounted fill that a claim's own evidence may re-attribute.
PADDING = ('padding', 'alignment-padding', 'linker-padding')
#: Categories whose contributions have a size fixed by pinned bytes or a
#: format: a library COFF section, a PE import record, compiler metadata.
FIXED_SIZE = ('library-runtime', 'library-vendor', 'compiler-metadata',
              'compiler-generated', 'linker-import', 'import-structure',
              'import-thunk', 'structural', 'source-initializer-exact',
              'source-cleanup-exact', GAME_CODE_EXACT, GAME_CODE_UNVERIFIED)
#: Model function channels coverage files as game code.
GAME_CODE_CHANNELS = ('src', 'src_compgen', 'src_dyninit')
#: Retail-access findings (`homm3.verify.data_access`) that dispute an extent.
EXTENT_ACCESS_FINDINGS = ('shortfall', 'undercount', 'adjacent', 'stride')
WORKLIST = BUILD / 'gen/data_worklist'
BYTE_TYPES = {'char', 'signed char', 'unsigned char', 'bool', 'BYTE', 'byte',
              'unsigned __int8', '__int8', 'CHAR', 'UCHAR'}


@dataclass(frozen=True)
class Status:
    """A byte run's verdict: a category and, when not exact, its reason."""
    start: int
    end: int
    category: str
    reason: str = ''


# --------------------------------------------------------- definitions ---

def definition_extent(size, offset, section_size, following, comdat, alignment,
                      payload=b''):
    """(verdict, definition size) of a claimed extent against its definition.

    The candidate datum at `offset` owns the bytes up to the next datum of
    its section, or the section end. A claim longer than that reads a
    neighbour (`beyond-candidate-definition`). A shorter claim is whole only
    when the remainder is the compiler's member padding: zero, shorter than
    the section alignment, and never inside a single-datum COMDAT;
    otherwise the candidate defines more than the claim
    (`candidate-larger-than-extent`).
    """
    end = following if following is not None else section_size
    room = end - offset
    if size > room:
        return 'beyond-candidate-definition', room
    gap = room - size
    if not gap:
        return 'ok', size
    tail = payload[offset + size:end] if payload else b''
    if comdat or gap >= alignment or any(tail):
        return 'candidate-larger-than-extent', room
    return 'ok', size


def alignment_bound(offset, size, section_alignment):
    """A multiple of the member's required alignment.

    alignof(T) divides sizeof(T), the member's offset in its section and the
    section's alignment, so it divides their common power of two.
    """
    bound = section_alignment or 1
    for value in (offset, size):
        if value:
            bound = min(bound, value & -value)
    return bound


def type_alignment(layout, node, depth=0):
    """MSVC i386 alignment of a layout-oracle type node, or None if unknown."""
    node = layout.node(node) if layout is not None else None
    if not node or depth > 16:
        return None
    kind = node.get('k')
    if kind == 'prim':
        return min(node.get('sz') or 0, 8) or None
    if kind == 'ptr':
        return 4
    if kind == 'arr':
        return type_alignment(layout, node.get('el'), depth + 1)
    if kind == 'rec':
        found = [4] if node.get('poly') else []
        for _off, _name, ref in node.get('m', ()):
            a = type_alignment(layout, ref, depth + 1)
            if a is None:
                return None
            found.append(a)
        return max(found, default=1)
    return None


def judge_alignment(comparisons, layout):
    """Set `aligned` on uninitialized-storage comparisons.

    A COMMON's placement was already judged against LINK's COMMON rule.
    Otherwise the retail address must be a multiple of the bound
    `alignment_bound` derives, or of the declared type's alignment from the
    layout oracle. An unknown alignment leaves `aligned` None (never exact).
    """
    for r in comparisons:
        if r.get('storage') != 'bss' or 'aligned' in r:
            continue
        bound = r.get('alignment_bound')
        if bound and not r['rva'] % bound:
            r['aligned'], r['alignment'] = True, bound
            continue
        var = layout.var(r.get('unit') or '', r.get('symbol') or r['name']) if layout else None
        alignment = type_alignment(layout, var['t']) if var else None
        if alignment is None and r['size'] == 1:
            alignment = 1
        r['aligned'] = None if alignment is None else not r['rva'] % alignment
        if alignment:
            r['alignment'] = alignment


def destination_comparisons(dynamic, pe, base_dir=None):
    """Raw comparisons of the objects that exact dynamic initializers build.

    `initializer_ranges` claims each destination with its source owner's
    size. The witness object's own definition of that symbol supplies the
    candidate extent and bytes (zero for uninitialized storage); the claim
    is then judged exactly like an enrolled datum.
    """
    from homm3.delink.coffx import Obj
    from homm3.verify.byte_accounting import _runs
    base_dir = Path(base_dir or BUILD / 'objdiff/base')
    objects, out = {}, []
    for row in dynamic.get('matches', ()):
        owner = row['owner']
        identity = f"{owner['source']}:{owner['name']}@{row['destination']:x}"
        start, size = row['destination'], owner['size']
        result = dict(unit=row.get('witness'), name=identity, rva=start, size=size,
                      type=owner.get('type', ''),
                      verdict='unavailable', different=0, unresolved=0, reason='')
        out.append(result)
        path = base_dir / f"{row.get('witness')}.obj"
        if not path.is_file():
            result['reason'] = 'witness object missing'; continue
        obj = objects.setdefault(path, Obj(path))
        hits = [(v, sn) for i, v, sn in obj.iter_symbols()
                if sn > 0 and obj.sym_name(i) == owner['symbol']]
        if len(hits) != 1:
            result['reason'] = 'candidate definition absent or ambiguous'; continue
        offset, sn = hits[0]
        sec = obj.section_table[sn - 1]
        bss = bool(sec['characteristics'] & 0x80)
        following = [v for v, _n, _c in obj.section_members(sn) if v > offset]
        payload = obj.section_payload(sn)
        extent, definition = definition_extent(
            size, offset, sec['size'], min(following) if following else None,
            bool(sec['characteristics'] & 0x1000), sec['alignment'], payload)
        result.update(storage='bss' if bss else 'data', extent=extent,
                      definition=definition, symbol=owner['symbol'],
                      alignment_bound=alignment_bound(offset, size, sec['alignment']))
        span = min(size, definition)
        if any(offset <= site < offset + span for site in obj.typed_relocations(sn)):
            result['reason'] = 'relocated initial value'; continue
        candidate = bytes(span) if bss else payload[offset:offset + span]
        retail = pe.read(start, span)
        if retail is None or len(candidate) != span:
            result['reason'] = 'incomplete raw extent'; continue
        wrong = {i for i, (a, b) in enumerate(zip(candidate, retail)) if a != b}
        result.update(verdict='mismatch' if wrong or span < size else 'exact',
                      different=len(wrong), span=span)
        if wrong:
            result['wrong'] = [[a, b, candidate[a:b][:64].hex(), retail[a:b][:64].hex()]
                               for a, b in _runs(wrong)]
    return out


# ------------------------------------------------------------ verdicts ---

def code_status(model, report, rvas):
    """{rva: (exact, reason)} for claimed functions.

    Exact means the function's current compiled bytes equal retail under the
    comparison already in force: CUR 100% in the objdiff report. MAX and
    HIST are never consulted; they describe earlier implementations.
    """
    from homm3.match.status import fn_fuzzy
    current = {}
    for key, fuzzy in fn_fuzzy(report).items():
        rva = rvas.get(key)
        if rva is not None:
            current[rva] = max(current.get(rva, -1.0), fuzzy)
    out = {}
    for b in model.functions:
        if not b.channel or not b.size:
            continue
        fuzzy = current.get(b.rva)
        out[b.rva] = ((False, 'no-comparison') if fuzzy is None else
                      (True, '') if fuzzy >= 100.0 - 1e-6 else
                      (False, f'mismatch:{fuzzy:.4f}%'))
    return out


def data_status(comparisons):
    """{identity: [Status]} from raw initializer comparisons, byte-precise.

    Inside a comparison's compared span, bytes the candidate emits with the
    retail value are exact, differing bytes are `game-data-mismatch` and
    unresolved pointer words stay unverified. Bytes past the candidate
    definition are unverified. A good byte is exact only when the extent
    agrees with the definition; uninitialized storage also needs a
    compatible alignment.
    """
    out = defaultdict(list)
    for r in comparisons:
        start, size = r['rva'], r['size']
        if not size:
            continue
        name, verdict, extent = r['name'], r['verdict'], r.get('extent', 'ok')
        if verdict == 'unavailable':
            owner = pending_owner(name, r.get('reason') or '')
            if owner:
                out[name].append(Status(start, start + size, GAME_DATA_PENDING_FUNCTION,
                                        f'pending-function:{owner}'))
                continue
            out[name].append(Status(start, start + size, GAME_DATA_UNVERIFIED,
                                    f"unavailable:{r.get('reason') or 'unknown'}"))
            continue
        span = r.get('span')
        if span is None:
            if verdict != 'exact':
                # A verdict without byte runs cannot be localized.
                reason = extent if extent != 'ok' else f"{verdict}:{r.get('reason') or ''}"
                out[name].append(Status(start, start + size, GAME_DATA_UNVERIFIED, reason))
                continue
            span = size
        bss = r.get('storage') == 'bss'
        if extent == 'candidate-larger-than-extent':
            good = Status(0, 0, GAME_DATA_UNVERIFIED, extent)
        elif bss and not r.get('aligned'):
            good = Status(0, 0, GAME_DATA_UNVERIFIED,
                          'bss-alignment-unknown' if r.get('aligned') is None
                          else 'bss-misaligned')
        else:
            good = Status(0, 0, GAME_BSS_EXACT if bss else GAME_DATA_EXACT)
        marks = {}
        for a, b, *_ in r.get('wrong', ()):
            for i in range(a, b):
                marks[i] = (GAME_DATA_MISMATCH, 'mismatch')
        for a, b in r.get('unknown', ()):
            for i in range(a, b):
                marks.setdefault(i, (GAME_DATA_UNVERIFIED, 'unresolved-pointer'))
        runs = []
        for i in range(span):
            mark = marks.get(i, (good.category, good.reason))
            if runs and runs[-1][2:] == mark:
                runs[-1][1] = i + 1
            else:
                runs.append([i, i + 1, *mark])
        for a, b, category, reason in runs:
            out[name].append(Status(start + a, start + b, category, reason))
        if span < size:
            out[name].append(Status(start + span, start + size, GAME_DATA_UNVERIFIED,
                                    'beyond-candidate-definition'))
    return out


_RANK = {GAME_DATA_EXACT: 0, GAME_BSS_EXACT: 0, GAME_DATA_MISMATCH: 1,
         GAME_DATA_PENDING_FUNCTION: 2, GAME_DATA_UNVERIFIED: 2}


def pending_owner(name, reason):
    """The owning function of an EH record whose comparison waits on it."""
    if not reason.startswith(PENDING_FUNCTION_REASON):
        return None
    for prefix in EH_RECORD_PREFIXES:
        if name.startswith(prefix):
            return name[len(prefix):]
    return None


def pending_functions(rows):
    """{owner: pending bytes} from GAME_DATA_PENDING_FUNCTION runs."""
    out = defaultdict(int)
    for row in rows:
        if row['category'] == GAME_DATA_PENDING_FUNCTION:
            owner = (row.get('reason') or '').removeprefix('pending-function:')
            out[owner] += row['end'] - row['start']
    return dict(sorted(out.items()))


def _identity_status(identity, start, end, statuses, labels):
    """Status runs covering one partition row owned by `identity`."""
    if identity.startswith('source literal '):
        # retail_records.source_literals: the source literal's own bytes and
        # NUL were compared with retail at the claimed address.
        return [Status(start, end, GAME_DATA_EXACT)]
    if identity.startswith('game:COMMON '):
        # library_code: a game COMMON whose declared size, alignment and zero
        # fill were verified against the image.
        return [Status(start, end, GAME_BSS_EXACT)]
    default = ('reference-only-extent' if identity.startswith('pushed literal ') else
               'label-only-extent' if identity in labels else 'no-comparison')
    own = statuses.get(identity, ())
    cuts = sorted({start, end} | {x.start for x in own if start < x.start < end}
                  | {x.end for x in own if start < x.end < end})
    out = []
    for a, b in zip(cuts, cuts[1:]):
        cover = [x for x in own if x.start <= a and b <= x.end]
        pick = min(cover, key=lambda x: _RANK[x.category]) if cover else Status(
            a, b, GAME_DATA_UNVERIFIED, default)
        out.append(Status(a, b, pick.category, pick.reason))
    return out


def game_runs(rows, *, text, functions, code, data, labels=frozenset(), disputes=()):
    """Split every `game` partition row (image rvas) into GAME_* runs.

    `functions` maps a code identity to its claimed (rva, size) spans and
    `code` maps a function rva to (exact, reason). `disputes` are
    (start, end, reason) extent violations: every byte in them is
    unverified, whatever a comparison said.
    """
    lo, hi = text
    spans = sorted(disputes)
    out = []
    for row in rows:
        if row['category'] != 'game':
            continue
        start, end = row['start'], row['end']
        identity = row['owners'][0] if row['owners'] else ''
        if lo <= start < hi:
            owner = next((rva for rva, size in functions.get(identity, ())
                          if rva <= start < rva + size), None)
            exact, reason = code.get(owner, (False, 'no-comparison'))
            out.append(Status(start, end, GAME_CODE_EXACT if exact else GAME_CODE_UNVERIFIED,
                              reason))
            continue
        for run in _identity_status(identity, start, end, data, labels):
            cuts = sorted({run.start, run.end}
                          | {a for a, _b, _r in spans if run.start < a < run.end}
                          | {b for _a, b, _r in spans if run.start < b < run.end})
            for a, b in zip(cuts, cuts[1:]):
                hit = next((r for s0, e0, r in spans if s0 <= a and b <= e0), None)
                out.append(Status(a, b, GAME_DATA_UNVERIFIED, hit) if hit
                           else Status(a, b, run.category, run.reason))
    return out


def _emit(out, row):
    if (out and out[-1]['end'] == row['start'] and
            all(out[-1].get(k) == row.get(k) for k in ('category', 'owners', 'reason'))):
        out[-1]['end'] = row['end']
        out[-1]['size'] += row['size']
    else:
        out.append(row)


def apply_runs(rows, runs, categories=('game',), translate=None, default=None):
    """Replace rows of `categories` by the runs covering them.

    `translate` maps a row's offsets to image rvas (the file domain). Bytes
    of such a row that no run covers take `default` (category, reason).
    """
    runs = sorted(runs, key=lambda r: r.start)
    starts = [r.start for r in runs]
    out = []
    for row in rows:
        if row['category'] not in categories:
            _emit(out, dict(row)); continue
        base = translate(row['start']) if translate else row['start']
        shift = base - row['start']
        a, b = base, base + row['size']
        k = max(0, bisect.bisect_right(starts, a) - 1)
        pos = a
        while pos < b:
            while k < len(runs) and runs[k].end <= pos:
                k += 1
            if k >= len(runs) or runs[k].start > pos:
                stop = min(b, runs[k].start if k < len(runs) else b)
                piece = dict(row, start=pos - shift, end=stop - shift, size=stop - pos)
                if default:
                    piece.update(category=default[0], reason=default[1])
                _emit(out, piece)
                pos = stop; continue
            stop = min(b, runs[k].end)
            _emit(out, dict(row, start=pos - shift, end=stop - shift, size=stop - pos,
                            category=runs[k].category, reason=runs[k].reason))
            pos = stop
    return out


# ------------------------------------------------------ extent evidence ---

def extent_violations(comparisons, model, findings=()):
    """Extent-versus-definition findings for every game data claim.

    Returns (violations, disputes, padding spans). A dispute makes every
    byte of the named extent unverified; a padding span re-attributes
    alignment padding to the claim whose definition or retail accesses
    reach it.
    """
    violations, disputes, spans = [], [], []
    for r in comparisons:
        extent = r.get('extent', 'ok')
        if extent == 'ok':
            continue
        start, size, definition = r['rva'], r['size'], r.get('definition', 0)
        violations.append(dict(kind=extent, name=r['name'], unit=r.get('unit'), rva=start,
                               extent=size, definition=definition))
        if extent == 'candidate-larger-than-extent' and definition > size:
            spans.append((start + size, start + definition, extent, r['name']))
    claims = {b.rva: b for b in model.data if b.channel and b.size}
    starts = sorted(claims)
    for category, _severity, rva, name, address, detail, _evidence in findings:
        if category in EXTENT_ACCESS_FINDINGS:
            b = claims.get(rva)
            size = b.size if b else 0
            violations.append(dict(kind=f'retail-access-{category}', name=name, rva=rva,
                                   extent=size, detail=detail))
            if size:
                disputes.append((rva, rva + size, f'retail-access-{category}'))
        elif category == 'unclaimed' and isinstance(address, int):
            # Retail touches bytes directly past the preceding claim's end.
            k = bisect.bisect_right(starts, address) - 1
            if k < 0:
                continue
            b = claims[starts[k]]
            touched = re.match(r'(\d+) B', detail)
            width = int(touched.group(1)) if touched else 1
            if b.rva + b.size <= address:
                spans.append((address, address + width, 'retail-access-past-extent', b.name))
                violations.append(dict(kind='retail-access-past-extent', name=b.name,
                                       rva=b.rva, extent=b.size, address=address,
                                       detail=detail))
    return violations, disputes, spans


def reassign_padding(rows, spans, translate=None):
    """Alignment padding that a claim's own evidence says is its datum.

    `spans` are (start, end, reason, identity) in image rvas: bytes the
    candidate definition extends over, or bytes retail accesses through the
    claim past its extent. Padding there is not padding; those bytes become
    `game-data-unverified`. Missing bytes stay missing.
    """
    out = []
    for row in rows:
        if row['category'] not in PADDING:
            out.append(row); continue
        base = translate(row['start']) if translate else row['start']
        shift = base - row['start']
        cuts = {base, base + row['size']}
        for a, b, _r, _i in spans:
            cuts |= {x for x in (a, b) if base < x < base + row['size']}
        cuts = sorted(cuts)
        for a, b in zip(cuts, cuts[1:]):
            hit = next(((r, i) for s0, e0, r, i in spans if s0 <= a and b <= e0), None)
            piece = dict(row, start=a - shift, end=b - shift, size=b - a)
            if hit:
                piece.update(category=GAME_DATA_UNVERIFIED, reason=hit[0], owners=(hit[1],))
            out.append(piece)
    return out


# ------------------------------------------------------ claim metadata ---

_ANNOTATION = re.compile(r'\b(DATA|DATA_COMPGEN|DATA_COMPGEN_GUARD)\s*\(\s*(0x[0-9a-fA-F]+)')


def annotations(root):
    """{rva: (file:line, kind, text of the declaration line)} of DATA macros."""
    root = Path(root)
    out = {}
    for path in sorted([*root.joinpath('src').rglob('*.cpp'),
                        *root.joinpath('include').rglob('*.h')]):
        lines = path.read_text(encoding='latin-1').split('\n')
        for number, line in enumerate(lines, 1):
            for m in _ANNOTATION.finditer(line):
                rva = int(m.group(2), 16) - 0x400000
                rest = line[m.end():]
                declaration = rest.split(')', 1)[1].strip() if ')' in rest else ''
                if not declaration and number < len(lines):
                    declaration = lines[number].strip()
                out.setdefault(rva, (f'{path.relative_to(root)}:{number}', m.group(1),
                                     declaration))
    return out


def claim_types(claims):
    """{rva: declared type string} from the source claim fragments."""
    return {c.rva: c.meta.get('type', '') for c in claims
            if c.kind == 'data' and c.meta.get('type')}


_ARRAY = re.compile(r'^(.*?)\s*((?:\[\d*\])+)$')


def byte_array(type_text):
    """True when the declared type is an array of one-byte elements."""
    m = _ARRAY.match((type_text or '').strip())
    if not m:
        return False
    element = re.sub(r'\b(const|volatile|static)\b', '', m.group(1)).strip()
    return element in BYTE_TYPES


def end_proven(identity, category, type_text):
    """(proven, why) for the end of the contribution that precedes padding.

    Proven means its size is fixed by pinned bytes or a format, or by a
    declared type whose size cannot silently grow: not a byte array, string
    literal or other string-like datum, and not a claim without a
    definition. `type_text` is the source declaration's type, if any.
    """
    if category in FIXED_SIZE:
        return True, f'{category} size is fixed'
    if category in ('missing', GAME_DATA_UNVERIFIED, GAME_DATA_MISMATCH,
                    PADDING_PROVISIONAL, 'section', 'overlap'):
        return False, f'preceded by {category}'
    if identity.startswith(('??_C@', 'source literal ', 'pushed literal ')) or \
            '$data$' in identity:
        return False, 'string literal: a longer retail array ends in zeros too'
    if identity.startswith(('__ehfuncinfo$', '__ehunwindmap$', '__catchsym$',
                            '__tryblocktable$', '__unwindtable$', '__CT', '__TI',
                            '??_R', '__real@')) or \
            '$static_init_guard$' in identity or identity.startswith('game:COMMON '):
        # EH and RTTI records, FP constants (the name spells the width),
        # one-byte guards and COMDATs sized by LINK's COMMON rule.
        return True, 'size fixed by its format'
    if identity.startswith(('??_7', '??_S')):
        return True, 'vtable compared against its class'
    if not type_text:
        return False, 'declared type unknown'
    if byte_array(type_text):
        return False, f'{type_text} is a byte array'
    return True, f'declared {type_text}'


def placement_alignment(type_text, size):
    """VC6's placement alignment of a global or static in `.data`/`.bss`.

    Measured with the pinned compiler (docs/vc6/data-layout.md): an array
    is placed at max(4, its element's alignment), whatever its element
    type; any other object of 1, 2 or 4 bytes on its size, and of 8 bytes
    or more on 8 (a class of chars included). Other sizes (3, 5, 6, 7)
    are placed irregularly and give None, as does an unknown type.
    """
    text = (type_text or '').strip()
    if not text or not size:
        return None
    m = _ARRAY.match(text)
    if m:
        count = 1
        for bound in re.findall(r'\[(\d*)\]', m.group(2)):
            if not bound or not int(bound):
                return None
            count *= int(bound)
        if size % count:
            return None
        element = placement_alignment(m.group(1), size // count)
        return None if element is None else max(4, element)
    if size in (1, 2, 4):
        return size
    return 8 if size >= 8 else None


#: VC6 emits game `.data`/`.bss` sections aligned to 4, or to 8 when they
#: hold an 8-aligned object; LINK starts each contribution on that boundary.
CONTRIBUTION_ALIGNMENTS = (4, 8)


def next_symbol_bound(rva, following, following_alignment, same_compiland):
    """(lowest size, slot, effective alignment) the next symbol allows.

    Inside one section contribution VC6 packs objects without gaps beyond
    the following object's own alignment, so the buffer ends fewer than
    that many bytes before it. Where no evidence places both in one
    compiland and the following start could open another contribution
    (it is 4-aligned), LINK's contribution alignment bounds the gap too.
    """
    slot = following - rva
    alignment = following_alignment
    if not same_compiland and not following % 4:
        alignment = max([alignment] + [a for a in CONTRIBUTION_ALIGNMENTS
                                       if not following % a])
    return max(1, slot - alignment + 1), slot, alignment


def header_compilands(pe, matches, xc_a, xc_z):
    """[(lo, hi, header)] address hulls that lie inside one compiland.

    Each compiland that includes a header with dynamically initialized
    statics emits their `.CRT$XCU` entries consecutively in declaration
    order, and its `.bss` contribution is contiguous. A run of consecutive
    slots whose exact bodies initialize every such static of one header,
    in line order, therefore belongs to one compiland, and the hull of
    their destinations lies inside its contribution.
    """
    import struct
    bodies, headers = {}, defaultdict(dict)
    for row in matches:
        owner = row.get('owner') or {}
        source = owner.get('source', '')
        if row.get('verdict') != 'exact' or not source.startswith('include/'):
            continue
        bodies[row['rva']] = (source, owner['name'], row['destination'], owner.get('size') or 0)
        headers[source][owner['name']] = owner.get('line', 0)
    order = {h: sorted(names, key=lambda n: (names[n], n)) for h, names in headers.items()}
    slots = []
    for address in range(xc_a + 4, xc_z, 4):
        word = pe.read(address, 4)
        target = struct.unpack('<I', word)[0] - pe.image_base if word else None
        slots.append((address, bodies.get(target)))
    out = []
    for k, (address, body) in enumerate(slots):
        if body is None or len(order[body[0]]) < 2 or body[1] != order[body[0]][0]:
            continue
        run = slots[k:k + len(order[body[0]])]
        if [b and (b[0], b[1]) for _a, b in run] != [(body[0], n) for n in order[body[0]]]:
            continue
        out.append((min(b[2] for _a, b in run), max(b[2] + b[3] for _a, b in run), body[0]))
    return out


def size_from_slot(claims_by_rva, annotation, pe, next_start, dreamcast=None, root=None,
                   *, following=None, verified_at=None, compilands=(), touched=None):
    """Byte-array claims whose element count only fills the retail slot.

    A declaration with a literal bound (not a named constant and not sized
    by its initializer) whose extent ends at the next claim or an aligned
    boundary, with retail zeros at its end, has a size the retail slot
    allows. Proofs, in order:
    - `dreamcast-record`: a Dreamcast CodeView record of the same
      compiland types the declared or cited name as an array of exactly
      this many bytes;
    - `next-symbol`: the next claim starts at a verified address and VC6's
      placement rule leaves no room for a shorter array before it (its
      alignment is 1, or the slot minus its alignment slack is still the
      declared size);
    - `next-symbol+usage`: the remaining slack is closed by a retail access
      that touches the buffer's last bytes (`touched`).
    `following(address)` gives the (size, type) of the claim there,
    `verified_at(address)` whether that claim's first byte verifies without
    this pass, `compilands` are hulls known to lie in one compiland and
    `touched(start, end)` the end of the retail direct accesses inside a
    buffer. Returns (flagged, proven): flagged is
    [(start, end, zero-tail start, identity, type, file:line, detail)],
    detail holding the residual size `range` and the `reason` it stays
    open; proven is [dict(rva, name, size, proof, evidence)].
    """
    flagged, proven = [], []
    for rva, (identity, size, type_text) in sorted(claims_by_rva.items()):
        where, _kind, declaration = annotation.get(rva, ('', '', ''))
        if not byte_array(type_text) or not size:
            continue
        bound = re.search(r'\[([^\]]*)\]', declaration)
        if not bound or not re.fullmatch(r'\s*(0x[0-9a-fA-F]+|\d+)\s*', bound.group(1)):
            continue
        end = rva + size
        nxt = next_start(end)
        if nxt != end and end % 4:
            continue
        data = pe.read(rva, size)
        if data is None or data[-1]:
            continue
        content = len(data.rstrip(b'\0'))
        tail = rva + (content + 1 if content else 0)
        if tail >= end:
            continue
        recorded = (dreamcast_proven_sizes(where, declaration, dreamcast, root)
                    if dreamcast else set())
        if size in recorded:
            proven.append(dict(rva=rva, name=identity, size=size, proof='dreamcast-record',
                               evidence=f'Dreamcast types it as {size} bytes'))
            continue
        detail = _slot_detail(rva, size, content, nxt, recorded, following,
                              verified_at, compilands, touched)
        if detail.get('proof'):
            proven.append(dict(rva=rva, name=identity, size=size, proof=detail['proof'],
                               evidence=detail['evidence']))
            continue
        flagged.append((rva, end, tail, identity, type_text, where, detail))
    return flagged, proven


def _slot_detail(rva, size, content, nxt, recorded, following, verified_at, compilands,
                 touched):
    """The next-symbol and usage verdict for one size-from-slot claim."""
    low = content + 1 if content else 1
    if nxt is None or following is None or verified_at is None or not verified_at(nxt):
        return dict(range=[low, None], reason='the next symbol is not verified',
                    settle='verify the claim that follows, or a consumer bound')
    next_size, next_type = following(nxt) or (0, '')
    alignment = placement_alignment(next_type, next_size)
    if alignment is None or nxt % alignment:
        return dict(range=[low, nxt - rva], reason=f'the next symbol ({next_type or "?"}) '
                    'has no measured placement alignment',
                    settle='a consumer bound or a Dreamcast record')
    same = bool(nxt % 4) or any(lo <= rva and nxt < hi for lo, hi, *_ in compilands)
    lowest, slot, effective = next_symbol_bound(rva, nxt, alignment, same)
    lowest = max(lowest, low)
    used = (touched(rva, nxt) or rva) - rva if touched else 0
    detail = dict(slot=slot, next_alignment=alignment, alignment=effective,
                  same_compiland=same)
    if used > lowest:
        detail['usage'] = used
    low_bound = max(lowest, used)
    if low_bound >= slot == size:
        what = (f'{next_type} follows at +{slot:#x} (placed on {alignment})'
                + ('' if same else f', contribution alignment {effective}'))
        if lowest >= slot:
            return dict(detail, proof='next-symbol', evidence=what)
        return dict(detail, proof='next-symbol+usage',
                    evidence=f'{what}; retail touches up to +{used:#x}')
    detail['range'] = [low_bound, slot]
    contradicted = sorted(n for n in recorded if not low_bound <= n <= slot)
    if contradicted:
        detail['dreamcast'] = contradicted
    if not same and effective > alignment:
        detail['reason'] = (f'the next symbol could open another compiland\'s '
                            f'{effective}-aligned contribution')
        detail['settle'] = 'evidence that both lie in one compiland, or a consumer bound'
    else:
        detail['reason'] = f'{next_type} is placed on {alignment} bytes'
        detail['settle'] = ('a consumer bound (a length argument, a loop bound, an access '
                            'to the last bytes) or a Dreamcast record')
    return detail


def dreamcast_array_sizes():
    """{(compiland stem, name): {byte size}} of Dreamcast CodeView data
    records typed as arrays (globals, file and function statics)."""
    import struct
    from homm3.core import inputs
    symbols = inputs.dreamcast_symbols()
    records = symbols.type_records

    def array_bytes(index):
        record = records.get(index)
        if record is None or len(record) < 14:
            return None
        leaf = struct.unpack_from('<H', record, 2)[0]
        if leaf == 0x1001 and len(record) >= 8:          # LF_MODIFIER
            return array_bytes(struct.unpack_from('<I', record, 4)[0])
        if leaf != 0x1003:                                # LF_ARRAY
            return None
        size = struct.unpack_from('<H', record, 12)[0]
        if size == 0x8002 and len(record) >= 16:
            size = struct.unpack_from('<H', record, 14)[0]
        elif size in (0x8003, 0x8004) and len(record) >= 18:
            size = struct.unpack_from('<I', record, 14)[0]
        elif size >= 0x8000:
            return None
        return size
    out = defaultdict(set)
    for module, items in symbols.module_info.items():
        stem = Path(module.replace('\\', '/')).stem.lower()
        for item in items:
            if item.get('kind') in ('global', 'static') and item.get('name'):
                size = array_bytes(item.get('type_index', 0))
                if size:
                    out[(stem, item['name'])].add(size)
    return out


def dreamcast_proven_sizes(where, declaration, dreamcast, root=None):
    """Byte sizes Dreamcast attests for a declaration: its own name, or a
    name the comment block above it cites, typed in the same compiland."""
    if not where:
        return set()
    path, _, line = where.rpartition(':')
    stem = Path(path).stem.lower()
    names = set(re.findall(r'\b(\w+)\s*\[', declaration)[:1])
    try:
        from homm3.core.common import HOMM3_DIR
        lines = Path(root or HOMM3_DIR, path).read_text(encoding='latin-1').split('\n')
        number = int(line) - 1
    except (OSError, ValueError):
        lines, number = [], 0
    k = number - 1
    while k >= 0 and lines[k].lstrip().startswith('//'):
        names.update(re.findall(r'\b[A-Za-z_]\w*\b', lines[k]))
        k -= 1
    return set().union(*(dreamcast.get((stem, n), set()) for n in names)) if names else set()


# -------------------------------------------------------------- driver ---

def verify_game(pe, model, domains, comparisons, *, report=None, findings=None,
                layout=None, root=None, inferred=(), compilands=(), accesses=()):
    """Replace `game` with the GAME_* categories in both domains.

    Returns (domains, verification). Code verdicts come from the current
    objdiff report; data verdicts from the raw initializer comparisons, the
    extent-versus-definition check and the retail access audit. Zero fill
    after an unproven end becomes `padding-provisional`.
    """
    from homm3.core.common import HOMM3_DIR
    from homm3.match.status import function_rvas, load_report, REPORT
    from homm3.retail_labels import fragments
    root = Path(root or HOMM3_DIR)
    notes = []
    if report is None:
        report = load_report() if REPORT.is_file() else {}
        if not report:
            notes.append('objdiff report absent: every game function is unverified')
    if findings is None:
        try:
            from homm3.verify.data_access import analysis
            audit = analysis()
            findings, layout, accesses = audit[4], audit[0].layout, audit[1]
        except Exception as exc:  # the audit needs a built tree
            findings = ()
            notes.append(f'retail access audit unavailable: {exc}')
    judge_alignment(comparisons, layout)
    code = code_status(model, report, function_rvas())
    functions = defaultdict(list)
    for b in model.functions:
        if b.channel and b.size:
            functions[b.name].append((b.rva, b.size))
    labels = frozenset(b.name for b in model.data if b.channel == 'data_vtables')
    violations, disputes, spans = extent_violations(comparisons, model, findings)
    image = domains['image']
    starts = [r['start'] for r in image]

    def row_at(address):
        k = bisect.bisect_right(starts, address) - 1
        return image[k] if k >= 0 else None
    # A retail access past an extent matters only where no other claim
    # already owns the touched bytes.
    def open_bytes(address):
        row = row_at(address)
        return row is not None and row['category'] in ('missing', *PADDING)
    violations = [v for v in violations if v['kind'] != 'retail-access-past-extent'
                  or open_bytes(v['address'])]
    spans = [x for x in spans if x[2] != 'retail-access-past-extent' or open_bytes(x[0])]

    claims = fragments.all_claims()
    types = claim_types(claims)
    annotation = annotations(root)
    data_claims = {}
    for b in model.data:
        if b.channel and b.size:
            data_claims[b.rva] = (b.name, b.size, types.get(b.rva, ''))
    claim_starts = sorted({r['start'] for r in image
                           if r['category'] not in ('missing', *PADDING, 'section')})

    def next_start(address):
        k = bisect.bisect_left(claim_starts, address)
        return claim_starts[k] if k < len(claim_starts) else None
    try:
        dreamcast = dreamcast_array_sizes()
    except Exception as exc:  # the Dreamcast image is optional evidence
        dreamcast = {}
        notes.append(f'Dreamcast array records unavailable: {exc}')
    text = pe.section('.text')
    text_range = (text['va'], text['va'] + text['vsize'])
    status = data_status(comparisons)
    # The next-symbol bound needs the following claim's own verdict, before
    # this pass disputes any slot.
    first = sorted(game_runs(image, text=text_range, functions=functions, code=code,
                             data=status, labels=labels, disputes=disputes),
                   key=lambda r: r.start)
    first_starts = [r.start for r in first]

    def verified_at(address):
        k = bisect.bisect_right(first_starts, address) - 1
        return (k >= 0 and first[k].start <= address < first[k].end
                and first[k].category in (GAME_DATA_EXACT, GAME_BSS_EXACT))
    sizes = {r['rva']: (r['size'], r.get('type', '')) for r in comparisons
             if r.get('rva') is not None and r.get('size')}
    sizes.update({rva: (size, type_text) for rva, (_n, size, type_text) in data_claims.items()})
    direct = sorted((a.target_rva, a.end_rva) for a in accesses
                    if a.form == 'direct' and a.width)
    direct_starts = [a for a, _b in direct]

    def touched(start, end):
        lo, hi = bisect.bisect_left(direct_starts, start), bisect.bisect_left(direct_starts, end)
        return max((min(b, end) for _a, b in direct[lo:hi]), default=None)
    slots, slot_proofs = size_from_slot(
        data_claims, annotation, pe, next_start, dreamcast, root, following=sizes.get,
        verified_at=verified_at, compilands=compilands, touched=touched)
    disputes += [(tail, end, 'size-from-slot') for _s, end, tail, *_ in slots]

    runs = game_runs(image, text=text_range, functions=functions, code=code,
                     data=status, labels=labels, disputes=disputes)

    def to_rva(offset):
        for sec in pe.sections:
            if sec['rptr'] <= offset < sec['rptr'] + sec['rsize']:
                return offset - sec['rptr'] + sec['va']
        raise ValueError(f'file offset {offset:#x} is in no section')

    claim_index = defaultdict(list)
    for rva, (name, size, _t) in data_claims.items():
        claim_index[name].append((rva, size))

    out = {}
    for domain, rows, translate in (('image', image, None), ('file', domains['file'], to_rva)):
        rows = apply_runs(rows, runs, translate=translate,
                          default=(GAME_DATA_UNVERIFIED, 'no-comparison'))
        out[domain] = reassign_padding(rows, spans, translate)

    def describe(row):
        # (name, category, declared type, file:line, size) of a claim row.
        identity = row['owners'][0] if row.get('owners') else ''
        owner = next(((rva, size) for rva, size in claim_index.get(identity, ())
                      if rva <= row['start'] < rva + size), None)
        rva, size = owner or (row['start'], row['end'] - row['start'])
        where = annotation.get(rva, ('', '', ''))[0] or units.get(identity, '')
        return identity, row['category'], data_claims.get(rva, ('', 0, ''))[2], where, size
    from homm3.verify import padding as reviewed_padding
    units = unit_sources(comparisons, model)
    proposals = reviewed_padding.propose(inferred, out['image'], pe, describe)
    reviewed = reviewed_padding.rows()
    by_proof = defaultdict(lambda: [0, 0])
    for r in reviewed:
        by_proof[r['proof']][0] += 1
        by_proof[r['proof']][1] += int(r['size'], 0)
    game_fns = {b.rva for b in model.functions if b.channel in GAME_CODE_CHANNELS and b.size}
    exact_fns = {rva for rva in game_fns if code.get(rva, (False, ''))[0]}
    worklist = build_worklist(out['image'], pe, comparisons, data_claims, annotation,
                              layout, model, text_range,
                              slots={x[0]: x[6] for x in slots})
    write_worklist(worklist)
    pending = pending_functions(out['image'])
    units_of = {b.name: b.unit for b in model.functions if b.name}
    PENDING.write_text(json.dumps(
        {owner: dict(unit=units_of.get(owner, ''), bytes=size)
         for owner, size in pending.items()}, indent=1) + '\n')
    verification = dict(
        violations=violations, notes=notes,
        size_from_slot=[dict(rva=s, end=e, zero_tail=t, name=n, type=ty, source=w, **d)
                        for s, e, t, n, ty, w, d in slots],
        size_from_slot_proven=slot_proofs,
        functions=dict(exact=len(exact_fns), unverified=len(game_fns) - len(exact_fns)),
        worklist=sum(i['category'] in FINISH_LINE for i in worklist),
        pending_functions=pending,
        padding=dict(proposals=proposals,
                     reviewed={proof: dict(rows=n, bytes=b) for proof, (n, b) in by_proof.items()}))
    return out, verification


# ------------------------------------------------------------ worklist ---

def _claim_record(identity, rva, size, type_text, annotation, units=None):
    where, _kind, _declaration = annotation.get(rva, ('', '', ''))
    if not where and units:
        # No annotation (a paired literal, a compiler function): the unit
        # whose candidate object emits it.
        where = units.get(identity, '')
    return dict(name=identity, rva=rva, end=rva + size if size else None,
                size=size, type=type_text, source=where)


def unit_sources(comparisons, model):
    """{identity: source file} from each claim's emitting unit."""
    try:
        from homm3.build.configure import load_manifest
        sources = {u['unit']: u['source'] for u in load_manifest()[2]}
    except (SystemExit, OSError, KeyError):
        sources = {}
    out = {}
    for r in comparisons:
        if r.get('unit') in sources:
            out.setdefault(r['name'], sources[r['unit']])
    for b in list(model.functions) + list(model.data):
        if b.name and b.unit in sources:
            out.setdefault(b.name, sources[b.unit])
    return out


def build_worklist(rows, pe, comparisons, data_claims, annotation, layout, model,
                   text=(0, 0), slots=None):
    """One actionable row per finish-line run (image rvas, by address)."""
    units = unit_sources(comparisons, model)
    by_name = defaultdict(list)
    for r in comparisons:
        by_name[r['name']].append(r)
    claim_rows = [r for r in rows if r['category'] not in ('missing', *PADDING,
                                                           PADDING_PROVISIONAL, 'section')]
    starts = [r['start'] for r in claim_rows]
    symbols = defaultdict(list)
    for b in list(model.functions) + list(model.data):
        if b.name:
            symbols[b.rva].append(b.name)

    named = defaultdict(list)
    for rva, (name, size, type_text) in data_claims.items():
        named[name].append((rva, size, type_text))

    def claim_of(row):
        identity = row['owners'][0] if row.get('owners') else ''
        owner = next(((rva, size, t) for rva, size, t in named.get(identity, ())
                      if rva <= row['start'] < rva + size), None)
        if owner:
            return _claim_record(identity, *owner, annotation, units)
        return _claim_record(identity, row['start'], row['end'] - row['start'], '',
                             annotation, units)

    def neighbour(index):
        if 0 <= index < len(claim_rows):
            return claim_of(claim_rows[index])
        return None

    out = []
    for row in rows:
        if row['category'] not in WORKLIST_CATEGORIES:
            continue
        start, end = row['start'], row['end']
        data = pe.read(start, min(end - start, 64)) or b''
        k = bisect.bisect_right(starts, start) - 1
        before = neighbour(k) if k >= 0 and claim_rows[k]['end'] <= start else neighbour(k - 1)
        after = neighbour(bisect.bisect_left(starts, end))
        owner = claim_of(row) if row.get('owners') and row['category'] != 'missing' else None
        item = dict(start=start, end=end, size=end - start, category=row['category'],
                    reason=row.get('reason', ''), retail=data.hex(), owner=owner,
                    previous=before, next=after, code=text[0] <= start < text[1])
        item['diagnosis'] = diagnose(item, by_name, layout, symbols, pe, slots)
        item['source'] = ((owner or {}).get('source') or (before or {}).get('source')
                          or (after or {}).get('source') or '')
        out.append(item)
    return out


def _describe_claim(c):
    if not c:
        return 'no claim'
    parts = [c['name']]
    extra = ', '.join(x for x in (c.get('source'), c.get('type'),
                                  f"sizeof {c['size']:#x}" if c.get('size') else '') if x)
    return f"{parts[0]} ({extra})" if extra else parts[0]


def _field(layout, comparison, offset):
    if layout is None or comparison is None:
        return ''
    var = layout.var(comparison.get('unit') or '', comparison.get('symbol') or comparison['name'])
    if not var:
        return ''
    try:
        return layout.field_at(layout.node(var['t']), offset).path
    except Exception:
        return ''


def diagnose(item, by_name, layout, symbols, pe, slots=None):
    """A sentence that says what to change in the source."""
    start, end, size = item['start'], item['end'], item['size']
    before, after, owner = item['previous'], item['next'], item['owner']
    category, reason = item['category'], item['reason']
    if category == GAME_DATA_PENDING_FUNCTION:
        owner_name = reason.removeprefix('pending-function:')
        return (f"{size} EH-record byte(s) verify when {owner_name} becomes exact; the "
                f"function's score tracks them, not the finish line.")
    if category == 'missing' and item.get('code'):
        return (f"{size} byte(s) of code ({item['retail'][:32]}) between "
                f"{_describe_claim(before)} and {_describe_claim(after)} are claimed by no "
                f"function: recover the body that emits them, or record them as residue.")
    if category == 'missing':
        if before and before.get('end') == start:
            return (f"{_describe_claim(before)} ends at {start:#x}; retail has {size} more "
                    f"byte(s) ({item['retail'][:32]}) before the next claim "
                    f"{_describe_claim(after)} at {end:#x}. The preceding definition is "
                    f"{size} byte(s) short, or a definition is missing.")
        return (f"{size} unclaimed byte(s) ({item['retail'][:32]}) between "
                f"{_describe_claim(before)} and {_describe_claim(after)}; no candidate "
                f"definition emits them.")
    if category == GAME_DATA_MISMATCH:
        name = (owner or {}).get('name', '')
        lines = []
        for comparison in by_name.get(name, ()):
            base = comparison['rva']
            for a, b, candidate, retail in comparison.get('wrong', ()):
                if base + b <= start or base + a >= end:
                    continue
                field = _field(layout, comparison, a)
                lines.append(f"{name}{field} at +{a:#x}: retail {retail} candidate {candidate}")
            for p in comparison.get('pointers', ()):
                if base + p['offset'] < start or base + p['offset'] >= end:
                    continue
                actual = p.get('actual')
                target = (actual - pe.image_base) if isinstance(actual, int) else None
                named = ', '.join(symbols.get(target, [])) or (f'{actual:#x}' if actual else '?')
                lines.append(f"pointer at +{p['offset']:#x}: candidate names {p.get('symbol')} "
                             f"(+{p.get('addend', 0):#x}), retail points at {named}")
        return '; '.join(lines) or f'{name}: emitted bytes differ from retail'
    if reason == 'beyond-candidate-definition':
        return (f"{_describe_claim(owner)}: the claimed extent runs past the candidate "
                f"definition; bytes {start:#x}..{end:#x} are emitted by no definition of "
                f"this claim. Enlarge the declaration or split the claim.")
    if reason == 'size-from-slot':
        detail = (slots or {}).get((owner or {}).get('rva'), {})
        low, high = detail.get('range') or (None, None)
        bound = (f" The retail size is {low}..{high if high is not None else '?'} bytes: "
                 f"{detail.get('reason', '')}." if low is not None else '')
        if detail.get('dreamcast'):
            bound += (f" Dreamcast's {'/'.join(map(str, detail['dreamcast']))} bytes lie "
                      f"outside that bound.")
        return (f"{_describe_claim(owner)}: its bound only fills the retail slot; the zero "
                f"tail {start:#x}..{end:#x} is unproven.{bound} Settle it with "
                f"{detail.get('settle') or 'a type or consumer bound (a named constant, a loop bound, Dreamcast)'}.")
    if reason == 'candidate-larger-than-extent':
        return (f"{_describe_claim(owner)}: the candidate definition is larger than the "
                f"claimed extent; fix the claim's size.")
    if reason.startswith('retail-access'):
        return (f"{_describe_claim(owner)}: retail accesses dispute the declared extent "
                f"({reason}); check the element count and type.")
    return f"{_describe_claim(owner)}: {reason or 'unverified'}"


def write_worklist(items):
    WORKLIST.with_suffix('.json').write_text(json.dumps(items, indent=1) + '\n')
    from homm3.core.tsv import write
    write(WORKLIST.with_suffix('.tsv'),
          ['# GENERATED by homm3.verify.game_bytes: finish-line runs and records '
           'pending their function, by address.'],
          ['start', 'end', 'size', 'category', 'reason', 'source', 'owner',
           'previous', 'next', 'retail', 'diagnosis'],
          [[hex(i['start']), hex(i['end']), i['size'], i['category'], i['reason'],
            i['source'], (i['owner'] or {}).get('name', ''),
            (i['previous'] or {}).get('name', ''), (i['next'] or {}).get('name', ''),
            i['retail'][:64], i['diagnosis']] for i in items])


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog='homm3 verify data-worklist',
                                 description='List finish-line runs from the last '
                                             'byte accounting (homm3 build or '
                                             'homm3 verify data-coverage --all-bytes).')
    ap.add_argument('--unit', help='only runs whose source file is src/<unit>.cpp')
    ap.add_argument('--category', choices=WORKLIST_CATEGORIES)
    ap.add_argument('--reason', help='substring of the reason')
    ap.add_argument('--limit', type=int, default=0)
    a = ap.parse_args(argv)
    path = WORKLIST.with_suffix('.json')
    if not path.is_file():
        print(f'{path} is absent: run homm3 build first')
        return 1
    items = json.loads(path.read_text())
    if not a.category:
        pending = [i for i in items if i['category'] == GAME_DATA_PENDING_FUNCTION]
        if pending:
            print(f"({len(pending)} run(s), {sum(i['size'] for i in pending):,} B of EH records "
                  f"pending their functions: --category {GAME_DATA_PENDING_FUNCTION})")
        items = [i for i in items if i['category'] in FINISH_LINE]
    if a.unit:
        items = [i for i in items if Path(i['source'].split(':')[0]).stem == a.unit]
    if a.category:
        items = [i for i in items if i['category'] == a.category]
    if a.reason:
        items = [i for i in items if a.reason in i['reason']]
    for i in items[:a.limit or None]:
        print(f"{i['start']:#08x}..{i['end']:#08x} {i['size']:5} {i['category']:22} "
              f"{i['reason'][:40]:40} {i['source']}")
        print(f"    {i['diagnosis']}")
    print(f'{len(items)} run(s)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
