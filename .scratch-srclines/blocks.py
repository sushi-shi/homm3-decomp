#!/usr/bin/env python3
"""Show the largest insert/delete hunks of the normalized disasm diff."""
import difflib
import sys

sys.path.insert(0, "/home/sheep/Projects/homm3/wt-cold-windows/.scratch-srclines")
import rank


def main():
    va = sys.argv[1]
    minsz = int(sys.argv[2]) if len(sys.argv) > 2 else 3
    T, B = rank.parse(rank.run(va, False)), rank.parse(rank.run(va, True))
    tn = [rank.norm(x[0]) for x in T]
    bn = [rank.norm(x[0]) for x in B]
    sm = difflib.SequenceMatcher(None, tn, bn, autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        if max(i2 - i1, j2 - j1) < minsz:
            continue
        print(f"@@ {tag} retail[{i1}:{i2}] ours[{j1}:{j2}] "
              f"(-{i2-i1} +{j2-j1})")
        for k in range(i1, i2):
            print(f"  - {tn[k]}")
        for k in range(j1, j2):
            print(f"  + {bn[k]}")
        print()


if __name__ == "__main__":
    main()
