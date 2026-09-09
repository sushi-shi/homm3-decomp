#!/usr/bin/env python3
"""Inventory project-source goto statements, excluding comments and carcasses.

This is a lexical source inventory, not a full preprocessor or a claim that a
goto existed in the original source. It neither edits code nor infers Dreamcast
source structure. Use --revision HEAD to compare with the committed tree.
"""

import argparse
import collections
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from homm3.match.status import _definition_text
from homm3.vc6._source import _definition_at, mask


def inventory(revision=None):
    command = (["git", "ls-tree", "-r", "--name-only", revision, "--", "src", "include"]
               if revision else ["git", "ls-files", "--", "src", "include"])
    filenames = subprocess.check_output(command, cwd=ROOT, text=True).splitlines()
    rows = []
    for filename in filenames:
        if Path(filename).suffix not in (".cpp", ".c", ".h", ".hpp"):
            continue
        raw = (subprocess.check_output(["git", "show", f"{revision}:{filename}"],
                                       cwd=ROOT, text=True)
               if revision else (ROOT / filename).read_text())
        code = mask(raw)
        gotos = list(re.finditer(r"\bgoto\s+(\w+)\s*;", code))
        if not gotos:
            continue
        owners = []
        for claim in re.finditer(r"\b(VA|DC_ONLY)\s*\(\s*(0x[\da-fA-F]+)\s*,[^)]*\)", code):
            definition = _definition_text(raw, code, claim.end())
            if not definition:
                continue
            start = claim.end()
            while code[start].isspace():
                start += 1
            end = start + len(definition)
            signature = " ".join(code[start:code.index("{", start)].split())
            owners.append((start, end, signature, claim.group(2).lower()))

        # Only unannotated helpers need an additional declaration scan. Reuse
        # the matching tools' balanced-definition reader to reject call sites.
        if any(not any(a <= g.start() < b for a, b, _, _ in owners) for g in gotos):
            for name in re.finditer(r"(?<![\w:])(?:\w+::)*\w+(?=\s*\()", code):
                if name.group() in {"if", "for", "while", "switch", "catch", "sizeof"}:
                    continue
                d = _definition_at(code, name.start(), name.group())
                if d is not None and not any(a <= d.head < b for a, b, _, _ in owners):
                    owners.append((d.head, d.body_close + 1, d.name, None))

        for g in gotos:
            candidates = [o for o in owners if o[0] <= g.start() < o[1]]
            if len(candidates) != 1:
                raise ValueError(f"{filename}:{raw.count(chr(10), 0, g.start()) + 1}: ambiguous goto owner")
            start, end, signature, address = candidates[0]
            label = g.group(1)
            destinations = list(re.finditer(r"\b" + re.escape(label) + r"\s*:(?!:)", code[start:end]))
            if len(destinations) != 1:
                raise ValueError(f"{filename}: unresolved label {label}")
            target = start + destinations[0].start()
            following = code[start + destinations[0].end():end].lstrip()
            rows.append(dict(file=filename, line=raw.count("\n", 0, g.start()) + 1,
                             function=signature, address=address, label=label,
                             target_line=raw.count("\n", 0, target) + 1,
                             direction="backward" if target < g.start() else "forward",
                             terminal_return=bool(re.fullmatch(r"return\b[^;]*;\s*}", following))))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--revision", help="read a git revision instead of the working tree")
    parser.add_argument("--json", type=Path, help="write the statement inventory as JSON")
    args = parser.parse_args()
    rows = inventory(args.revision)
    counts = collections.Counter((r["file"], r["function"]) for r in rows)
    print(f"{len(rows)} gotos in {len(counts)} functions, {len({r['file'] for r in rows})} files")
    print(f"{sum(r['direction'] == 'backward' for r in rows)} backward; "
          f"{sum(r['terminal_return'] for r in rows)} target a terminal return")
    for (file, function), count in sorted(counts.items()):
        print(f"{count:3}  {file}  {function}")
    if args.json:
        args.json.write_text(json.dumps(rows, indent=2) + "\n")


if __name__ == "__main__":
    main()
