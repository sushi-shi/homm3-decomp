#!/usr/bin/env python3
"""homm3.build.patch_src - stage a vendor TU with its reviewed deviation patch.

vendor/ source snapshots stay byte-pristine; the ninja graph runs this
actor to produce the COMPILED view of a unit that retail provably
built from locally modified vendor source
(vendor/zlib-1.1.3/gzio.c.patch is the one such deviation: NWC's
`int len` in check_header). The staged directory receives the TU's
sibling headers (MSVC resolves quoted includes file-relatively) plus
the patched source.

Application is FAIL-CLOSED: every hunk's context and removed lines must
match the vendor bytes exactly, every hunk must apply, and the result
must differ from the input. A drifted vendor file or a stale patch
kills the build rather than silently compiling the wrong source - a
mis-applied deviation would poison the match evidence.
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

_HUNK = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")


def die(msg: str):
    print(f"[patch_src] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def parse_patch(text: str):
    """[(start_line_1based, [hunk lines])] from a single-file unified diff."""
    hunks = []
    current = None
    for line in text.splitlines():
        if line.startswith("#") or line.startswith("--- ") or line.startswith("+++ "):
            continue
        m = _HUNK.match(line)
        if m:
            current = (int(m.group(1)), [])
            hunks.append(current)
            continue
        if current is not None and line[:1] in (" ", "-", "+", ""):
            current[1].append(line)
    if not hunks:
        die("patch contains no hunks")
    return hunks


def apply_patch(source_lines: list[str], hunks) -> list[str]:
    out = list(source_lines)
    for start, body in reversed(sorted(hunks)):
        index = start - 1
        cursor = index
        replacement = []
        for line in body:
            tag, text = line[:1], line[1:]
            if tag in (" ", ""):
                if cursor >= len(out) or out[cursor].rstrip("\n") != text:
                    die(f"context mismatch at line {cursor + 1}: patch expects "
                        f"{text!r}, vendor has "
                        f"{out[cursor].rstrip(chr(10))!r}"
                        if cursor < len(out) else
                        f"context runs past end of file at line {cursor + 1}")
                replacement.append(out[cursor])
                cursor += 1
            elif tag == "-":
                if cursor >= len(out) or out[cursor].rstrip("\n") != text:
                    die(f"removed-line mismatch at line {cursor + 1}: patch "
                        f"removes {text!r}, vendor has "
                        f"{out[cursor].rstrip(chr(10))!r}")
                cursor += 1
            elif tag == "+":
                replacement.append(text + "\n")
        out[index:cursor] = replacement
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", required=True)
    ap.add_argument("--patch", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args(argv)
    src, patch, out = Path(a.src), Path(a.patch), Path(a.out)
    if not src.is_file():
        die(f"source missing: {src}")
    if not patch.is_file():
        die(f"patch missing: {patch}")

    source_lines = src.read_text().splitlines(keepends=True)
    patched = apply_patch(source_lines, parse_patch(patch.read_text()))
    if patched == source_lines:
        die(f"{patch.name} applied without changing {src.name} - a no-op "
            "deviation patch is a stale patch")

    out.parent.mkdir(parents=True, exist_ok=True)
    for header in sorted(src.parent.glob("*.h")):
        target = out.parent / header.name
        if not target.exists() or target.read_bytes() != header.read_bytes():
            shutil.copyfile(header, target)
    out.write_text("".join(patched))
    print(f"[patch_src] {src.name} + {patch.name} -> "
          f"{out} ({len(list(src.parent.glob('*.h')))} headers staged)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
