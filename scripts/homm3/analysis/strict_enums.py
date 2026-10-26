"""Strict enum-domain audit: every C++ TU of every image under Clang C++20.

include/domains.h expands each enum domain twice. VC6 sees plain enums and the
retail storage types; a C++20 compiler sees scoped enums and width-preserving
storage proxies (H3_STRICT_ENUMS), so a domain value cannot silently decay to
an integer or flow into another domain. This command is that oracle: it runs
`clang -std=c++20 -fsyntax-only` with each TU's manifest profile, for every
pinned image, and reports per-TU error counts. A source shared by several
images is parsed once per distinct profile.

`--reference` defines H3_STRICT_ENUMS=0 (plain enums under the same C++20
front end), separating language-mode errors (VC6 for-scope, overload
leniency) from domain-typing errors. `--delta` runs both and reports only the
strict error sites the reference parse lacks: the domain work queue.

Nothing is compiled with VC6 and no matching object, ledger or source changes.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys

from homm3 import manifest
from homm3.analysis.compiler_warnings import parse_diagnostics
from homm3.build.compilation_database import commands
from homm3.core import clang, common, images
from homm3.core.cc_wrap import msvc_dir

STRICT_OPTIONS = [
    "/std:c++20",
    "/clang:-fsyntax-only", "/clang:-ferror-limit=0",
    # VC6 sources rely on MSVC's late template parsing, which C++20 clang-cl
    # otherwise turns off; this keeps the audit about domains, not two-phase
    # lookup in the 1998 library headers.
    "/clang:-fdelayed-template-parsing",
    "/clang:-fno-color-diagnostics", "/clang:-fno-caret-diagnostics",
    "/clang:-fdiagnostics-format=clang",
]
REFERENCE_OPTION = "/DH3_STRICT_ENUMS=0"


def manifest_path(root: Path, image: str) -> Path:
    name = "units.toml" if image == images.DEFAULT_IMAGE else f"units.{image}.toml"
    return root / "config" / name


def is_cpp(unit: dict, flags: list[str]) -> bool:
    if "/TC" in flags:
        return False
    return "/TP" in flags or Path(unit["source"]).suffix.lower() in {".cpp", ".cxx", ".cc"}


def plan(root: Path, selected_images: list[str], units: set[str] | None,
         compiler: str, mirror: Path | None, reference: bool) -> list[dict]:
    """One job per distinct (source, Clang arguments); records every
    (image, unit) that compiles it."""
    jobs: dict[tuple, dict] = {}
    for image in selected_images:
        data = manifest.load(manifest_path(root, image))
        rows = [u for u in data["unit"] if is_cpp(u, data["flags"][u["flags"]])
                and (units is None or u["unit"] in units)]
        includes = ([mirror] if mirror else []) + [
            clang.lowercase_include(root / p, root)
            for p in data["build"].get("includes", ["include"])]
        for unit, row in zip(rows, commands(dict(data, unit=rows), root, compiler, includes)):
            arguments = [a for a in row["arguments"] if a != "/c"]
            arguments[1:1] = STRICT_OPTIONS + ([REFERENCE_OPTION] if reference else [])
            key = (unit["source"], tuple(arguments))
            job = jobs.setdefault(key, dict(source=unit["source"], command=arguments, owners=[]))
            job["owners"].append(f"{image}:{unit['unit']}")
    return sorted(jobs.values(), key=lambda j: j["source"])


def run(job: dict, root: Path, timeout: float) -> dict:
    try:
        process = subprocess.run(job["command"], cwd=root, stdin=subprocess.DEVNULL,
                                 capture_output=True, text=True, errors="replace",
                                 timeout=timeout)
        text, rc = process.stdout + process.stderr, process.returncode
    except subprocess.TimeoutExpired:
        text, rc = "", None
    diagnostics, unparsed = parse_diagnostics(text)
    errors = [d for d in diagnostics if d["severity"] in {"error", "fatal error"}]
    for row in errors:
        path = row["path"]
        for base in (root / "src", root / "include", root / "vendor"):
            prefix = str(base) + "/"
            if path.startswith(prefix):
                row["path"] = base.name + "/" + path[len(prefix):]
    return dict(job, returncode=rc, errors=errors, unparsed=unparsed,
                status="timeout" if rc is None else
                       "error" if errors or rc != 0 else "clean")


def summarize(results: list[dict]) -> dict:
    """Unique error sites (path, line, column, message) and their owners."""
    sites: dict[tuple, set[str]] = defaultdict(set)
    for result in results:
        for row in result["errors"]:
            sites[(row["path"], row["line"] or 0, row["column"] or 0, row["message"])].update(
                result["owners"])
    files = Counter(key[0] for key in sites)
    return dict(unique=len(sites), files=files, sites=sites)


def write(out: Path, metadata: dict, results: list[dict], summary: dict) -> None:
    out.mkdir(parents=True, exist_ok=True)
    (out / "run.json").write_text(json.dumps(metadata, indent=2) + "\n")
    (out / "jobs.json").write_text(json.dumps(results, indent=2) + "\n")
    with (out / "errors.tsv").open("w") as stream:
        stream.write("path\tline\tcolumn\tmessage\towners\n")
        for (path, line, column, message), owners in sorted(summary["sites"].items()):
            stream.write(f"{path}\t{line}\t{column}\t{message}\t{','.join(sorted(owners))}\n")


def domain_sites(strict: dict, reference: dict) -> dict[tuple, set[str]]:
    """Strict error sites that the plain-enum reference parse does not have."""
    return {key: owners for key, owners in strict["sites"].items()
            if key not in reference["sites"]}


def audit(args, root: Path, selected: list[str], exe: str, mirror: Path | None,
          reference: bool) -> tuple[list[dict], dict]:
    jobs = plan(root, selected, set(args.unit) if args.unit else None, exe, mirror, reference)
    mode = "reference" if reference else "strict"
    print(f"[enums] {mode}: {len(jobs)} C++20 parses for {', '.join(selected)}", flush=True)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda job: run(job, root, args.timeout), jobs))
    summary = summarize(results)
    failing = [r for r in results if r["status"] != "clean"]
    unparsed = sum(len(r["unparsed"]) for r in results)
    metadata = dict(mode=mode, images=selected, started=datetime.now(timezone.utc).isoformat(),
                    revision=subprocess.run(["git", "rev-parse", "HEAD"], cwd=root,
                                            capture_output=True, text=True).stdout.strip(),
                    parses=len(results), failing=len(failing), unique_errors=summary["unique"],
                    error_occurrences=sum(len(r["errors"]) for r in results), unparsed=unparsed)
    write(args.output / mode, metadata, results, summary)
    print(f"[enums] {mode}: {len(failing)}/{len(results)} parses with errors; "
          f"{metadata['error_occurrences']} errors ({summary['unique']} unique sites); "
          f"{unparsed} unparsed lines; {args.output / mode}/errors.tsv", flush=True)
    summary["failing"], summary["unparsed"] = failing, unparsed
    return results, summary


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 enums strict", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--image", action="append",
                        help="pinned image key (repeatable; default: every image)")
    parser.add_argument("--unit", action="append", help="manifest unit (repeatable)")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--reference", action="store_true",
                      help="plain enums (H3_STRICT_ENUMS=0) under the same C++20 front end")
    mode.add_argument("--delta", action="store_true",
                      help="run both and report only the strict sites the reference lacks")
    parser.add_argument("--jobs", type=int, default=3, help="concurrent Clang processes (default: 3)")
    parser.add_argument("--timeout", type=float, default=300, help="seconds per TU (default: 300)")
    parser.add_argument("--top", type=int, default=25, help="files/TUs to list (default: 25)")
    parser.add_argument("--output", type=Path, default=common.HOMM3_DIR / "build/strict-enums",
                        help="directory receiving <mode>/run.json, jobs.json and errors.tsv")
    args = parser.parse_args(argv)
    if args.jobs < 1 or args.timeout <= 0:
        parser.error("jobs and timeout must be positive")
    root = common.HOMM3_DIR.resolve()
    known = images.images(root)
    selected = args.image or known
    unknown = set(selected) - set(known)
    if unknown:
        parser.error("unknown image(s): " + ", ".join(sorted(unknown)))
    exe = clang.clang_bin()
    if not exe:
        parser.error("configured Clang is unavailable; run inside the build shell")
    mirror = clang.mirror(root, msvc_dir(root))
    if not plan(root, selected, set(args.unit) if args.unit else None, exe, mirror, False):
        parser.error("no C++ unit selected")
    results, summary = audit(args, root, selected, exe, mirror, args.reference)
    if args.delta:
        _, reference = audit(args, root, selected, exe, mirror, True)
        sites = domain_sites(summary, reference)
        with (args.output / "delta.tsv").open("w") as stream:
            stream.write("path\tline\tcolumn\tmessage\towners\n")
            for (path, line, column, message), owners in sorted(sites.items()):
                stream.write(f"{path}\t{line}\t{column}\t{message}\t{','.join(sorted(owners))}\n")
        units = Counter(owner for owners in sites.values() for owner in owners)
        for owner, count in units.most_common(args.top):
            print(f"  {count:6d}  {owner}")
        print("[enums] domain error sites by file:")
        for path, count in Counter(key[0] for key in sites).most_common(args.top):
            print(f"  {count:6d}  {path}")
        print(f"[enums] delta: {len(sites)} domain error sites in {len(units)} TUs "
              f"(strict sites absent from the reference parse); {args.output / 'delta.tsv'}",
              flush=True)
        return 0 if not sites and not summary["unparsed"] else 1
    for result in sorted(summary["failing"], key=lambda r: (-len(r["errors"]), r["source"]))[:args.top]:
        print(f"  {len(result['errors']):6d}  {result['source']}  [{result['status']}] "
              f"{' '.join(result['owners'])}")
    if summary["files"]:
        print("[enums] error sites by file:")
        for path, count in summary["files"].most_common(args.top):
            print(f"  {count:6d}  {path}")
    return 0 if not summary["failing"] and not summary["unparsed"] else 1


if __name__ == "__main__":
    sys.exit(main())
