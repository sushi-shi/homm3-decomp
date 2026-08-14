#!/usr/bin/env python3
"""Count our code-generating source lines in a [start,end] range of a file.

A "statement line" = a non-blank line that is not pure punctuation
({ } ) ;), not a comment, not a preprocessor line, not a label-only line.
Continuation lines of one statement are folded: a line is counted only if the
*logical statement* starts on it (previous non-comment line ended in ; { } : or
is a control keyword head).
"""
import re
import sys


def load(path, a, b):
    lines = open(path, errors="replace").read().splitlines()
    return lines[a - 1:b]


def strip_comments(txt):
    txt = re.sub(r"/\*.*?\*/", " ", txt, flags=re.S)
    txt = re.sub(r"//[^\n]*", "", txt)
    return txt


def main():
    path, a, b = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    raw = load(path, a, b)
    body = strip_comments("\n".join(raw)).splitlines()
    n = 0
    prev_open = True   # start of a statement
    out = []
    for i, ln in enumerate(body):
        s = ln.strip()
        if not s or s.startswith("#"):
            continue
        core = s.strip("{}();, \t")
        if not core:
            prev_open = True
            continue
        if re.fullmatch(r"(case\s+[^:]+|default|\w+)\s*:", s):
            prev_open = True
            continue
        if prev_open:
            n += 1
            out.append((a + i, s))
        prev_open = bool(re.search(r"[;{}]\s*$", s)) or s.endswith(":")
    for ln, s in out:
        print(f"  {ln:6d} {s[:90]}")
    print(f"STATEMENT LINES: {n}   (raw span {a}..{b} = {b-a+1})")


if __name__ == "__main__":
    main()
