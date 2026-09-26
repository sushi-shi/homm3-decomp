"""Compile one complete authored TU with CodeWarrior for the Mac target.

Ninja's `mwcc` rule (homm3.build.configure) runs this for every shared game
unit in config/units.toml:

    python3 -m homm3.mac.cc_wrap --unit hero --src src/hero.cpp \
        --out build/mac/obj/hero.o

It compiles the actual source file - never a generated selection of bodies -
with the unit's flags from config/mac/units.toml, the project include roots
and the staged CodeWarrior library headers. Beside the object it writes:

    <unit>.log         CodeWarrior's complete diagnostics (also on failure)
    <unit>.dis.txt     MWLinkPPC -dis listing of the object
    <unit>.hunks.json  every emitted MWOB code/data hunk, for pairing
    <unit>.o.d         gcc-style depfile over the project-header closure

A failed compile exits nonzero with its real diagnostics; the TU stays in the
graph. `homm3 mac objects` summarizes the per-TU state.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from homm3.core import common

ROOT = common.HOMM3_DIR
OBJECTS = ROOT / "build/mac/obj"
# The compiler-spelling header is a prefix, as the selected-body path's
# leading include was; the TU itself is compiled unchanged.
MAC_DEFINES = ("-msext", "on", "-DHOMM3_TARGET_MAC=1", "-prefix", "include/codewarrior_prefix.h")


def flags_for(unit: str) -> tuple[str, ...]:
    from homm3.mac import profiles
    flags = profiles.flags(ROOT, unit)
    if "-nolink" not in flags:
        raise ValueError(f"{unit}: Mac profile must emit an object with -nolink")
    return (*flags, *MAC_DEFINES)


def include_roots() -> tuple[str, ...]:
    import tomllib
    from homm3.mac import sdk
    project = tomllib.loads((ROOT / "config/units.toml").read_text())
    directories = tuple(project.get("build", {}).get("includes", ["include"]))
    return ("src", *dict.fromkeys(("include", *directories)), *sdk.include_dirs(ROOT))


def _wine_env() -> dict[str, str]:
    from homm3.mac.toolchain import wine_version
    env = dict(os.environ)
    version = re.sub(r"[^A-Za-z0-9._-]", "_", wine_version())
    env["WINEPREFIX"] = os.environ.get("HOMM3_MAC_WINEPREFIX",
                                       str(ROOT / "build/mac/wineprefix" / version))
    env.setdefault("WINEDEBUG", "-all")
    Path(env["WINEPREFIX"]).mkdir(parents=True, exist_ok=True)
    env["MWCIncludes"] = ";".join(include_roots())
    return env


def _depfile(out: Path, source: Path) -> None:
    from homm3.core.cc_wrap import scan_header_deps
    roots = [ROOT / root for root in include_roots() if not root.startswith("build/")]
    headers = scan_header_deps(source, *roots)
    relative = [Path(path).relative_to(ROOT).as_posix() if Path(path).is_relative_to(ROOT) else path
                for path in headers]
    target = out.relative_to(ROOT).as_posix()
    escaped = " ".join(path.replace(" ", "\\ ") for path in relative)
    out.with_name(out.name + ".d").write_text(f"{target}: {escaped}\n")


def compile_unit(unit: str, source: Path, out: Path) -> int:
    from homm3.mac import sdk, toolchain
    from homm3.mac.object import parse_code_hunks, parse_data_hunks
    tools = toolchain.stage()
    sdk.stage(root=ROOT)
    out.parent.mkdir(parents=True, exist_ok=True)
    log = out.with_suffix(".log")
    for stale in (out, out.with_suffix(".dis.txt"), out.with_suffix(".hunks.json")):
        stale.unlink(missing_ok=True)
    _depfile(out, source)
    env = _wine_env()
    command = ["wine", str(tools / "MWCPPC.exe"), *flags_for(unit),
               "-o", out.relative_to(ROOT).as_posix(), source.relative_to(ROOT).as_posix()]
    try:
        completed = subprocess.run(command, cwd=ROOT, env=env, capture_output=True,
                                   text=True, errors="replace", timeout=600)
        diagnostics, code = completed.stdout + completed.stderr, completed.returncode
    except subprocess.TimeoutExpired:
        diagnostics, code = "CodeWarrior timed out after 600 seconds\n", 124
    log.write_text(" ".join(command) + "\n" + diagnostics)
    if code or not out.is_file() or not out.read_bytes().startswith(b"MWOBPPC "):
        out.unlink(missing_ok=True)
        sys.stderr.write(f"[mac] {unit}: CodeWarrior failed ({code}); see {log.relative_to(ROOT)}\n")
        sys.stderr.write(diagnostics[-4000:])
        return 1
    listing = subprocess.run(["wine", str(tools / "MWLinkPPC.exe"), "-dis",
                              out.relative_to(ROOT).as_posix()],
                             cwd=ROOT, env=env, capture_output=True, text=True,
                             errors="replace", timeout=600)
    if listing.returncode:
        sys.stderr.write(f"[mac] {unit}: MWLinkPPC -dis failed\n{listing.stdout}{listing.stderr}")
        out.unlink(missing_ok=True)
        return 1
    text = listing.stdout + listing.stderr
    out.with_suffix(".dis.txt").write_text(text)
    code_hunks = parse_code_hunks(text)
    data_hunks = parse_data_hunks(text)
    out.with_suffix(".hunks.json").write_text(json.dumps({
        "unit": unit, "source": source.relative_to(ROOT).as_posix(),
        "flags": list(flags_for(unit)),
        "code": [{"symbol": hunk.name, "size": len(hunk.data), "references": hunk.xrefs}
                 for hunk in code_hunks],
        "data": [{"symbol": hunk.name, "size": hunk.declared_size} for hunk in data_hunks],
    }, indent=2) + "\n")
    return 0


def first_error(log: str) -> str:
    """`file:line: message` of CodeWarrior's first error, or its last line.

    Each diagnostic is a `###` block: optional `#    In: file` / `#    File:`
    context, `#  NNN: source`, a `#   Error: ^^^` caret, then the message.
    """
    lines = log.splitlines()[1:]
    where, number = "", ""
    for index, line in enumerate(lines):
        context = re.match(r"#\s+(?:In|File):\s*(\S.*)$", line)
        if context:
            where = context.group(1).strip().replace("\\", "/")
        source = re.match(r"#\s+(\d+):", line)
        if source:
            number = source.group(1)
        if line.startswith("### "):
            where, number = "", ""
        if re.match(r"#\s+Error:", line):
            message = lines[index + 1].lstrip("#").strip() if index + 1 < len(lines) else ""
            return f"{where or '(main file)'}:{number}: {message}"
        if re.search(r"#\s*error\b", line):
            return f"{where or '(main file)'}:{number}: {line.lstrip('#').strip()}"
    return lines[-1].strip() if lines else ""


def status(root: Path = ROOT) -> list[dict]:
    """Per-unit full-TU Mac object state for every manifest unit."""
    from homm3 import manifest
    from homm3.mac import profiles
    platform = {unit: kind for unit, (kind, _evidence) in profiles.dispositions(root).items()}
    units = manifest.units(root / "config/units.toml")
    unknown = set(platform) - {unit["unit"] for unit in units}
    if unknown:
        raise ValueError(f"config/mac/units.toml: dispositions name unknown units {sorted(unknown)}")
    rows = []
    for unit in units:
        name = unit["unit"]
        obj = root / "build/mac/obj" / f"{name}.o"
        log = obj.with_suffix(".log")
        hunks = obj.with_suffix(".hunks.json")
        state, detail, emitted = "not_built", "", 0
        if name in platform:
            state = platform[name]
        elif obj.is_file() and hunks.is_file():
            emitted = len(json.loads(hunks.read_text())["code"])
            state = "compiled"
        elif log.is_file():
            state = "failed"
            detail = first_error(log.read_text(errors="replace"))
        rows.append(dict(unit=name, source=unit["source"], state=state,
                         code_hunks=emitted, detail=detail[:200]))
    return rows


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="python3 -m homm3.mac.cc_wrap", description=__doc__)
    parser.add_argument("--unit", required=True)
    parser.add_argument("--src", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    try:
        return compile_unit(args.unit, (ROOT / args.src).resolve(), (ROOT / args.out).resolve())
    except (OSError, ValueError) as exc:
        print(f"[mac] {args.unit}: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
