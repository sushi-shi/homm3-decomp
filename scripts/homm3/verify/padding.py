"""Reviewed padding: every credited alignment gap is an explicit row.

Coverage no longer infers padding. A gap is accounted as `padding` only when
a reviewed row names it: data sections in config/retail/data-extents.tsv,
`.text` in config/retail/code-extents.tsv, both with category `padding`.
Padding bytes are not compared with any candidate; a gap without a row stays
`missing`. Each row records the evidence a reviewer needs:

    section    the retail section
    fill       the observed fill: 00, CC, 90 or mixed (only those values)
    preceding  the claim that ends at the gap: name | file:line | type | sizeof
    following  the claim that starts after it, in the same form
    alignment  the alignment that explains the gap
    proof      proven-end: the preceding end is fixed by pinned bytes, a
               format, code or a declared type that cannot silently grow;
               after-byte-array / after-literal / after-unknown-type /
               after-unverified: a longer retail datum would also end in
               this fill, so the row is a reviewed risk
    evidence   the generator that proposed it and its reason

The former inference passes (link alignment, compiler member padding, LINK
fill, INT3/NOP fill of non-exact bodies) are generators only:
`homm3 verify padding --propose` writes build/gen/padding_proposals.tsv and
`--write` appends the proposals to the reviewed files. Byte-compared padding
(`source-padding-exact`, `source-initializer-padding-exact`) stays a real
comparison and needs no row.

A row whose bytes an exact comparison also covers is superseded, not an
error: the comparison wins those bytes and the row is reported stale
(`--stale`, `--stale --retire`). Any other overlap remains an error.
"""
from __future__ import annotations

import bisect
import re
from pathlib import Path

from homm3.core.paths import BUILD, RETAIL
from homm3.core.tsv import read, write

DATA_EXTENTS = RETAIL / 'data-extents.tsv'
CODE_EXTENTS = RETAIL / 'code-extents.tsv'
PROPOSALS = BUILD / 'gen/padding_proposals.tsv'
FIELDS = ['rva', 'size', 'category', 'section', 'fill', 'preceding', 'following',
          'alignment', 'proof', 'evidence']
PROOFS = ('proven-end', 'after-byte-array', 'after-literal', 'after-unknown-type',
          'after-unverified')
FILLS = {'00': {0}, 'CC': {0xCC}, '90': {0x90}, 'mixed': {0, 0xCC, 0x90}}
#: Categories inference used to credit; now proposal sources only.
INFERRED = ('alignment-padding', 'linker-padding', 'source-padding-aligned')


def identity(rva):
    return f'padding@{rva:x}'


def fill_of(data):
    values = set(data)
    for name in ('00', 'CC', '90'):
        if values <= FILLS[name]:
            return name
    return 'mixed' if values <= FILLS['mixed'] else None


def rows(paths=(DATA_EXTENTS, CODE_EXTENTS)):
    out = []
    for path in paths:
        if Path(path).is_file():
            out += [dict(r, source=str(path)) for r in read(path)[2]
                    if r.get('category') == 'padding']
    return out


def claims(pe, reviewed=None):
    """Validated reviewed padding rows as coverage claims.

    A row must lie inside one retail section (data rows outside `.text`,
    code rows inside it), carry a proof class and evidence, and its bytes
    must be the recorded fill. Its content is not otherwise compared.
    """
    from homm3.verify.byte_accounting import Range
    text = pe.section('.text')
    out = []
    for r in rows() if reviewed is None else reviewed:
        rva, size = int(r['rva'], 0), int(r['size'], 0)
        end = rva + size
        where = f"{r.get('source', 'padding row')} {r['rva']}"
        section = next((s for s in pe.sections
                        if s['va'] <= rva and end <= s['va'] + max(s['vsize'], s['rsize'])), None)
        if size <= 0 or section is None or section['name'] != r.get('section'):
            raise ValueError(f'{where}: padding row is not inside section {r.get("section")}')
        in_text = text['va'] <= rva < text['va'] + text['vsize']
        source = r.get('source')
        if source and in_text != source.endswith('code-extents.tsv'):
            raise ValueError(f'{where}: .text padding belongs in code-extents.tsv, '
                             'data padding in data-extents.tsv')
        if r.get('proof') not in PROOFS or not r.get('evidence', '').strip():
            raise ValueError(f'{where}: padding row needs a proof class and evidence')
        data = pe.read(rva, size)
        if data is None or r.get('fill') not in FILLS or not set(data) <= FILLS[r['fill']]:
            raise ValueError(f'{where}: retail bytes are not the recorded {r.get("fill")} fill')
        out.append(Range(rva, end, 'padding', identity(rva), 2))
    return out


