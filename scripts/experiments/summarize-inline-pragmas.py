"""Join exhaustive deletion controls to their emitted source identities.

Usage: python scripts/experiments/summarize-inline-pragmas.py [OUTPUT_DIR]
OUTPUT_DIR is the inventory/manifest directory made by audit-inline-pragmas.py.
The baseline comes from each runner snapshot, never today's changed source.
Unscored alternatives and ambiguous repeated contexts are errors, not passes.
"""
from pathlib import Path
from collections import Counter
import argparse
import json
import os
import sys

ROOT = Path(os.environ.get("HOMM3_DIR", Path(__file__).resolve().parents[2])).resolve()
sys.path.insert(0, str(ROOT / "scripts"))
from homm3.vc6 import source_families as families


def summarize(output):
    inventory = json.loads((output / "inventory.json").read_text())
    sites = {(row["unit"], row["start"]): row for row in inventory}
    if len(sites) != len(inventory):
        raise ValueError("duplicate inventory sites")
    contexts = {}
    selected_path = output / "contexts.json"
    selected = json.loads(selected_path.read_text()) if selected_path.is_file() else None
    for path in sorted((ROOT / "build/source-families").glob("*/checkpoint.json")):
        manifest = json.loads((path.parent / "input.json").read_text())
        if manifest["axes"][0]["name"] != "pragma-removal":
            continue
        unit = manifest["units"][0]
        if selected is not None and selected.get(unit) != path.parent.name:
            continue
        expected = output / (unit + ".json")
        if not expected.is_file() or json.loads(expected.read_text()) != manifest:
            continue
        if unit in contexts:
            raise ValueError(f"{unit}: multiple contexts; inspect their baseline/profile identities")
        contexts[unit] = path
    if set(contexts) != {row["unit"] for row in inventory}:
        raise ValueError("missing unit contexts")
    selected_path.write_text(json.dumps({unit: path.parent.name for unit, path in contexts.items()},
                                        indent=2) + "\n")

    result, combinations = [], []
    for unit, path in sorted(contexts.items()):
        input_path = path.parent / "input.json"
        checkpoint = json.loads(path.read_text())
        _, originals, axes = families.load_manifest(input_path, path.parent / "snapshot")
        if len(checkpoint["seen"]) != len(axes[0].options):
            raise ValueError(f"{unit}: family was not exhausted")
        records = {row["id"]: row for row in checkpoint["records"]}

        def row_for(index):
            rendered = families.render(originals, axes, [index])
            key = families.digest(json.dumps(rendered, sort_keys=True).encode())[:24]
            row = records[key]
            if not row.get("scores"):
                raise ValueError(f"{unit}/{index}: unscored alternative: {row.get('error')}")
            return row

        base = row_for(0)
        for index, option in enumerate(axes[0].options[1:], 1):
            row = row_for(index)
            if set(row["scores"]) != set(base["scores"]):
                raise ValueError(f"{unit}: score vector coverage changed")
            changed = [dict(symbol=key.split("|", 1)[1], before=value,
                            after=row["scores"][key],
                            delta=round(row["scores"][key] - value, 4))
                       for key, value in base["scores"].items()
                       if row["scores"][key] != value]
            gains = sum(change["delta"] > 0 for change in changed)
            losses = sum(change["delta"] < 0 for change in changed)
            identical = row["object_hash"] == base["object_hash"]
            if identical:
                category = "code-identical"
            elif not changed:
                category = "scores-identical-code-differs"
            elif gains and not losses:
                category = "gain-only"
            elif losses and not gains:
                category = "loss-only"
            else:
                category = "mixed"
            details = dict(category=category, gains=gains, losses=losses,
                           changes=changed, context=path.parent.name,
                           candidate=row["id"], baseline=base["id"],
                           code_identical=identical)
            if option.name == "remove-all":
                combinations.append(dict(unit=unit, **details))
            else:
                line = int(option.name.rsplit("L", 1)[1])
                result.append(dict(**sites[(unit, line)], **details))

    if len(result) != len(sites) or len({(r["unit"], r["start"]) for r in result}) != len(sites):
        raise ValueError("deletion coverage did not match the inventory")
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    (output / "all-removed-results.json").write_text(json.dumps(combinations, indent=2) + "\n")
    print(f"{len(result)} regions, {len(contexts)} units: "
          f"{dict(Counter(row['category'] for row in result))}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("output", nargs="?", type=Path, default=ROOT / "build/pragma-audit")
    summarize(parser.parse_args().output.resolve())
