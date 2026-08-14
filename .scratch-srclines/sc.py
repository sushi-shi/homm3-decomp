#!/usr/bin/env python3
"""Print match% for the given VAs (or unit:substr) from the objdiff report."""
import json
import sys

d = json.load(open("/home/sheep/Projects/homm3/wt-cold-windows/"
                   "build/objdiff/report.json"))
want = sys.argv[1:]
for u in d["units"]:
    un = u.get("name", "")
    for f in u.get("functions", []):
        nm = f.get("name", "")
        pct = f.get("fuzzy_match_percent", 0.0)
        for w in want:
            if w in nm or w == un:
                print(f"{pct:9.4f}  {un}  {nm[:80]}")
                break
