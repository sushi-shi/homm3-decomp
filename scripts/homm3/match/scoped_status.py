"""Checkpoint explicitly selected, freshly compiled units without rebuilding peers."""
from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path

from homm3.core import common, compile_receipt
from homm3.core.cc_wrap import scan_header_deps
from homm3.core.project import Project
from homm3.match import status


def selection(project: Project, units: set[str]) -> tuple[dict, dict]:
    specs = {u["unit"]: u for u in project.manifest["unit"]}
    unknown = units - specs.keys()
    if not units or unknown:
        common.die("unknown or empty checkpoint unit selection: " + ", ".join(sorted(unknown)))
    config = json.loads((status.OBJDIFF_DIR / "objdiff.json").read_text())
    entries = [u for u in config.get("units", []) if u.get("name") in units]
    if {u["name"] for u in entries} != units or len(entries) != len(units):
        common.die("selected units must each have exactly one comparison entry")
    if config.get("options", {}).get("functionRelocDiffs") != "all":
        common.die("scoped checkpoint requires strict relocation comparison")
    return {name: specs[name] for name in units}, {**config, "units": entries}


def require_fresh(project: Project, specs: dict, config: dict) -> None:
    """Ninja, compiler content receipts and paired normalization all must agree."""
    from homm3.build.normalized_freshness import freshness_problems, ValidationContext
    status.require_built_sources(set(specs))
    root = project.root
    compiler = [p for p in (project.toolchain / "bin").iterdir()
                if p.is_file() and p.suffix.lower() in (".exe", ".dll")]
    if not compiler:
        common.die("compiler inputs unavailable for scoped checkpoint")
    hashes, context = {}, ValidationContext()
    for entry in config["units"]:
        unit = entry["name"]
        spec = specs[unit]
        source = root / spec["source"]
        raw = status.OBJDIFF_DIR / "base" / f"{unit}.obj"
        target = status.OBJDIFF_DIR / "target" / f"{unit}.c.obj"
        required = [source, root / "config/units.toml", root / "config/project.toml",
                    root / "scripts/homm3/core/cc_wrap.py",
                    root / "scripts/homm3/core/compile_receipt.py", *compiler,
                    *scan_header_deps(source, project.toolchain / "include", *project.includes)]
        if compile_receipt.current(raw, flags=project.manifest["flags"][spec["flags"]],
                                   required=required, hashes=hashes) is None:
            common.die(f"{unit}: raw compiler object lacks a current content receipt; rebuild this unit")
        for side, expected, inputs in (
                ("base_path", status.OBJDIFF_DIR / "normalized/base" / f"{unit}.obj",
                 {"raw": raw, "target": target}),
                ("target_path", status.OBJDIFF_DIR / "normalized/target" / f"{unit}.c.obj",
                 {"raw": target, "base": raw})):
            if (status.OBJDIFF_DIR / entry[side]).resolve() != expected.resolve():
                common.die(f"{unit}: {side} is not its normalized comparison object")
            problems = freshness_problems(expected, required_inputs={
                **inputs, "project": root / "config/project.toml",
                "symbol_names": status.SYMBOL_NAMES}, context=context)
            if problems:
                common.die("stale selected comparison:\n  " + "\n  ".join(problems[:10]))


def measure(config: dict) -> dict:
    """Use the normal objdiff policy, with no unselected objects in its project."""
    entries = [{**entry, **{side: str((status.OBJDIFF_DIR / entry[side]).resolve())
                            for side in ("base_path", "target_path")}}
               for entry in config["units"]]
    with tempfile.TemporaryDirectory(prefix="checkpoint-", dir=status.OBJDIFF_DIR) as tmp:
        directory = Path(tmp)
        (directory / "objdiff.json").write_text(json.dumps({**config, "units": entries}))
        output = directory / "report.json"
        result = subprocess.run(["objdiff-cli", "-C", str(directory), "report", "generate",
                                 "-o", str(output)], cwd=directory,
                                capture_output=True, text=True)
        if result.returncode:
            common.die("selected objdiff report failed:\n" + result.stdout + result.stderr)
        report = json.loads(output.read_text())
    names = [u.get("name") for u in report.get("units", [])]
    if sorted(names) != sorted(e["name"] for e in entries):
        common.die("selected report has missing, duplicate or unexpected units")
    if any(not u.get("functions") for u in report["units"]):
        common.die("selected report has a unit without compared functions")
    return report