#: Byte-exact comparisons that win bytes a reviewed padding row also names:
#: the compiler's compared fill after a body, and exact source code.
EXACT = ('source-padding-exact', 'source-initializer-padding-exact',
         'source-initializer-exact', 'source-cleanup-exact')
#: Game claims whose verdict `game_bytes.verify_game` decides later; a padding
#: row beneath one is superseded only where that verdict is exact.
PENDING_VERDICT = ('game',)
EXACT_GAME = ('game-code-exact', 'game-data-exact', 'game-bss-exact')
#: Claims that may lie beneath a padding row without deciding its bytes.
UNDERLAY = ('section', 'structural')


def supersede(padding, others):
    """(kept padding claims, superseded rows) of the reviewed rows.

    A reviewed row whose bytes an exact comparison also covers is stale: the
    comparison wins those bytes and the row keeps only the rest. Game claims
    are provisionally superseded here; `confirm` requires their final verdict
    to be exact. Overlap with any other claim, or between two padding rows,
    is an error.
    """
    from homm3.verify.byte_accounting import Range
    spans = sorted((r.start, r.end, r.identity, r.category) for r in others
                   if r.category not in ('padding', *UNDERLAY))
    starts = [s[0] for s in spans]
    width = max((e - s for s, e, _i, _c in spans), default=0)
    problems, kept, stale = [], [], []
    ordered = sorted(padding, key=lambda r: (r.start, r.end))
    for a, b in zip(ordered, ordered[1:]):
        if b.start < a.end:
            problems.append(f'{a.identity} {a.start:#x}..{a.end:#x} overlaps '
                            f'padding {b.identity} {b.start:#x}..{b.end:#x}')
    for p in ordered:
        covered = []
        k = bisect.bisect_left(starts, p.start - width)
        while k < len(spans) and spans[k][0] < p.end:
            s, e, ident, category = spans[k]
            k += 1
            if not (s < p.end and p.start < e):
                continue
            if category in EXACT or category in PENDING_VERDICT:
                covered.append(dict(start=max(s, p.start), end=min(e, p.end),
                                    category=category, claim=ident))
            else:
                problems.append(f'{p.identity} {p.start:#x}..{p.end:#x} overlaps '
                                f'{category} {ident} {s:#x}..{e:#x}')
        if not covered:
            kept.append(p)
            continue
        remaining, pos = [], p.start
        for c in sorted(covered, key=lambda c: c['start']):
            if pos < c['start']:
                remaining.append((pos, c['start']))
            pos = max(pos, c['end'])
        if pos < p.end:
            remaining.append((pos, p.end))
        kept += [Range(a, b, p.category, p.identity, p.priority) for a, b in remaining]
        stale.append(dict(rva=hex(p.start), size=p.end - p.start, covered=covered,
                          covered_bytes=p.end - p.start - sum(b - a for a, b in remaining),
                          remaining=[[hex(a), b - a] for a, b in remaining]))
    if problems:
        raise ValueError('reviewed padding overlaps claims that are not exact comparisons: '
                         + '; '.join(problems[:10]))
    return kept, stale


def confirm(stale, image_rows):
    """Require every game claim that superseded a row to be exact.

    `image_rows` is the final image partition. A superseding game claim
    whose bytes are not all exact code or data makes the row an error, as
    any other overlap is. Each covered part is relabelled with its verdict.
    """
    starts = [r['start'] for r in image_rows]
    problems = []
    for row in stale:
        for c in row['covered']:
            if c['category'] not in PENDING_VERDICT:
                continue
            k = max(0, bisect.bisect_right(starts, c['start']) - 1)
            verdicts = set()
            while k < len(image_rows) and image_rows[k]['start'] < c['end']:
                if image_rows[k]['end'] > c['start']:
                    verdicts.add(image_rows[k]['category'])
                k += 1
            if verdicts and verdicts <= set(EXACT_GAME):
                c['category'] = min(verdicts)
            else:
                problems.append(f"{identity(int(row['rva'], 16))} {c['start']:#x}..{c['end']:#x} "
                                f"overlaps {c['claim']} ({', '.join(sorted(verdicts)) or '-'})")
    if problems:
        raise ValueError('reviewed padding overlaps game bytes that are not exact: '
                         + '; '.join(problems[:10]))
    return stale


