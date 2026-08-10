"""homm3.vc6.oracle - real-compiler ground-truth runner for the probe corpus.

Each probe under scripts/homm3/vc6/probes/ is a standalone TU that reproduces
one catalogued compiler behaviour (docs/vc6/behavior-catalog.md). The runner
compiles a probe with the pinned VC6 via homm3.core.cc_wrap, adds /FAs for an
.asm listing, and checks the probe's machine-readable expectations against it.

Probe header directives (C++ line comments, one per line):

    // CATALOG: A1 A2                    catalog IDs the probe reproduces
    // FLAGS: /O2 /Ob2 ...               cl flags (runner appends /c /FAs)
    // EXPECT-ASM: <regex>               DOTALL search over the whole listing
    // EXPECT-ASM(scope): <regex>        ... over one function's PROC..ENDP
    // EXPECT-NOT-ASM[(scope)]: <regex>  must NOT match
    // EXPECT-COUNT-ASM(scope): <n> <regex>   exactly n matches
    // EXPECT-SAME-ASM: <scopeA> <scopeB>     normalized instruction streams
                                              of two functions must be equal

Matching text is the FILTERED listing: whole-line comments and blank lines
dropped, inline `;` comments stripped, and inside a scope the PROC/ENDP lines
reduced to the bare words "PROC"/"ENDP" (so a function's own name cannot
false-match a pattern like `call`, and windows like [\\s\\S]{0,80} are not
inflated by comment text). `scope` is a substring of the mangled PROC line.

Statuses: PASS (all expectations hold), FAIL (any does not), UNCHECKED
(compiled, no expectations), ERROR (compile or harness problem).
rc 0 = all selected probes pass; 1 = any FAIL/ERROR (gate semantics); the
run dies (rc 2) only on harness-level misuse. Scratch: build/vc6/probes/.
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

from homm3.vc6 import _common

PROBES_DIR = Path(__file__).resolve().parent / "probes"
SCRATCH = _common.REPO / "build/vc6/probes"

_SUBSYSTEM_PREFIX = {"inline": "a", "reg": "b", "state": "c", "flow": "d", "all": ""}

_DIRECTIVE_RE = re.compile(
    r"^//\s*(EXPECT-ASM|EXPECT-NOT-ASM|EXPECT-COUNT-ASM|EXPECT-SAME-ASM)"
    r"(?:\(([^)]*)\))?:\s*(.*\S)\s*$")
_FLAGS_RE = re.compile(r"^//\s*FLAGS:\s*(.*\S)\s*$")
_CATALOG_RE = re.compile(r"^//\s*CATALOG:\s*(.*\S)\s*$")
_LABEL_RE = re.compile(r"\$[A-Za-z]+\$?\d+")


def _parse_probe(src: Path):
    """Return (flags: list[str], catalog: str, checks: list[(kind, scope, value)])."""
    flags, catalog, checks = None, "", []
    for line in src.read_text().splitlines():
        line = line.strip()
        if not line.startswith("//"):
            continue
        m = _FLAGS_RE.match(line)
        if m:
            flags = m.group(1).split()
            continue
        m = _CATALOG_RE.match(line)
        if m:
            catalog = m.group(1)
            continue
        m = _DIRECTIVE_RE.match(line)
        if m:
            checks.append((m.group(1), m.group(2), m.group(3)))
    return flags, catalog, checks


def _filter_listing(asm_text: str) -> list[str]:
    """Drop whole-line comments/blanks, strip inline comments."""
    out = []
    for line in asm_text.splitlines():
        line = line.rstrip()
        if not line or line.lstrip().startswith(";"):
            continue
        out.append(re.sub(r"\s+;.*$", "", line))
    return out


def _scope_text(lines: list[str], scope: str):
    """PROC..ENDP block of the function whose PROC line contains `scope`;
    the boundary lines are reduced to bare PROC/ENDP markers."""
    body, inside = [], False
    for line in lines:
        if not inside and scope in line and " PROC" in line:
            inside = True
            body.append("PROC")
            continue
        if inside:
            if " ENDP" in line:
                body.append("ENDP")
                return "\n".join(body)
            body.append(line)
    return None


def _normalized_stream(lines: list[str], scope: str):
    """Instruction/label stream of a function with label numbers masked."""
    text = _scope_text(lines, scope)
    if text is None:
        return None
    return [_LABEL_RE.sub("$#", ln.strip()) for ln in text.splitlines()[1:-1]]


def _compile_probe(src: Path, flags: list[str]):
    """cc_wrap the probe; return (asm_path or None, tail-of-output)."""
    out = SCRATCH / (src.stem + ".obj")
    cmd = [sys.executable, "-m", "homm3.core.cc_wrap",
           "--out", str(out), "--src", str(src),
           "--", "/c", *flags, "/FAs"]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    asm = SCRATCH / (src.stem + ".asm")
    if proc.returncode != 0 or not asm.is_file():
        tail = "\n".join((proc.stdout + proc.stderr).strip().splitlines()[-8:])
        return None, tail
    return asm, ""


def _evaluate(checks, lines):
    """Run every expectation; return list of result dicts."""
    results = []
    full = "\n".join(lines)
    for kind, scope, value in checks:
        res = {"directive": kind, "scope": scope or "", "value": value,
               "ok": False, "detail": ""}
        if kind == "EXPECT-SAME-ASM":
            names = value.split()
            if len(names) != 2:
                res["detail"] = "needs exactly two scope names"
            else:
                a = _normalized_stream(lines, names[0])
                b = _normalized_stream(lines, names[1])
                if a is None or b is None:
                    res["detail"] = "scope not found: " + names[a is not None]
                elif a == b:
                    res["ok"] = True
                else:
                    delta = next((i for i, (x, y) in enumerate(zip(a, b))
                                  if x != y), min(len(a), len(b)))
                    res["detail"] = (f"streams differ at instruction {delta} "
                                     f"({len(a)} vs {len(b)} lines)")
            results.append(res)
            continue
        text = full if scope is None else _scope_text(lines, scope)
        if text is None:
            res["detail"] = f"scope not found: {scope}"
            results.append(res)
            continue
        if kind == "EXPECT-COUNT-ASM":
            m = re.match(r"(\d+)\s+(.*)", value)
            if not m:
                res["detail"] = "malformed count directive"
                results.append(res)
                continue
            want, pattern = int(m.group(1)), m.group(2)
            got = sum(1 for _ in re.finditer(pattern, text, re.DOTALL))
            res["ok"] = got == want
            if not res["ok"]:
                res["detail"] = f"matched {got}, expected {want}"
        elif kind == "EXPECT-ASM":
            res["ok"] = re.search(value, text, re.DOTALL) is not None
            if not res["ok"]:
                res["detail"] = "no match"
        elif kind == "EXPECT-NOT-ASM":
            hit = re.search(value, text, re.DOTALL)
            res["ok"] = hit is None
            if not res["ok"]:
                res["detail"] = f"unexpected match: {hit.group(0)[:60]!r}"
        results.append(res)
    return results


def _select_probes(args) -> list[Path]:
    if not PROBES_DIR.is_dir():
        _common.die(f"probe corpus missing: {PROBES_DIR}")
    probes = sorted(PROBES_DIR.glob("*.cpp"))
    key = (getattr(args, "subsystem", None) or "all").lower()
    if key in _SUBSYSTEM_PREFIX:
        prefix = _SUBSYSTEM_PREFIX[key]
        probes = [p for p in probes if p.stem.startswith(prefix)]
    else:  # a probe id/stem prefix, e.g. `oracle a06`
        probes = [p for p in probes if p.stem.startswith(key)]
    if getattr(args, "probe", None):
        probes = [p for p in probes if p.stem.startswith(args.probe)]
    return probes


def run(args) -> int:
    probes = _select_probes(args)
    if not probes:
        _common.die(f"no probes selected (subsystem={args.subsystem!r}, "
                    f"probe={getattr(args, 'probe', None)!r})")
    SCRATCH.mkdir(parents=True, exist_ok=True)
    report, rc = [], 0
    for src in probes:
        flags, catalog, checks = _parse_probe(src)
        entry = {"probe": src.stem, "catalog": catalog, "checks": []}
        if flags is None:
            entry["status"] = "ERROR"
            entry["detail"] = "no // FLAGS: line"
            rc = 1
        else:
            asm, tail = _compile_probe(src, flags)
            if asm is None:
                entry["status"] = "ERROR"
                entry["detail"] = f"compile failed:\n{tail}"
                rc = 1
            elif not checks:
                entry["status"] = "UNCHECKED"
                entry["detail"] = f"compiled OK, read {asm}"
            else:
                lines = _filter_listing(asm.read_text(errors="replace"))
                entry["checks"] = _evaluate(checks, lines)
                bad = [c for c in entry["checks"] if not c["ok"]]
                entry["status"] = "FAIL" if bad else "PASS"
                if bad:
                    rc = 1
        report.append(entry)
    if getattr(args, "json", False):
        print(json.dumps(report, indent=2))
        return rc
    for e in report:
        n = len(e["checks"])
        note = f"({n} check{'s' if n != 1 else ''})" if n else e.get("detail", "")
        print(f"[oracle] {e['probe']:<32} {e['status']:<9} {note:<12} {e['catalog']}")
        if e["status"] == "FAIL":
            for c in e["checks"]:
                if not c["ok"]:
                    scope = f"({c['scope']})" if c["scope"] else ""
                    print(f"         FAILED {c['directive']}{scope}: "
                          f"{c['value']}\n                {c['detail']}")
        elif e["status"] == "ERROR":
            print(f"         {e.get('detail', '')}")
    counts = {s: sum(1 for e in report if e["status"] == s)
              for s in ("PASS", "FAIL", "UNCHECKED", "ERROR")}
    print(f"[oracle] {len(report)} probes: {counts['PASS']} PASS, "
          f"{counts['FAIL']} FAIL, {counts['UNCHECKED']} UNCHECKED, "
          f"{counts['ERROR']} ERROR")
    return rc
