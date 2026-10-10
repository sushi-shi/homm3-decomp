"""Ledger history queries: `status hist-gap` and `status last-exact`.

Both read only committed history of the image's checkpoint ledger
(`git log -p --first-parent`) and the working tree; neither builds, banks or
writes anything.

  hist-gap     rows whose HIST exceeds MAX by at least --min-gap, each with
               the last ledger commit that banked CUR at HIST (the peak) and
               the first later commit that banked it lower (the drop).
  last-exact   for selected rows: the peak and drop commits, the commits in
               between that touched the function's source file, and (--diff)
               the function's definition at the peak against the working tree.

A ledger commit records the score of the source it was banked from, so the
change that lost a peak lies in peak..drop, usually in the drop commit itself.
"""
from __future__ import annotations

import difflib
import json
import re
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

from homm3.core import common
from homm3.match import status

COMMIT_MARK = "@@homm3-commit "
TOLERANCE = 1e-4


@dataclass
class Commit:
    sha: str
    date: str
    subject: str


@dataclass
class Timeline:
    """Every banked (CUR, best of CUR/MAX/HIST) of every ledger identity, in
    first-parent order."""
    commits: list[Commit] = field(default_factory=list)
    series: dict[object, list[tuple[int, float, float]]] = field(default_factory=dict)