def retire(stale, paths=(DATA_EXTENTS, CODE_EXTENTS)):
    """Delete fully superseded rows; trim partially superseded ones.

    A partially covered row keeps its uncovered remainder as reviewed
    padding with the same evidence. Returns (deleted, trimmed, bytes retired).
    """
    by_rva = {int(r['rva'], 16): r for r in stale}
    deleted = trimmed = retired = 0
    for path in paths:
        if not Path(path).is_file():
            continue
        banner, header, existing = read(path)
        out, changed = [], False
        for r in existing:
            hit = by_rva.get(int(r['rva'], 0)) if r.get('category') == 'padding' else None
            if hit is None or int(r['size'], 0) != hit['size']:
                out.append(r)
                continue
            changed = True
            retired += hit['covered_bytes']
            if hit['remaining']:
                trimmed += 1
            else:
                deleted += 1
            out += [dict(r, rva=start, size=str(size)) for start, size in hit['remaining']]
        if changed:
            write(path, banner, header, out)
    return deleted, trimmed, retired


# ------------------------------------------------------------- proposals ---

def proof(identity_, category, type_text):
    """The proof class of a gap after a claim (see module docstring)."""
    from homm3.verify import game_bytes as gb
    if category in gb.FIXED_SIZE:
        return 'proven-end'
    if category in ('missing', gb.GAME_DATA_UNVERIFIED, gb.GAME_DATA_MISMATCH,
                    'section', 'overlap', 'padding', *INFERRED) or not category:
        return 'after-unverified'
    if identity_.startswith(('??_C@', 'source literal ', 'pushed literal ')) or \
            '$data$' in identity_:
        return 'after-literal'
    proven, _why = gb.end_proven(identity_, category, type_text)
    if proven:
        return 'proven-end'
    if type_text and gb.byte_array(type_text):
        return 'after-byte-array'
    return 'after-unknown-type'


def explaining_alignment(start, end):
    """The smallest power of two that places `end` and exceeds the gap."""
    size, alignment = end - start, 1
    while alignment <= 4096:
        if not end % alignment and size < alignment:
            return str(alignment)
        alignment *= 2
    return 'unexplained'


def _generator_alignment(text):
    m = re.search(r'link alignment (\d+)', text)
    return m.group(1) if m else None


def _claim_text(row, describe):
    if row is None:
        return '-'
    name, category, type_text, where, size = describe(row)
    return ' | '.join(x for x in (name or category, where, type_text,
                                  f'sizeof {size:#x}' if size else '') if x)


def propose(inferred, image_rows, pe, describe):
    """Candidate reviewed rows from the inference generators.

    `inferred` are the generators' Ranges (image rvas); only bytes the final
    image partition still leaves `missing` are proposed. `describe(row)`
    returns (name, category, type, file:line, size) of a partition row.
    """
    starts = [r['start'] for r in image_rows]
    out = []
    seen = set()
    for r in sorted(inferred, key=lambda r: (r.start, r.end)):
        k = bisect.bisect_right(starts, r.start) - 1
        while 0 <= k < len(image_rows) and image_rows[k]['start'] < r.end:
            row = image_rows[k]
            k += 1
            if row['category'] != 'missing':
                continue
            a, b = max(r.start, row['start']), min(r.end, row['end'])
            if a >= b or (a, b) in seen:
                continue
            seen.add((a, b))
            data = pe.read(a, b - a)
            fill = fill_of(data or b'')
            if fill is None:
                continue
            index = bisect.bisect_right(starts, a) - 1
            before = image_rows[index - 1] if index > 0 and image_rows[index - 1]['end'] == a \
                and a == row['start'] else None
            after = image_rows[index + 1] if index + 1 < len(image_rows) and \
                image_rows[index + 1]['start'] == b and b == row['end'] else None
            section = next(s['name'] for s in pe.sections
                           if s['va'] <= a < s['va'] + max(s['vsize'], s['rsize']))
            name, category, type_text, _w, _s = describe(before) if before else ('', '', '', '', 0)
            out.append(dict(rva=hex(a), size=str(b - a), category='padding', section=section,
                            fill=fill, preceding=_claim_text(before, describe),
                            following=_claim_text(after, describe),
                            alignment=_generator_alignment(r.identity)
                            or explaining_alignment(a, b),
                            proof=proof(name, category, type_text) if before else
                            'after-unverified',
                            evidence=f'{r.category}: {r.identity}'))
    return out


