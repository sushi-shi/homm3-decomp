"""Three-way merge of the generated match ledger during a lane rebase.

``homm3 status merge-baseline`` reads Git's stages 1/2/3; three revision
arguments read ``REV:config/match_baseline.tsv`` instead. CUR is a snapshot,
so only MAX, HIST, source hash and retail RVA establish that a side earned a
row. The result is written to the worktree for review and staging.
"""
from __future__ import annotations

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


def merge(base_text: str, main_text: str, lane_text: str) -> tuple[str, int]:
    """Return merged TSV and count of lane-owned rows; reject body rebinding."""
    _, base_lines, _ = _parts(base_text)
    header, main_lines, trailing = _parts(main_text)
    _, lane_lines, _ = _parts(lane_text)
    base, main, lane = map(status.parse_baseline,
                           (base_text, main_text, lane_text))
    output, taken = list(header), 0
    for key in sorted(main.keys() | lane.keys()):
        m, l, b = main.get(key), lane.get(key), base.get(key)
        if l is None:
            choice = "main"
        elif m is None or _earned(m) == _earned(b):
            choice = "lane"
        elif _earned(l) == _earned(b):
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
        if historical > chosen.hist:
            cols = line.split("\t")
            if len(cols) != 7:
                raise ValueError(f"{key}: cannot raise HIST in legacy row")
            cols[4] = f"{historical:.4f}"
            line = "\t".join(cols)
        output.extend(comments)
        output.append(line)
        taken += choice == "lane" and _earned(l) != _earned(b)
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
    try:
        text, taken = merge(*(_show(spec) for spec in specs))
    except ValueError as exc:
        print(f"[status] baseline merge: {exc}", file=sys.stderr)
        return 2
    status.BASELINE.write_text(text)
    print(f"[status] merged {relative}: {taken} earned row(s) from lane; "
          "review, then git add the file")
    return 0
