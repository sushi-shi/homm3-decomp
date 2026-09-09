"""Inventory and test every source-level inline override region.

Usage: python scripts/experiments/audit-inline-pragmas.py [--run] [UNIT ...]
Results stay under build/pragma-audit and build/source-families. The inventory
includes directives in #if 0 blocks; the compiler distinguishes inactive and
active-but-redundant overrides. Pair resets with opens, do not count each line
as an independent workaround. This is not a Dreamcast source-shape comparator.

One-region deletion controls through the repository source-family runner.

This inventories directives, not Dreamcast source shape. No authored C++ or
ledger is changed. A final all-regions-removed option checks interactions.
"""
from pathlib import Path
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
import os
import re
import subprocess
import sys
import tomllib

ROOT = Path(os.environ.get("HOMM3_DIR", Path(__file__).resolve().parents[2])).resolve()
OUT = ROOT / "build/pragma-audit"
DIRECTIVE = re.compile(r"^[ \t]*#[ \t]*pragma\s+(inline_depth|auto_inline)\s*\(([^)]*)\)[ \t]*$")


def inventory(path):
    lines = path.read_text().splitlines(keepends=True)
    opened = {}
    regions = []
    for index, line in enumerate(lines):
        match = DIRECTIVE.fullmatch(line.rstrip("\n\r"))
        if not match:
            continue
        kind, setting = match.groups()
        if setting.strip() in ("0", "1", "off"):
            assert kind not in opened, (path, index, "nested region")
            opened[kind] = (index, setting.strip())
        else:
            assert setting.strip() in ("", "on"), (path, index, setting)
            assert kind in opened, (path, index, "unpaired reset")
            start, setting = opened.pop(kind)
            regions.append(dict(source=str(path.relative_to(ROOT)),
                                start=start + 1, end=index + 1,
                                kind=kind, setting=setting))
    assert not opened, (path, opened)
    return lines, sorted(regions, key=lambda row: row["start"])


def option(lines, region):
    start, end = region["start"] - 1, region["end"] - 1
    # Extend a duplicate anchor with real neighboring lines until unique.
    left, right = max(0, start - 1), min(len(lines), end + 2)
    source = "".join(lines)
    while source.count("".join(lines[left:right])) != 1:
        left, right = max(0, left - 1), min(len(lines), right + 1)
    return dict(name=f"remove-{region['kind']}-{region['setting']}-L{start + 1}",
                find="".join(lines[left:right]),
                replace="".join(line for i, line in enumerate(lines[left:right], left)
                                if i not in (start, end)))


def generate():
    OUT.mkdir(parents=True, exist_ok=True)
    units = tomllib.loads((ROOT / "config/units.toml").read_text())["unit"]
    inventory_rows = []
    manifests = []
    for unit in units:
        source = ROOT / unit["source"]
        if not source.is_relative_to(ROOT / "src"):
            continue
        lines, regions = inventory(source)
        if not regions:
            continue
        inventory_rows.extend(dict(unit=unit["unit"], **row) for row in regions)
        options = [option(lines, row) for row in regions]
        options.insert(0, dict(name="unchanged", find=options[0]["find"]))
        remove_lines = {row[field] - 1 for row in regions for field in ("start", "end")}
        options.append(dict(name="remove-all", find="".join(lines),
                            replace="".join(line for i, line in enumerate(lines)
                                            if i not in remove_lines)))
        manifest = dict(schema=1, source=unit["source"], units=[unit["unit"]],
                        axes=[dict(name="pragma-removal", options=options)])
        path = OUT / f"{unit['unit']}.json"
        path.write_text(json.dumps(manifest, indent=2) + "\n")
        manifests.append(path)
    (OUT / "inventory.json").write_text(json.dumps(inventory_rows, indent=2) + "\n")
    print(f"{len(inventory_rows)} regions in {len(manifests)} units", flush=True)
    return manifests


def run_one(path):
    env = dict(os.environ, HOMM3_DIR=str(ROOT), PYTHONPATH=str(ROOT / "scripts"),
               WINEPREFIX=str(ROOT / "build/wineprefix"))
    args = [sys.executable, "-m", "homm3.vc6.source_families", str(path),
            "--width", "60", "--keep", "10", "--jobs", "3", "--generations", "2"]
    with (OUT / f"{path.stem}.log").open("w") as log:
        check = subprocess.run(args + ["--validate-only"], env=env, cwd=ROOT,
                               stdout=log, stderr=subprocess.STDOUT)
        if check.returncode:
            return path.stem, check.returncode, None
        result = subprocess.run(args, env=env, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    marker = "[source-families] output "
    contexts = [Path(line[len(marker):]).name
                for line in (OUT / f"{path.stem}.log").read_text().splitlines()
                if line.startswith(marker)]
    return path.stem, result.returncode, contexts[-1] if contexts else None


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--output-dir", type=Path, default=OUT)
    parser.add_argument("units", nargs="*")
    args = parser.parse_args()
    OUT = args.output_dir.resolve()
    paths = generate()
    if args.units:
        unknown = set(args.units) - {path.stem for path in paths}
        if unknown:
            parser.error("no inventoried override regions for: " + ", ".join(sorted(unknown)))
        paths = [path for path in paths if path.stem in args.units]
    if args.run:
        failures = []
        contexts = {}
        with ThreadPoolExecutor(max_workers=2) as pool:
            tasks = [pool.submit(run_one, path) for path in paths]
            for future in as_completed(tasks):
                unit, code, context = future.result()
                print(f"{unit}: exit {code}", flush=True)
                if code:
                    failures.append(unit)
                elif context:
                    contexts[unit] = context
        (OUT / "contexts.json").write_text(json.dumps(contexts, indent=2) + "\n")
        if failures:
            raise SystemExit("Failed controls: " + ", ".join(failures))
