"""Read-only score queries: `status functions`, `status snapshot`, `status diff`.

None of these bank scores or write the ledger. They read the last measured
report through `status.read_report_view`, so a unit with unbuilt edits never
hides the other units' scores; its rows are flagged stale instead.
"""
from __future__ import annotations

import datetime
import json
from pathlib import Path
import subprocess
import sys

from homm3.core import common
from homm3.match import status

IMAGE_BASE = 0x400000
SNAPSHOT_SCHEMA = "homm3.status.snapshot.v1"


def parse_address(text: str) -> int:
    """A retail VA (0x00524dd0) or RVA (0x124dd0), returned as an RVA."""
    try:
        value = int(text, 0)
    except ValueError:
        value = int(text, 16)
    return value - IMAGE_BASE if value >= IMAGE_BASE else value


def _record(key, cur, maximum, hist, rva, stale) -> dict:
    return {"unit": key[0], "function": key[1],
            "va": None if rva is None else f"0x{rva + IMAGE_BASE:08x}",
            "rva": None if rva is None else f"0x{rva:x}",
            "cur": cur, "max": maximum, "hist": hist, "stale": key[0] in stale}


def function_records(report: dict, stale=()) -> list[dict]:
    """Banked MAX/HIST beside the measured CUR, one record per function."""
    rows = status.load_baseline()
    current = status.fn_fuzzy(report)
    rvas = status.function_rvas()
    out = []
    for key in sorted(set(rows) | set(current)):
        row = rows.get(key)
        cur = current.get(key, row.cur if row else None)
        maximum = row.max if row else cur
        historical = row.hist if row else cur
        rva = row.rva if row and row.rva is not None else rvas.get(key)
        out.append(_record(key, cur, maximum, historical, rva, stale))
    return out


def select(records: list[dict], *, filters=(), units=(), addresses=(),
           below: float | None = None, metric: str = "max") -> list[dict]:
    """Every filter must hold. FILTERs are case-insensitive substrings of
    "unit function"; --below compares MAX (or CUR with metric="cur")."""
    needles = [value.lower() for value in filters]
    wanted = {parse_address(value) for value in addresses}
    out = []
    for record in records:
        haystack = f"{record['unit']} {record['function']}".lower()
        if needles and not all(n in haystack for n in needles):
            continue
        if units and record["unit"] not in units:
            continue
        if wanted and (record["rva"] is None or int(record["rva"], 16) not in wanted):
            continue
        if below is not None:
            value = record[metric]
            if value is None or value >= below - 1e-6:
                continue
        out.append(record)
    return out


def _pct(value) -> str:
    return "    -   " if value is None else f"{value:7.2f}%"


def cmd_functions(view: status.ReportView, args) -> int:
    below = 100.0 if args.non_exact and args.below is None else args.below
    records = select(function_records(view.report, view.stale),
                     filters=args.filters, units=set(args.unit or ()),
                     addresses=args.va or (), below=below,
                     metric="cur" if args.cur else "max")
    if args.json:
        json.dump(records, sys.stdout, indent=1)
        sys.stdout.write("\n")
        return 0
    pending = status._pending_function_records()
    print("  MAX       CUR      HIST      RVA       Unit / function")
    print("  " + "-" * 78)
    for record in records:
        addr = "-" if record["rva"] is None else f"0x{int(record['rva'], 16):06x}"
        mark = " *" if record["stale"] else ""
        print(f"  {_pct(record['max'])}  {_pct(record['cur'])}  "
              f"{_pct(record['hist'])}  {addr:<8}  "
              f"{record['unit']} / {record['function']}{mark}")
        held = pending.get(record["function"])
        if held and held.get("unit", record["unit"]) in ("", record["unit"]):
            print(f"{'':42}{held['bytes']} EH-record byte(s) verify when this "
                  f"function becomes exact")
    if not records and (args.filters or args.unit or args.va or below is not None):
        print("  (no function matches the filters)", file=sys.stderr)
    return 0


def projected_records(view: status.ReportView) -> dict:
    """{(unit, function): record} with measured CUR and projected MAX/HIST."""
    rows = status.projected_rows(
        view.report, fingerprint_pair=status.fresh_fingerprints(view.report, view.stale))
    return {key: _record(key, row.cur, row.max, row.hist, row.rva, view.stale)
            for key, row in rows.items()}


def _head() -> str | None:
    result = subprocess.run(["git", "rev-parse", "HEAD"], cwd=common.HOMM3_DIR,
                            capture_output=True, text=True)
    return result.stdout.strip() or None


