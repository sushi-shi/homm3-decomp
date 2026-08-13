#!/usr/bin/env python3
"""homm3.match.verify_va_claims - FATAL gates over the src VA() claims.

Source is the authority for names, which makes the `VA(0xADDR, size)`
claims in src/ + include/ the contract this gate enforces against the
admitted retail inventory (config/retail-functions.tsv) and the universe
classifier. Three checks, all fatal in the `homm3 build` tail (never in
`--fast` - the gates belong to the orchestrator's loop, not the
matcher's inner loop):

  UNIQUE       one VA = one claim, tree-wide. Two claims on one address
               is either a paste error or two files fighting over one
               function.
  RECONCILED   every claimed VA is a carved function ENTRY and the
               claimed size equals the admitted size. A typo in a
               hand-edited VA/size dies here, not three tools later.
  CLASSIFIED   a VA() claim must land on game-target code; claiming a
               zlib/runtime/funclet/thunk address is a mis-attribution.
               (VA_COMPGEN may additionally claim init-thunks - those
               bodies are compiler-generated and reconstructed
               implicitly. No uses exist yet; the allowance is wired for
               their arrival.)
  IN ORDER     within each file the VA() claims are strictly increasing:
               the carcass preserves retail link order, and a function
               pasted into the wrong place breaks the order before it
               breaks anything else. VA() only - DC_ONLY() carries
               Dreamcast addresses and is being removed as functions get
               retail claims, so it is deliberately not order-checked.

Known-backlog ratchet (the gruntz single_view shape): the violations
that existed when the gate landed are frozen in
config/va-claims-baseline.tsv - reported as standing debt, drained in
explicit claim-review sessions, never silently re-blessed - and any
violation NOT in that file is fatal. `--write-baseline` re-freezes
(only ever after a review). The 2026-08-04 backlog is 11 CLASS rows:
linkorder-grade DC names bracketed onto addresses that are byte-provably
compiler-generated initializer thunks (guard byte + _atexit
registration) - the name transfer slid one slot across interleaved
thunks; the claims need retail homes re-proven.

Runs its embedded selftest (synthetic defects that MUST be detected +
a clean sample that MUST pass) on every invocation - the gate proves it
can fail before it judges the tree.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

from homm3.core import common

FUNCTIONS = common.HOMM3_DIR / "config/retail-functions.tsv"
BASELINE = common.HOMM3_DIR / "config/va-claims-baseline.tsv"
ROOTS = ("src", "include")
EXTS = {".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}

_CLAIM = re.compile(
    r"^[ \t]*(VA|VA_COMPGEN)\(\s*(0[xX][0-9a-fA-F]+)\s*,\s*"
    r"(0[xX][0-9a-fA-F]+|\d+)")

# Universe classes a claim may land on, per macro.
_ALLOWED = {"VA": {"target"}, "VA_COMPGEN": {"target", "init-thunk"}}


def parse_claims(text: str):
    """[(lineno, macro, va, size)] for every VA-family claim in one file."""
    out = []
    for lineno, line in enumerate(text.splitlines(), 1):
        m = _CLAIM.match(line)
        if m:
            out.append((lineno, m.group(1),
                        int(m.group(2), 16), int(m.group(3), 0)))
    return out


def load_functions() -> dict[int, int]:
    functions = {}
    for line in FUNCTIONS.open():
        if line.startswith("#") or line.startswith("rva\t"):
            continue
        rva, size = line.split("\t")
        functions[int(rva, 16)] = int(size)
    return functions


def check(claims_by_file: dict, functions: dict, classes: dict) -> list[tuple]:
    """All (kind, va, message) violations across the four checks; [] =
    the contract holds. `claims_by_file` maps a display path to that
    file's parsed claims; `functions` is rva->size; `classes` is
    rva->universe category."""
    violations = []
    owners: dict[int, str] = {}
    for path, claims in sorted(claims_by_file.items()):
        previous_va = None
        for lineno, macro, va, size in claims:
            where = f"{path}:{lineno}"
            rva = va - common.IMAGE_BASE if va >= common.IMAGE_BASE else va

            first = owners.setdefault(rva, where)
            if first != where:
                violations.append((
                    "DUPLICATE", va,
                    f"DUPLICATE claim: 0x{va:08x} at {where} already "
                    f"claimed at {first}"))

            admitted = functions.get(rva)
            if admitted is None:
                violations.append((
                    "UNKNOWN", va,
                    f"UNKNOWN VA: {where} claims 0x{va:08x} but rva "
                    f"0x{rva:x} is not a carved function entry "
                    "(config/retail-functions.tsv)"))
            elif size != admitted:
                violations.append((
                    "SIZE", va,
                    f"SIZE MISMATCH: {where} claims 0x{va:08x} size "
                    f"{size} (0x{size:x}) but the admitted size is "
                    f"{admitted} (0x{admitted:x})"))
            else:
                category = classes.get(rva, "target")
                if category not in _ALLOWED[macro]:
                    violations.append((
                        "CLASS", va,
                        f"CLASS overlap: {where} claims 0x{va:08x} which "
                        f"is {category} code, not a game-target function"))

            if macro == "VA":
                if previous_va is not None and va <= previous_va:
                    violations.append((
                        "ORDER", va,
                        f"ORDER violation: {where} claims 0x{va:08x} "
                        f"after 0x{previous_va:08x} - VA() claims must "
                        "be strictly increasing (retail link order)"))
                previous_va = va
    return violations


def load_backlog() -> set[tuple]:
    """{(kind, va)} rows frozen at gate-landing time, awaiting explicit
    claim re-review. Keyed on (kind, va), not file:line - carcass edits
    move lines, the debt's identity is the address."""
    if not BASELINE.is_file():
        return set()
    out = set()
    for line in BASELINE.read_text().splitlines():
        if line.startswith("#") or "\t" not in line:
            continue
        kind, va = line.split("\t")[:2]
        out.add((kind, int(va, 16)))
    return out


