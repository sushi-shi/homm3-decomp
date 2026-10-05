"""Three-way merge of the generated match ledger during a lane rebase.

``homm3 status merge-baseline`` reads Git's stages 1/2/3; three revision
arguments read ``REV:config/match_baseline.tsv`` instead. CUR is a snapshot,
so only MAX, HIST, source hash and retail RVA establish that a side earned a
row. The result is written to the worktree for review and staging.
"""
from __future__ import annotations

import re
import subprocess
import sys

from homm3.match import status


def _parts(contents: str) -> tuple[list[str], dict, list[str]]:
    # Keep evidence comments attached to their following row. The generated
    # header comes from main; comments for a lane-owned row travel with it.
    header, rows, pending = [], {}, []
    seen = False
    for line in contents.splitlines():
        if not seen and line.startswith("#"):
            header.append(line)
        elif not line or line.startswith("#"):
            pending.append(line)
        else:
            seen = True
            cols = line.split("\t")
            if len(cols) not in (3, 6, 7):
                raise ValueError(f"malformed match baseline row: {line!r}")
            key = cols[0], cols[1]
            if key in rows:
                raise ValueError(f"duplicate match baseline row: {key}")
            rows[key] = (line, pending)
            pending = []
    status.parse_baseline(contents)
    return header, rows, pending


def _earned(row: status.MatchRow | None):
    return None if row is None else (row.max, row.hist, row.src_hash, row.rva)


_MARKER = re.compile(r"^(<{7,}|\|{7,}|={7,}|>{7,})(?: .*)?$")


def _pick_side(lines: list[str], side: int) -> list[str]:
    """Resolve every conflict hunk to one side (0 = first, 1 = second).

    Git's recursive strategy writes a virtual merge base that can itself
    contain conflict hunks, with markers lengthened per nesting level
    (`<<<<<<<<< Temporary merge branch 1`). A hunk closes only at markers
    of its own length, so nested hunks resolve recursively.
    """
    out, i = [], 0
    while i < len(lines):
        match = _MARKER.match(lines[i])
        if not match or match.group(1)[0] != "<":
            out.append(lines[i])
            i += 1
            continue
        size = len(match.group(1))
        sections, current, i = [[], [], []], 0, i + 1
        while i < len(lines):
            inner = _MARKER.match(lines[i])
            if inner and len(inner.group(1)) == size:
                kind = inner.group(1)[0]
                if kind == "|":
                    current = 1
                elif kind == "=":
                    current = 2
                elif kind == ">":
                    break
                else:
                    raise ValueError(f"nested conflict marker of equal size: {lines[i]!r}")
            else:
                sections[current].append(lines[i])
            i += 1
        else:
            raise ValueError(f"unterminated conflict hunk {match.group(0)!r}")
        i += 1
        out.extend(_pick_side(sections[0] if side == 0 else sections[2], side))
    return out


def base_variants(contents: str) -> list[str]:
    """The merge base as one or more conflict-free ledgers.

    A recursive merge's virtual base keeps conflicts between its own merge
    bases; each side of those hunks is a real ancestor ledger, so a row
    equal to either side is unchanged since the base.
    """
    lines = contents.splitlines()
    if not any(_MARKER.match(line) for line in lines):
        return [contents]
    return ["\n".join(_pick_side(lines, side)) + "\n" for side in (0, 1)]