def cmd_snapshot(view: status.ReportView, path: Path, units=()) -> int:
    records = [r for r in projected_records(view).values()
               if not units or r["unit"] in units]
    payload = {"schema": SNAPSHOT_SCHEMA,
               "created": datetime.datetime.now(datetime.timezone.utc).isoformat(),
               "revision": _head(), "score_policy": status.SCORE_POLICY,
               "stale_units": sorted(view.stale), "functions": records}
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=1) + "\n")
    print(f"[status] snapshot: {len(records)} function(s) -> {path}")
    return 0


def load_before(against: str) -> tuple[dict, str]:
    """Rows from a snapshot file, else from the ledger at a git revision."""
    path = Path(against)
    if path.is_file():
        try:
            payload = json.loads(path.read_text())
        except ValueError as exc:
            common.die(f"{path} is not a status snapshot: {exc}")
        if not isinstance(payload, dict) or payload.get("schema") != SNAPSHOT_SCHEMA:
            common.die(f"{path} is not a `homm3 status snapshot` file")
        return ({(r["unit"], r["function"]): r for r in payload["functions"]},
                f"snapshot {path}")
    rows = status.baseline_at_ref(against)
    return ({key: _record(key, row.cur, row.max, row.hist, row.rva, ())
             for key, row in rows.items()}, f"ledger at {against}")


def _delta(a, b) -> float:
    return (b or 0.0) - (a or 0.0)


def diff_records(before: dict, after: dict) -> list[dict]:
    """Pair functions by retail RVA (labels change), else by label.

    The kind follows CUR, the measured score, then MAX: improved,
    regressed, new, removed or unchanged.
    """
    by_rva = {}
    for key, record in before.items():
        if record.get("rva") is not None:
            by_rva.setdefault(record["rva"], []).append(key)
    used, out = set(), []
    for key, now in sorted(after.items()):
        candidates = by_rva.get(now.get("rva"), []) if now.get("rva") else []
        old_key = candidates[0] if len(candidates) == 1 else key
        then = before.get(old_key)
        if then is not None and (old_key in used or (
                then.get("rva") and now.get("rva") and then["rva"] != now["rva"])):
            then = None
        if then is not None:
            used.add(old_key)
        if then is None:
            kind = "new"
        else:
            change = _delta(then["cur"], now["cur"])
            if abs(change) <= status.EPS:
                change = _delta(then["max"], now["max"])
            kind = ("improved" if change > status.EPS else
                    "regressed" if change < -status.EPS else "unchanged")
        out.append({"kind": kind, "unit": now["unit"], "function": now["function"],
                    "va": now["va"], "rva": now["rva"], "stale": now["stale"],
                    "before": None if then is None else
                    {k: then[k] for k in ("cur", "max", "hist")},
                    "after": {k: now[k] for k in ("cur", "max", "hist")},
                    "renamed_from": None if then is None or old_key == key
                    else old_key[1]})
    for key, then in sorted(before.items()):
        if key in used:
            continue
        out.append({"kind": "removed", "unit": key[0], "function": key[1],
                    "va": then.get("va"), "rva": then.get("rva"), "stale": False,
                    "before": {k: then[k] for k in ("cur", "max", "hist")},
                    "after": None, "renamed_from": None})
    return out


KINDS = ("improved", "regressed", "new", "removed")


def cmd_diff(view: status.ReportView, against: str, *, units=(),
             as_json: bool = False, show_all: bool = False) -> int:
    before, label = load_before(against)
    rows = diff_records(before, projected_records(view))
    if units:
        rows = [row for row in rows if row["unit"] in units]
    counts = {kind: sum(row["kind"] == kind for row in rows)
              for kind in (*KINDS, "unchanged")}
    shown = rows if show_all else [row for row in rows if row["kind"] != "unchanged"]
    if as_json:
        json.dump({"against": label, "counts": counts, "functions": shown},
                  sys.stdout, indent=1)
        sys.stdout.write("\n")
        return 0
    print(f"[status] diff against {label}: " + ", ".join(
        f"{counts[kind]} {kind}" for kind in (*KINDS, "unchanged")))
    if not shown:
        return 0
    print("  CUR before -> after    MAX before -> after    RVA       kind       "
          "Unit / function")
    print("  " + "-" * 96)
    order = {kind: i for i, kind in enumerate((*KINDS, "unchanged"))}
    for row in sorted(shown, key=lambda r: (order[r["kind"]], r["unit"], r["function"])):
        then, now = row["before"] or {}, row["after"] or {}
        addr = "-" if row["rva"] is None else f"0x{int(row['rva'], 16):06x}"
        renamed = f" (was {row['renamed_from']})" if row["renamed_from"] else ""
        mark = " *" if row["stale"] else ""
        print(f"  {_pct(then.get('cur'))} -> {_pct(now.get('cur'))}  "
              f"{_pct(then.get('max'))} -> {_pct(now.get('max'))}  {addr:<8}  "
              f"{row['kind']:<9}  {row['unit']} / {row['function']}{renamed}{mark}")
    return 0