def merge_rows(report: dict, previous: dict, units: set[str], hashes: dict,
               legacy: dict, rvas: dict) -> tuple[dict, dict]:
    selected = {k: v for k, v in previous.items() if k[0] in units}
    untouched = {k: v for k, v in previous.items() if k[0] not in units}
    current = status.fn_fuzzy(report)
    represented = {rvas[k] for k in current if k in rvas}
    collisions = {k[0] for k, row in untouched.items() if row.rva in represented}
    if collisions:
        common.die("retail body moved across checkpoint scope; also select " + ", ".join(sorted(collisions)))
    missing = [k for k, row in selected.items() if row.rva is not None
               and row.rva not in represented]
    if missing:
        common.die("selected report lost banked retail bodies: " + ", ".join(k[1] for k in missing[:10]))
    selected = status.migrate_source_hashes(selected, hashes, legacy)
    updated, stats = status.update_rows(current, selected, rvas, hashes)
    return {**untouched, **updated}, stats


def ledger_text(original: str, rows: dict, units: set[str]) -> str:
    """Replace rows in place; rationale follows a renamed body's stable RVA."""
    replacement, by_rva = {}, {}
    for (unit, name), row in sorted(rows.items()):
        if unit not in units:
            continue
        cur = "-" if row.cur is None else f"{row.cur:.4f}"
        rva = "-" if row.rva is None else f"0x{row.rva:x}"
        key = (unit, name)
        replacement[key] = (f"{unit}\t{name}\t{cur}\t{row.max:.4f}\t{row.hist:.4f}\t"
                            f"{rva}\t{row.src_hash or '-'}\n")
        if row.rva is not None:
            by_rva[row.rva] = key
    header, blocks, pending = [], [], []
    for line in original.splitlines(keepends=True):
        if not line.strip() or line.startswith("#"):
            pending.append(line)
            continue
        if not blocks:
            header, pending = pending, []
        blocks.append((pending, line))
        pending = []
    if not blocks:
        header, pending = pending, []
    lines, used, orphaned = header, set(), []
    for comments, line in blocks:
        key, old = next(iter(status.parse_baseline(line).items()))
        if key[0] not in units:
            lines.extend([*comments, line])
            continue
        new_key = by_rva.get(old.rva, key)
        if new_key in replacement and new_key not in used:
            lines.extend([*comments, replacement[new_key]])
            used.add(new_key)
        else:
            # Retired zero-score legacy labels have no body to anchor a note;
            # retain their notes at the end, as the complete writer does.
            orphaned.extend(comments)
    added = [text for key, text in replacement.items() if key not in used]
    if added and lines and not lines[-1].endswith("\n"):
        lines.append("\n")
    lines.extend([*added, *orphaned, *pending])
    return "".join(lines)


def update(units: set[str], *, readme: bool = False) -> int:
    with status.BASELINE.open(newline="") as stream:
        original = stream.read()
    if f"# score_policy={status.SCORE_POLICY}" not in original.splitlines():
        common.die("scoped checkpoint cannot migrate scoring policy; use a complete checkpoint")
    project = Project(common.HOMM3_DIR)
    specs, config = selection(project, units)
    require_fresh(project, specs, config)
    report = measure(config)
    hashes, legacy = status.source_hash_pair(only_units=units)
    rows, stats = merge_rows(report, status.parse_baseline(original), units,
                             hashes, legacy, status.function_rvas())
    # No input changes during objdiff/source scanning may slip into a receipt.
    require_fresh(project, specs, config)
    with status.BASELINE.open(newline="") as stream:
        unchanged = stream.read() == original
    if not unchanged:
        common.die("match ledger changed during scoped checkpoint; retry")
    if readme:
        status.write_readme({}, checkpoint_rows=rows)
    with status.BASELINE.open("w", newline="") as stream:
        stream.write(ledger_text(original, rows, units))
    print(f"[status] scoped checkpoint: {', '.join(sorted(units))}; "
          f"{stats['raised']} raised, {stats['reset']} source MAX resets; other units unchanged")
    return 0