def merge(base_text: str, main_text: str, lane_text: str, *,
          warnings: list[str] | None = None) -> tuple[str, int]:
    """Return merged TSV and count of lane-owned rows; reject body rebinding.

    A row one side deleted (a retired or renamed label) while the other left
    it unchanged since the base stays deleted; the retail RVA, not the label,
    identifies the body, so the merged ledger has one row per RVA.
    """
    bases = [status.parse_baseline(text) for text in base_variants(base_text)]
    header, main_lines, trailing = _parts(main_text)
    _, lane_lines, _ = _parts(lane_text)
    main, lane = map(status.parse_baseline, (main_text, lane_text))

    def unchanged(key, row) -> bool:
        return any(_earned(row) == _earned(base.get(key)) for base in bases)

    chosen_rows, taken = {}, 0
    for key in sorted(main.keys() | lane.keys()):
        m, l = main.get(key), lane.get(key)
        if l is None:
            if unchanged(key, m):
                continue  # the lane retired this label
            choice = "main"
        elif m is None:
            if unchanged(key, l):
                continue  # main retired this label
            choice = "lane"
        elif unchanged(key, m):
            choice = "lane"
        elif unchanged(key, l):
            choice = "main"
        else:
            if m.rva is not None and l.rva is not None and m.rva != l.rva:
                raise ValueError(f"{key}: conflicting retail RVA bindings "
                                 f"0x{m.rva:x} and 0x{l.rva:x}")
            choice = "lane" if l.max >= m.max else "main"
        line, comments = (lane_lines if choice == "lane" else main_lines)[key]
        chosen = lane[key] if choice == "lane" else main[key]
        historical = max(chosen.hist, *(row.hist for row in (m, l)
                                        if row is not None))
        chosen_rows[key] = (line, comments, chosen, historical)
        taken += choice == "lane" and not unchanged(key, l)

    # One row per retail body. A label renamed on one side and changed on
    # the other leaves both labels; the label absent from the base is the
    # rename and keeps the old label's HIST.
    by_rva = {}
    for key, (_line, _comments, row, _hist) in chosen_rows.items():
        if row.rva is not None:
            by_rva.setdefault(row.rva, []).append(key)
    for rva, keys in by_rva.items():
        if len(keys) < 2:
            continue
        renamed = [key for key in keys
                   if not any(key in base for base in bases)]
        if len(renamed) != 1:
            # Both sides renamed the body (or the inputs already carried a
            # stale label). The build's current label decides at the next
            # checkpoint, which retires the others by RVA.
            if warnings is not None:
                warnings.append(f"retail RVA 0x{rva:x} keeps several labels "
                                f"until the next checkpoint: "
                                + ", ".join(f"{u} {f}" for u, f in sorted(keys)))
            continue
        keep = renamed[0]
        line, comments, row, historical = chosen_rows[keep]
        for key in keys:
            if key != keep:
                historical = max(historical, chosen_rows.pop(key)[3])
        chosen_rows[keep] = (line, comments, row, historical)

    output = list(header)
    for key in sorted(chosen_rows):
        line, comments, chosen, historical = chosen_rows[key]
        if historical > chosen.hist:
            cols = line.split("\t")
            if len(cols) != 7:
                raise ValueError(f"{key}: cannot raise HIST in legacy row")
            cols[4] = f"{historical:.4f}"
            line = "\t".join(cols)
        output.extend(comments)
        output.append(line)
    output.extend(trailing)
    text = "\n".join(output) + "\n"
    status.parse_baseline(text)  # validate CUR <= MAX <= HIST after merging
    return text, taken


def _show(spec: str) -> str:
    result = subprocess.run(["git", "show", spec], capture_output=True, text=True,
                            cwd=status.common.HOMM3_DIR)
    if result.returncode:
        raise ValueError(f"cannot read {spec}: {result.stderr.strip()}")
    return result.stdout


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    relative = status.BASELINE.relative_to(status.common.HOMM3_DIR)
    if not argv:
        specs = [f":{stage}:{relative}" for stage in (1, 2, 3)]
    elif len(argv) == 3:
        specs = [f"{ref}:{relative}" for ref in argv]
    else:
        print("usage: homm3 status merge-baseline [BASE MAIN LANE]",
              file=sys.stderr)
        return 2
    warnings = []
    try:
        text, taken = merge(*(_show(spec) for spec in specs), warnings=warnings)
    except ValueError as exc:
        print(f"[status] baseline merge: {exc}", file=sys.stderr)
        return 2
    for warning in warnings:
        print(f"[status] baseline merge: {warning}", file=sys.stderr)
    status.BASELINE.write_text(text)
    print(f"[status] merged {relative}: {taken} earned row(s) from lane; "
          "review, then git add the file")
    return 0
