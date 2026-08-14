#!/usr/bin/env python3
"""Generic /Ob2 mass titrator: insert N self-assign pad units at the top of a
function body and report the objdiff score.

usage: mass.py <src.cpp> <line-of-opening-brace> <fn-substr> <n1,n2,...>
The opening brace line is the `{` that starts the body (1-based).
"""
import json
import subprocess
import sys

ROOT = "/home/sheep/Projects/homm3/wt-cold-windows"


def main():
    path, brace, fn, ns = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]
    full = f"{ROOT}/{path}"
    orig = open(full).read().splitlines(keepends=True)
    assert orig[brace - 1].strip() == "{", repr(orig[brace - 1])
    for n in [int(x) for x in ns.split(",")]:
        pad = "".join(f"    int zzp{i} = {i}; zzp{i} = zzp{i};\n"
                      for i in range(n))
        out = orig[:brace] + [pad] + orig[brace:]
        open(full, "w").write("".join(out))
        subprocess.run(["nix", "develop", ".#build", "--command", "bash", "-c",
                        "homm3 build --fast >/dev/null 2>&1"], cwd=ROOT)
        d = json.load(open(f"{ROOT}/build/objdiff/report.json"))
        got = None
        for u in d["units"]:
            for f in u.get("functions", []):
                if fn in f.get("name", ""):
                    got = f.get("fuzzy_match_percent", 0.0)
        print(f"  mass {n:4d}  {got}", flush=True)
    open(full, "w").write("".join(orig))


if __name__ == "__main__":
    main()