def write_proposals(proposals):
    write(PROPOSALS, ['# GENERATED by homm3.verify.padding: gaps the inference '
                      'generators would credit. Review, then `homm3 verify padding '
                      '--propose --write`.'], FIELDS, [[p[f] for f in FIELDS] for p in proposals])


def merge(proposals, pe):
    """Append proposals to the reviewed files (data or code by section)."""
    for path, keep in ((DATA_EXTENTS, lambda p: p['section'] != '.text'),
                       (CODE_EXTENTS, lambda p: p['section'] == '.text')):
        banner, header, existing = read(path)
        fields = header + [f for f in FIELDS if f not in header]
        taken = {int(r['rva'], 0) for r in existing}
        added = [p for p in proposals if keep(p) and int(p['rva'], 0) not in taken]
        merged = sorted(existing + added, key=lambda r: int(r['rva'], 0))
        write(path, banner, fields, [[r.get(f, '-') or '-' for f in fields] for r in merged])
        print(f'[padding] {path.name}: {len(added)} row(s) added')


def stale_rows():
    """Superseded rows from the latest accounting, recomputed when stale.

    build/gen/data_coverage.json is reused only when it is newer than both
    reviewed files; otherwise the byte accounting runs again.
    """
    import json
    from homm3.core.paths import BUILD as build
    coverage = build / 'gen/data_coverage.json'
    inputs = [Path(x) for x in (DATA_EXTENTS, CODE_EXTENTS) if Path(x).is_file()]
    if coverage.is_file() and all(coverage.stat().st_mtime > x.stat().st_mtime
                                  for x in inputs):
        doc = json.loads(coverage.read_text())
        stale = doc.get('padding', {}).get('stale')
        if stale is not None:
            return stale['rows']
    from homm3.verify.byte_accounting import report
    return report()['padding']['stale']['rows']


def main(argv=None) -> int:
    import argparse
    import collections
    ap = argparse.ArgumentParser(prog='homm3 verify padding', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--propose', action='store_true',
                    help='run the byte accounting and write padding proposals')
    ap.add_argument('--write', action='store_true',
                    help='with --propose: append the proposals to the reviewed files')
    ap.add_argument('--proof', choices=PROOFS, help='list reviewed rows of one proof class')
    ap.add_argument('--stale', action='store_true',
                    help='list reviewed rows an exact comparison supersedes')
    ap.add_argument('--retire', action='store_true',
                    help='with --stale: delete superseded rows (trim partial ones)')
    a = ap.parse_args(argv)
    if a.retire and not a.stale:
        ap.error('--retire needs --stale')
    if a.stale:
        stale = stale_rows()
        for r in stale:
            by = ', '.join(sorted({c['category'] for c in r['covered']}))
            left = ' '.join(f'{x}+{n}' for x, n in r['remaining'])
            print(f"{r['rva']:>10} {r['size']:>4} {r.get('source', ''):17} "
                  f"{r['covered_bytes']:>4} B by {by}" + (f'; keeps {left}' if left else ''))
        print(f"[padding] {len(stale)} stale row(s), "
              f"{sum(r['covered_bytes'] for r in stale):,} B superseded")
        if a.retire:
            deleted, trimmed, retired = retire(stale)
            print(f'[padding] retired {deleted} row(s), trimmed {trimmed}, {retired:,} B')
        return 0
    if a.propose:
        from homm3.verify.byte_accounting import report
        from homm3.core.pe import image
        doc = report()
        proposals = doc['padding']['proposals']
        print(f'[padding] {len(proposals)} proposal(s) -> {PROPOSALS}')
        if a.write:
            merge(proposals, image())
        return 0
    reviewed = rows()
    counts, sizes = collections.Counter(), collections.Counter()
    for r in reviewed:
        counts[r['proof']] += 1
        sizes[r['proof']] += int(r['size'], 0)
    for p in PROOFS:
        print(f'{p:20} {counts[p]:6} row(s) {sizes[p]:8,} B')
    if a.proof:
        for r in reviewed:
            if r['proof'] == a.proof:
                print(f"{r['rva']:>10} {r['size']:>4} {r['section']:7} {r['fill']:5} "
                      f"after {r['preceding']}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