def _git(*args: str, root: Path | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(["git", *args], cwd=root or common.HOMM3_DIR,
                          capture_output=True, text=True)


def _identity(cols: list[str], names: dict[tuple[str, str], int]):
    """(identity, banked CUR, best banked column) of one ledger row, or None
    for a non-row.

    Rows pair by retail RVA across renames; a legacy three-column row has no
    RVA, so its label resolves through the current ledger's names."""
    try:
        if len(cols) == 3:
            label = (cols[0], cols[1])
            value = float(cols[2])
            return names.get(label, label), value, value
        if len(cols) in (6, 7):
            identity = ((cols[0], cols[1]) if cols[5] == "-"
                        else int(cols[5], 0))
            cur = 0.0 if cols[2] == "-" else float(cols[2])
            return identity, cur, max(cur, float(cols[3]), float(cols[4]))
    except ValueError:
        pass
    return None


def parse_history(text: str, names: dict | None = None) -> Timeline:
    """Parse `git log -p --reverse -U0` output of one ledger file."""
    names = names or {}
    timeline = Timeline()
    for line in text.splitlines():
        if line.startswith(COMMIT_MARK):
            sha, date, subject = (line[len(COMMIT_MARK):].split(" ", 2) + ["", ""])[:3]
            timeline.commits.append(Commit(sha, date, subject))
            continue
        if not line.startswith("+") or line.startswith("+++") or not timeline.commits:
            continue
        parsed = _identity(line[1:].split("\t"), names)
        if parsed is None:
            continue
        identity, cur, best = parsed
        timeline.series.setdefault(identity, []).append(
            (len(timeline.commits) - 1, cur, best))
    return timeline


def load_timeline(rows: dict | None = None) -> Timeline:
    rows = status.load_baseline() if rows is None else rows
    names = {key: row.rva for key, row in rows.items() if row.rva is not None}
    relative = status.BASELINE.relative_to(common.HOMM3_DIR)
    result = _git("log", "-p", "--first-parent", "--diff-merges=first-parent",
                  "--reverse", "-U0", "--date=short",
                  f"--format={COMMIT_MARK}%h %ad %s", "--", str(relative))
    if result.returncode:
        common.die(f"cannot read the ledger history: {result.stderr.strip()}")
    return parse_history(result.stdout, names)


@dataclass
class PeakEvent:
    target: float
    peak: Commit | None
    drop: Commit | None
    drop_score: float | None
    # True when no commit banked CUR at the target and the peak is the last
    # commit whose MAX/HIST columns carried it (a peak inherited from older
    # history or a migration seed, not a measured CUR).
    inherited: bool = False


def peak_event(timeline: Timeline, identity, target: float) -> PeakEvent:
    """The last commit that banked CUR >= target, and the next one below it."""
    series = timeline.series.get(identity, [])
    for column, inherited in ((1, False), (2, True)):
        peak_index = None
        for position, entry in enumerate(series):
            if entry[column] >= target - TOLERANCE:
                peak_index = position
        if peak_index is not None:
            break
    if peak_index is None:
        return PeakEvent(target, None, None, None)
    peak = timeline.commits[series[peak_index][0]]
    if peak_index + 1 < len(series):
        commit, cur, _best = series[peak_index + 1]
        return PeakEvent(target, peak, timeline.commits[commit], cur, inherited)
    return PeakEvent(target, peak, None, None, inherited)


def gap_rows(rows: dict, *, min_gap: float = 0.05, exact_only: bool = False):
    """Ledger rows whose HIST exceeds MAX by min_gap, largest HIST first."""
    out = []
    for key, row in rows.items():
        gap = row.hist - row.max
        if gap <= 1e-9 or gap < min_gap - 1e-9:
            continue
        if exact_only and row.hist < 100.0 - TOLERANCE:
            continue
        out.append((key, row))
    out.sort(key=lambda item: (-item[1].hist, item[1].max - item[1].hist, item[0]))
    return out


def _event_record(key, row, event: PeakEvent) -> dict:
    def commit(value):
        return None if value is None else {
            "commit": value.sha, "date": value.date, "subject": value.subject}
    return {"unit": key[0], "function": key[1],
            "rva": None if row.rva is None else f"0x{row.rva:x}",
            "cur": row.cur, "max": row.max, "hist": row.hist,
            "target": event.target, "peak": commit(event.peak),
            "drop": commit(event.drop), "drop_score": event.drop_score,
            "inherited": event.inherited}


def _identity_of(key, row):
    return row.rva if row.rva is not None else key


def _format_commit(commit: Commit | None) -> str:
    return "-" if commit is None else f"{commit.sha} {commit.date} {commit.subject}"


def cmd_hist_gap(args) -> int:
    rows = status.load_baseline()
    selected = gap_rows(rows, min_gap=args.min_gap, exact_only=args.exact)
    timeline = load_timeline(rows)
    records = [_event_record(key, row, peak_event(
        timeline, _identity_of(key, row), row.hist)) for key, row in selected]
    if args.json:
        print(json.dumps(records, indent=2))
        return 0
    for record in records:
        cur = "-" if record["cur"] is None else f"{record['cur']:.4f}"
        print(f"{record['unit']}\t{record['function']}\t{record['rva'] or '-'}\t"
              f"CUR {cur} MAX {record['max']:.4f} HIST {record['hist']:.4f}")
        peak, drop = record["peak"], record["drop"]
        label = "peak* " if record["inherited"] else "peak  "
        print("    " + label + ("-" if peak is None else
                                f"{peak['commit']} {peak['date']} {peak['subject']}"))
        if drop is not None:
            print(f"    drop  {drop['commit']} {drop['date']} -> "
                  f"{record['drop_score']:.4f} {drop['subject']}")
    print(f"[status] {len(records)} row(s) with HIST - MAX >= {args.min_gap:g}"
          f"{' and HIST = 100' if args.exact else ''}; peak* = no commit banked "
          "CUR at HIST, only an inherited MAX/HIST column")
    return 0


# --- function source at a revision ---------------------------------------


def _va_pattern(rva: int) -> str:
    return rf"VA\(0x0*{common.IMAGE_BASE + rva:x}\b"


def claim_paths(rva: int, rev: str | None = None) -> list[str]:
    """Tracked source/header paths with a VA claim of this RVA at rev
    (the working tree when rev is None)."""
    args = ["grep", "-l", "-i", "-E", _va_pattern(rva)]
    if rev:
        args.append(rev)
    result = _git(*args, "--", "src", "include")
    paths = []
    for line in result.stdout.splitlines():
        paths.append(line.split(":", 1)[1] if rev and line.startswith(rev + ":") else line)
    return paths


def definition_at(text: str, rva: int, function: str) -> str | None:
    """The definition annotated with this RVA's VA claim in one file's text."""
    from homm3.retail_labels import source
    masked = source.mask_lexical_noise(text)
    va_head = source.MACRO_HEADS["VA"][0]
    for _start, end, args, _raw in source.macro_invocations(masked, va_head, text):
        if end is None or len(args) != 2 or not source.ADDR_ARG_RE.match(args[0]):
            continue
        if int(args[0], 16) - common.IMAGE_BASE != rva:
            continue
        definition = status._canonical_definition_text(
            text, masked, status._after_windows_claim(masked, end), function)
        if definition is not None:
            return definition
    return None


def source_at(rva: int, function: str, rev: str | None) -> tuple[str | None, str | None]:
    """(path, definition) of the function at rev, or the working tree."""
    for path in claim_paths(rva, rev):
        if rev:
            shown = _git("show", f"{rev}:{path}")
            if shown.returncode:
                continue
            text = shown.stdout
        else:
            text = (common.HOMM3_DIR / path).read_text(errors="replace")
        definition = definition_at(text, rva, function)
        if definition is not None:
            return path, definition
    return None, None


def function_diff(old: str, new: str, *, old_name: str, new_name: str) -> str:
    return "\n".join(difflib.unified_diff(
        old.splitlines(), new.splitlines(), old_name, new_name, lineterm="", n=2))


def _select(rows: dict, selectors: list[str]) -> list:
    def parse_address(text: str) -> int:
        value = int(text, 16)
        return value - common.IMAGE_BASE if value >= common.IMAGE_BASE else value
    out = []
    for key, row in rows.items():
        haystack = f"{key[0]} {key[1]}".lower()
        ok = True
        for selector in selectors:
            if re.fullmatch(r"0x[0-9a-fA-F]+", selector):
                ok &= row.rva is not None and row.rva == parse_address(selector)
            else:
                ok &= selector.lower() in haystack
        if ok:
            out.append((key, row))
    return sorted(out, key=lambda item: item[0])


def cmd_last_exact(args) -> int:
    rows = status.load_baseline()
    selected = _select(rows, args.selectors)
    if not selected:
        print(f"[status] no ledger row matches {' '.join(args.selectors)!r}")
        return 1
    if len(selected) > args.limit:
        for key, _row in selected[:20]:
            print(f"  {key[0]}\t{key[1]}")
        print(f"[status] {len(selected)} rows match; narrow the selector "
              f"(or raise --limit)")
        return 1
    timeline = load_timeline(rows)
    records = []
    for key, row in selected:
        target = args.at if args.at is not None else (
            100.0 if row.hist >= 100.0 - TOLERANCE else row.hist)
        event = peak_event(timeline, _identity_of(key, row), target)
        record = _event_record(key, row, event)
        path = definition = None
        if row.rva is not None:
            path, definition = source_at(row.rva, key[1], None)
        record["path"] = path
        if event.peak is not None and path is not None:
            stop = event.drop.sha if event.drop else "HEAD"
            log = _git("log", "--format=%h %ad %s", "--date=short",
                       f"{event.peak.sha}..{stop}", "--", path)
            record["file_commits"] = log.stdout.splitlines()
            if args.diff and row.rva is not None:
                old_path, old = source_at(row.rva, key[1], event.peak.sha)
                if old is None:
                    record["diff"] = None
                else:
                    record["diff"] = function_diff(
                        old, definition or "",
                        old_name=f"{event.peak.sha}:{old_path}",
                        new_name=f"worktree:{path}")
        records.append(record)
    if args.json:
        print(json.dumps(records, indent=2))
        return 0
    for record in records:
        cur = "-" if record["cur"] is None else f"{record['cur']:.4f}"
        print(f"{record['unit']}\t{record['function']}\t{record['rva'] or '-'}\t"
              f"CUR {cur} MAX {record['max']:.4f} HIST {record['hist']:.4f}")
        peak, drop = record["peak"], record["drop"]
        if peak is None:
            print(f"    no ledger commit banked CUR >= {record['target']:.4f}")
            continue
        inherited = " (inherited MAX/HIST, never banked as CUR)" if record["inherited"] else ""
        print(f"    last at {record['target']:.4f}{inherited}: {peak['commit']} "
              f"{peak['date']} {peak['subject']}")
        if drop is not None:
            print(f"    dropped: {drop['commit']} {drop['date']} -> "
                  f"{record['drop_score']:.4f} {drop['subject']}")
        if record.get("file_commits"):
            print(f"    commits touching {record['path']} in that range:")
            for line in record["file_commits"]:
                print(f"      {line}")
        if args.diff:
            if record.get("diff") is None:
                print("    (no definition found at the peak commit)")
            elif record["diff"]:
                print(record["diff"])
            else:
                print("    (definition unchanged since the peak)")
    return 0


def add_parsers(sub, shared) -> None:
    import argparse
    gap = sub.add_parser(
        "hist-gap", parents=[shared],
        help="rows whose HIST exceeds MAX, with the commits that lost the peak",
        description="List ledger rows whose HIST (all-time peak) exceeds MAX by "
        "at least --min-gap, each with the last first-parent ledger commit "
        "that banked CUR at HIST and the first later commit that banked it "
        "lower. Reads committed ledger history only. Use the global --image "
        "for an editor ledger.")
    gap.add_argument("--min-gap", type=float, default=0.05, metavar="PCT",
                     help="smallest HIST - MAX to report (default 0.05; smaller "
                          "gaps are usually TU-state noise)")
    gap.add_argument("--exact", action="store_true",
                     help="only rows whose HIST is 100%%")
    gap.add_argument("--json", action="store_true", help="print JSON")

    last = sub.add_parser(
        "last-exact", parents=[shared],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        help="the last commit a function was exact (or at its HIST), and its diff since",
        description="For each ledger row matching every SELECTOR (substring of "
        "'unit function', or a retail VA/RVA), print the last ledger commit "
        "that banked CUR at the target (100%% when HIST is 100, else HIST), "
        "the first commit that banked it lower, and the commits in between "
        "that touched the function's source file. --diff prints the "
        "function's definition at that commit against the working tree.",
        epilog="examples:\n"
        "  homm3 status last-exact windowHandler@TViewArmyWindow --diff\n"
        "  homm3 status last-exact 0x5b4b20\n"
        "  homm3 --image h3maped status last-exact initializeTraitsTable --diff")
    last.add_argument("selectors", nargs="+", metavar="SELECTOR")
    last.add_argument("--at", type=float, metavar="PCT",
                      help="target score instead of 100 / HIST")
    last.add_argument("--diff", action="store_true",
                      help="diff the definition at the peak against the working tree")
    last.add_argument("--limit", type=int, default=5, metavar="N",
                      help="refuse when more than N rows match (default 5)")
    last.add_argument("--json", action="store_true", help="print JSON")