def write_backlog(violations: list[tuple]) -> None:
    head = ("# KNOWN VA-claim backlog - standing debt reported by\n"
            "# homm3.match.verify_va_claims, drained by explicit review\n"
            "# claim-review sessions; NEW violations are fatal.\n")
    rows = "".join(f"{kind}\t0x{va:08x}\t{msg.split(': ', 1)[1]}\n"
                   for kind, va, msg in violations)
    BASELINE.write_text(head + rows)


# --- the embedded negative control ------------------------------------------------

def selftest() -> list[str]:
    functions = {0x1000: 13, 0x2000: 32, 0x3000: 8}
    classes = {0x1000: "target", 0x2000: "target", 0x3000: "runtime"}

    def one(claims, kind):
        got = check({"t.cpp": claims}, functions, classes)
        return any(k == kind for k, _va, _m in got)

    failures = []
    clean = [(1, "VA", 0x401000, 13), (5, "VA", 0x402000, 32)]
    if check({"t.cpp": clean}, functions, classes):
        failures.append("clean sample did not pass")
    if not one([(1, "VA", 0x401000, 13), (9, "VA", 0x401000, 13)],
               "DUPLICATE"):
        failures.append("duplicate claim not detected")
    if not check({"a.cpp": [(1, "VA", 0x401000, 13)],
                  "b.cpp": [(1, "VA", 0x401000, 13)]}, functions, classes):
        failures.append("cross-file duplicate not detected")
    if not one([(1, "VA", 0x409999, 13)], "UNKNOWN"):
        failures.append("unknown VA not detected")
    if not one([(1, "VA", 0x401000, 14)], "SIZE"):
        failures.append("size mismatch not detected")
    if not one([(1, "VA", 0x403000, 8)], "CLASS"):
        failures.append("class overlap not detected")
    if not one([(1, "VA", 0x402000, 32), (9, "VA", 0x401000, 13)],
               "ORDER"):
        failures.append("order violation not detected")
    if one([(1, "VA_COMPGEN", 0x402000, 32), (9, "VA", 0x401000, 13)],
           "ORDER"):
        failures.append("VA_COMPGEN wrongly order-checked")
    # the backlog split: a frozen (kind, va) is debt, a new one is fatal
    got = check({"t.cpp": [(1, "VA", 0x403000, 8)]}, functions, classes)
    frozen = {("CLASS", 0x403000)}
    if [v for v in got if (v[0], v[1]) not in frozen]:
        failures.append("baselined violation wrongly fatal")
    if not [v for v in got if (v[0], v[1]) not in set()]:
        failures.append("unbaselined violation wrongly tolerated")
    return failures


# --- entry points -----------------------------------------------------------------

def _scan():
    claims_by_file = {}
    total = 0
    for root in ROOTS:
        base = common.HOMM3_DIR / root
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix not in EXTS or not path.is_file():
                continue
            claims = parse_claims(path.read_text(errors="ignore"))
            if claims:
                claims_by_file[str(path.relative_to(common.HOMM3_DIR))] = claims
                total += len(claims)
    from homm3.match import universe
    classes, _sizes = universe.classify()
    return claims_by_file, total, check(
        claims_by_file, load_functions(), classes)


def run_gate() -> list[str]:
    """Selftest + the real tree. Returns FATAL lines (empty = pass) -
    violations frozen in the backlog print as standing debt instead."""
    broken = selftest()
    if broken:
        return [f"va-claims SELFTEST BROKEN: {b}" for b in broken]
    claims_by_file, total, violations = _scan()
    backlog = load_backlog()
    known = [v for v in violations if (v[0], v[1]) in backlog]
    fatal = [msg for kind, va, msg in violations
             if (kind, va) not in backlog]
    stale = backlog - {(k, va) for k, va, _ in violations}
    summary = (f"[build] va-claims: {total} claims in "
               f"{len(claims_by_file)} files")
    if known:
        summary += f"; {len(known)} known-backlog (va-claims-baseline.tsv)"
    if stale:
        summary += (f"; {len(stale)} baseline row(s) no longer fire - "
                    "remove them")
    if not fatal:
        print(summary + " - no new violations")
    return fatal


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--selftest" in argv:
        broken = selftest()
        for b in broken:
            print(f"SELFTEST BROKEN: {b}", file=sys.stderr)
        print("selftest OK" if not broken else "selftest FAILED")
        return 2 if broken else 0
    if "--write-baseline" in argv:
        _files, _total, violations = _scan()
        write_backlog(violations)
        print(f"va-claims backlog frozen: {len(violations)} row(s) -> "
              f"{BASELINE.relative_to(common.HOMM3_DIR)}")
        return 0
    if "--backlog" in argv:
        _files, _total, violations = _scan()
        backlog = load_backlog()
        for kind, va, msg in violations:
            tag = "known " if (kind, va) in backlog else "NEW   "
            print(f"{tag}{msg}")
        return 0
    fatal = run_gate()
    for line in fatal:
        print(line, file=sys.stderr)
    return 1 if fatal else 0


if __name__ == "__main__":
    sys.exit(main())
