"""Generate a fresh, full-manifest Clang and VC6 compiler-diagnostic report.

Run with ``homm3 warnings`` inside the build shell. Compiler objects and raw
logs are isolated under build/compiler-warnings; matching objects, the score
ledger, source files and README are never updated by this command.
"""
from __future__ import annotations

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor, as_completed
import csv
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time

from homm3 import manifest
from homm3.build.compilation_database import commands
from homm3.core import cc_wrap, clang, common
from homm3.vc6 import _toolchain


CLANG_OPTIONS = [
    "-Weverything", "-Wsystem-headers", "-fsyntax-only", "-ferror-limit=0",
    "-fno-color-diagnostics", "-fno-caret-diagnostics",
    "-fdiagnostics-format=clang", "-fdiagnostics-show-option",
    "-ftemplate-backtrace-limit=0",
]
CLANG_DIAGNOSTIC = re.compile(
    r"^(.*?):(\d+):(\d+): (warning|error|fatal error|note): (.*)$")
MSVC_DIAGNOSTIC = re.compile(
    r"^(.*?)\((\d+)(?:,(\d+))?\)\s*:\s*"
    r"(warning|error|fatal error|note)(?: ([A-Z]\d+))?\s*:\s*(.*)$")
DRIVER_DIAGNOSTIC = re.compile(
    r"^(.*?)\s*:\s*(warning|error|fatal error)(?: ([A-Z]\d+))?\s*:\s*(.*)$")
BARE_DIAGNOSTIC = re.compile(r"^(warning|error|fatal error) ([A-Z]\d+)\s*:\s*(.*)$")
CLANG_OPTION = re.compile(r"\s+\[(-W[^\]]+)\]$")
SENSITIVE_CODES = {
    "-Wuninitialized", "-Wsometimes-uninitialized", "-Wconditional-uninitialized",
    "-Wstatic-self-init", "-Wreturn-type", "-Wreturn-stack-address",
    "-Wdangling", "-Wdangling-field", "-Wdangling-gsl", "-Wdangling-else",
    "-Warray-bounds", "-Wdivision-by-zero", "-Wtautological-undefined-compare",
    "-Wdelete-incomplete", "-Wdelete-non-abstract-non-virtual-dtor",
    "-Wnon-pod-varargs", "-Wvarargs", "-Wformat", "-Wformat-security",
    "-Wformat-insufficient-args", "-Wformat-extra-args", "-Wsizeof-pointer-memaccess",
    "C4700", "C4701", "C4715", "C4716", "C4172", "C4717", "C4150",
}


def parse_diagnostics(text: str) -> tuple[list[dict], list[str]]:
    """Keep primary diagnostics; their full notes/context stay in the raw log."""
    result, unparsed = [], []
    for raw in text.splitlines():
        line = raw.strip()
        match = CLANG_DIAGNOSTIC.match(line)
        if match:
            path, row, column, severity, message = match.groups()
            code = ""
        else:
            match = MSVC_DIAGNOSTIC.match(line)
            if match:
                path, row, column, severity, code, message = match.groups()
            else:
                match = DRIVER_DIAGNOSTIC.match(line)
                if match:
                    path, severity, code, message = match.groups()
                    row = column = None
                else:
                    match = BARE_DIAGNOSTIC.match(line)
                    if match:
                        severity, code, message = match.groups()
                        path, row, column = "<no-location>", None, None
                    else:
                        if re.search(r"\b(?:warning|(?:fatal )?error)(?: [A-Z]\d+)?\s*:", line):
                            unparsed.append(raw)
                        continue
        if severity == "note":
            continue
        option = CLANG_OPTION.search(message)
        if option:
            code = option[1]
            message = message[:option.start()]
        result.append(dict(path=path, line=int(row) if row else None,
                           column=int(column) if column else None,
                           severity=severity, code=code or "unclassified",
                           message=message))
    return result, unparsed


def location(path: str, root: Path, msvc: Path, mirror: Path) -> tuple[str, str]:
    """Stable paths without conflating the Clang mirror and original VC6 SDK."""
    value = path.replace("\\", "/")
    if value == "<no-location>":
        return value, "unlocated"
    if value[:3].lower() == "z:/":
        value = value[2:]
    for base, label, origin in [
        (mirror, "<msvc-mirror>", "sdk"), (msvc, "<msvc>", "sdk"),
        (root / "src", "src", "project-source"),
        (root / "include", "include", "project-header"),
        (root / "vendor", "vendor", "vendor"),
    ]:
        prefix = str(base).replace("\\", "/").rstrip("/") + "/"
        if value.lower().startswith(prefix.lower()):
            suffix = value[len(prefix):]
            if origin == "sdk":
                suffix = suffix.lower()
            return label + "/" + suffix, origin
    if value.startswith(("src/", "include/", "vendor/")):
        return value, {"src": "project-source", "include": "project-header",
                       "vendor": "vendor"}[value.split("/", 1)[0]]
    if value.startswith("<") or "/lib/clang/" in value:
        return value, "compiler-support"
    return value, "driver" if not re.search(r"\.[hc](?:pp)?$", value, re.I) else "other"


def aggregate(jobs: list[dict], root: Path, msvc: Path, mirror: Path) -> list[dict]:
    indexed = {}
    for job in jobs:
        for diagnostic in job["diagnostics"]:
            row = dict(diagnostic)
            row["path"], row["origin"] = location(row["path"], root, msvc, mirror)
            key = (job["compiler"], row["path"], row["line"], row["column"],
                   row["severity"], row["code"], row["message"],
                   job["unit"] if row["line"] is None else None)
            if key not in indexed:
                indexed[key] = dict(row, compiler=job["compiler"], occurrences=0, units=set())
            target = indexed[key]
            target["occurrences"] += 1
            target["units"].add(job["unit"])
    for row in indexed.values():
        row["units"] = sorted(row["units"])
    return sorted(indexed.values(), key=lambda r: (
        r["compiler"], r["origin"], r["path"], r["line"] or 0,
        r["column"] or 0, r["severity"], r["code"], r["message"]))


def source_inventory(root: Path) -> dict[str, str]:
    paths = subprocess.check_output(
        ["git", "ls-files", "--", "src", "include", "vendor", "config/units.toml"],
        cwd=root, text=True).splitlines()
    return {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
            for name in paths if (root / name).is_file()}


def run_job(job: dict, timeout: float) -> dict:
    started = time.monotonic()
    with Path(job["log_path"]).open("wb") as log:
        process = subprocess.Popen(
            job["command"], cwd=job["directory"], env=job["env"],
            stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
            start_new_session=True)
        timed_out = False
        try:
            rc = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            rc = process.wait()
    text = Path(job["log_path"]).read_bytes().decode(
        "latin1" if job["compiler"] == "msvc" else "utf-8", errors="replace")
    diagnostics, unparsed = parse_diagnostics(text)
    errors = sum(r["severity"] in {"error", "fatal error"} for r in diagnostics)
    has_object = Path(job["object"]).is_file() if job.get("object") else None
    status = ("timeout" if timed_out else "compile-error" if errors or rc != 0
              else "missing-object" if has_object is False
              else "unparsed-diagnostics" if unparsed else "complete")
    return dict(compiler=job["compiler"], unit=job["unit"], source=job["source"],
                command=job["command"], directory=job["directory"],
                log=str(Path(job["log_path"]).name), returncode=rc,
                status=status, object_produced=has_object,
                seconds=round(time.monotonic() - started, 3),
                diagnostics=diagnostics, unparsed_diagnostics=unparsed)


def md(value) -> str:
    return str(value).replace("|", "\\|").replace("\n", " ")


def write_report(out: Path, metadata: dict, jobs: list[dict], diagnostics: list[dict]) -> None:
    (out / "run.json").write_text(json.dumps(metadata, indent=2) + "\n")
    (out / "jobs.json").write_text(json.dumps(jobs, indent=2) + "\n")
    (out / "diagnostics.json").write_text(json.dumps(diagnostics, indent=2) + "\n")
    fields = ["compiler", "severity", "origin", "path", "line", "column",
              "code", "message", "occurrences", "units"]
    with (out / "diagnostics.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        for row in diagnostics:
            writer.writerow(dict(row, units=",".join(row["units"])))
    warnings = [r for r in diagnostics if r["severity"] == "warning"]
    errors = [r for r in diagnostics if r["severity"] in {"error", "fatal error"}]
    report = ["# Compiler warning audit", "", "Generated by `homm3 warnings`; regenerate instead of editing.", "",
              f"Source commit: `{metadata['revision']}`. Run: {metadata['started_at']}.",
              f"Input files unchanged during run: **{metadata['inputs_unchanged']}**.", "",
              "## Coverage", "",
              f"Selected **{len(metadata['selected_units'])}/{metadata['manifest_total']}** manifest TUs.", "",
              "Each row is one manifest TU/compiler invocation. Warnings do not establish defects;",
              "compiler errors mean that TU's later diagnostics may be incomplete. Inactive branches,",
              "uninstantiated templates and sources outside the manifest are not fully analyzed.", "",
              "| Compiler | Attempted | Completed without errors | Warning occurrences | Unique warnings | Unique errors |",
              "| --- | ---: | ---: | ---: | ---: | ---: |"]
    for compiler in metadata["compilers"]:
        runs = [r for r in jobs if r["compiler"] == compiler]
        ws = [r for r in warnings if r["compiler"] == compiler]
        report.append(f"| {compiler} | {len(runs)} | {sum(r['status'] == 'complete' for r in runs)} | "
                      f"{sum(r['occurrences'] for r in ws)} | {len(ws)} | "
                      f"{sum(r['compiler'] == compiler for r in errors)} |")
    report += ["", "A unique diagnostic is `(compiler, normalized path, line, column, severity, code, message)`.",
               "Locationless diagnostics additionally include the TU, since compiler-generated names can repeat.",
               "Repeated emissions across TUs are deduplicated; distinct template messages remain distinct.",
               "Original SDK headers and the generated Clang mirror are separately named. Notes and",
               "instantiation traces remain in full in the raw logs. Counts are not comparable measures",
               "of compiler quality: the front ends, header views and warning taxonomies differ.", "",
               "## Compiler setup", ""]
    for compiler, info in metadata["compilers"].items():
        report += [f"- **{compiler}**: {md(info['version'])}", f"  {info['mode']}"]
    report += ["", "Exact per-TU commands are in [jobs.json](jobs.json); binary hashes and source hashes are in",
               "[run.json](run.json). All primary diagnostics, including errors, are in",
               "[diagnostics.tsv](diagnostics.tsv) and [diagnostics.json](diagnostics.json).", "",
               "## Warning ownership", "", "| Compiler | Origin | Unique warnings | Emitted occurrences |",
               "| --- | --- | ---: | ---: |"]
    for compiler, origin in sorted({(r["compiler"], r["origin"]) for r in warnings}):
        group = [r for r in warnings if (r["compiler"], r["origin"]) == (compiler, origin)]
        report.append(f"| {compiler} | {origin} | {len(group)} | {sum(r['occurrences'] for r in group)} |")
    report += ["", "## Warning classes", "", "| Compiler | Code / option | Unique warnings | Project source/header |",
               "| --- | --- | ---: | ---: |"]
    groups = Counter((r["compiler"], r["code"]) for r in warnings)
    for (compiler, code), count in sorted(groups.items()):
        project = sum(r["compiler"] == compiler and r["code"] == code
                      and r["origin"].startswith("project-") for r in warnings)
        report.append(f"| {compiler} | `{code}` | {count} | {project} |")
    report += ["", "## Selected correctness-related project diagnostics", "",
               "This is a triage subset, not the complete warning inventory or confirmed bug list.",
               "In particular, assembly implementations may trigger missing-return warnings.", "",
               "| Compiler | Location | Code | Diagnostic |", "| --- | --- | --- | --- |"]
    for row in warnings:
        if row["origin"].startswith("project-") and row["code"] in SENSITIVE_CODES:
            report.append(f"| {row['compiler']} | `{row['path']}:{row['line']}:{row['column'] or ''}` | "
                          f"`{row['code']}` | {md(row['message'])} |")
    report += ["", "## Every TU and raw log", "", "| Compiler | TU | Status | Warnings | Errors | Log |",
               "| --- | --- | --- | ---: | ---: | --- |"]
    for job in jobs:
        counts = Counter(r["severity"] for r in job["diagnostics"])
        report.append(f"| {job['compiler']} | {job['unit']} | {job['status']} | {counts['warning']} | "
                      f"{counts['error'] + counts['fatal error']} | [raw](logs/{job['log']}) |")
    omitted = metadata["non_manifest_sources"]
    report += ["", "## Project source files outside the matching manifest", "",
               "These files are listed for scope accounting, not compiled with an invented profile.", ""]
    report += [f"- `{path}`" for path in omitted] or ["None."]
    report += ["", "## Existing warning controls", "",
               "The audit removes the Clang database's global warning suppression and uses VC6 `/W4`.",
               "Source/header diagnostic pragmas remain in force. Tracked project/vendor controls are",
               "inventoried in `run.json`; SDK controls also apply but are not inventoried.",
               "Compiler-supported warning levels cannot reveal every source or runtime defect."]
    (out / "report.md").write_text("\n".join(report) + "\n")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 warnings", description=__doc__)
    parser.add_argument("--compiler", choices=["both", "clang", "msvc"], default="both")
    parser.add_argument("--unit", action="append", help="manifest unit; repeat for a partial audit")
    parser.add_argument("--jobs", type=int, default=6, help="concurrent compiler processes (default: 6)")
    parser.add_argument("--timeout", type=float, default=300, help="seconds per TU (default: 300)")
    parser.add_argument("--output", type=Path, default=common.HOMM3_DIR / "build/compiler-warnings",
                        help="parent directory for a new timestamped report")
    args = parser.parse_args(argv)
    if args.jobs < 1 or args.timeout <= 0:
        parser.error("jobs and timeout must be positive")
    root = common.HOMM3_DIR.resolve()
    data = manifest.load(root / "config/units.toml")
    units = data["unit"]
    if args.unit:
        unknown = set(args.unit) - {r["unit"] for r in units}
        if unknown:
            parser.error("unknown unit(s): " + ", ".join(sorted(unknown)))
        units = [r for r in units if r["unit"] in args.unit]
    selected = ["clang", "msvc"] if args.compiler == "both" else [args.compiler]
    msvc = cc_wrap.msvc_dir().resolve()
    mirror = clang.MIRROR.resolve()
    compilers, jobs = {}, []
    args.output.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix=datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ-"),
                                dir=args.output.resolve()))
    (out / "logs").mkdir()
    (out / "objects").mkdir()
    inputs = source_inventory(root)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    started_at = datetime.now(timezone.utc).isoformat()
    if "clang" in selected:
        exe = clang.clang_bin()
        mirror = clang.mirror()
        if not exe or mirror is None:
            parser.error("configured Clang or the VC6 header mirror is unavailable; run homm3 init")
        version = subprocess.check_output([exe, "--version"], text=True).splitlines()[0]
        compilers["clang"] = dict(binary=exe, sha256=common.sha256_of(Path(exe)), version=version,
                                  mirror=str(mirror), mirror_patch_version=clang.PATCH_VERSION,
                                  mode="Manifest-derived clang-cl editor ABI/defines and SDK mirror; "
                                       "`-Weverything -Wsystem-headers -fsyntax-only`; unlimited errors/template notes.")
        rows = commands(dict(data, unit=units), root, exe, [mirror, root / "include", root / cc_wrap.ZLIB_INC])
        for unit, row in zip(units, rows):
            command = [a for a in row["arguments"] if a not in {"-Wno-everything", "/c"}]
            command[1:1] = ["/clang:" + option for option in CLANG_OPTIONS]
            jobs.append(dict(compiler="clang", unit=unit["unit"], source=unit["source"],
                             command=command, env=dict(os.environ), directory=str(root),
                             log_path=str(out / "logs" / (unit["unit"] + ".clang.log"))))
    if "msvc" in selected:
        binaries = {name: str(_toolchain.resolve(name)) for name in ["CL.EXE", "C1.DLL", "C1XX.DLL", "C2.DLL"]}
        compilers["msvc"] = dict(binaries=binaries, identities={name: _toolchain.PINNED[name] for name in binaries},
                                 version="VC6 SP3: CL 12.00.8168; C1/C1XX 12.00.8472; C2 12.00.8447 (hash verified)",
                                 mode="Actual per-TU matching flags with `/W4`, compiling fresh disposable objects through Wine.")
        env = dict(os.environ)
        env.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
        env["INCLUDE"] = ";".join(cc_wrap.winepath_w(p) for p in [msvc / "include", root / "include", root / cc_wrap.ZLIB_INC])
        cc_wrap.ensure_wineserver()
        for unit in units:
            obj = out / "objects" / (unit["unit"] + ".obj")
            command = ["wine", binaries["CL.EXE"], *data["flags"][unit["flags"]], "/W4",
                       "/Fo" + cc_wrap.winepath_w(obj), cc_wrap.winepath_w(root / unit["source"])]
            jobs.append(dict(compiler="msvc", unit=unit["unit"], source=unit["source"],
                             command=command, env=env, directory=str(out / "objects"),
                             object=str(obj), log_path=str(out / "logs" / (unit["unit"] + ".msvc.log"))))
    print(f"[warnings] {len(units)}/{len(data['unit'])} manifest TUs; {len(jobs)} compiler runs; {out}", flush=True)
    finished = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(run_job, job, args.timeout): job for job in jobs}
        for future in as_completed(futures):
            job = futures[future]
            try:
                result = future.result()
            except Exception as exc:
                result = dict(compiler=job["compiler"], unit=job["unit"], source=job["source"],
                              command=job["command"], directory=job["directory"],
                              log=Path(job["log_path"]).name, status="invocation-error", error=str(exc),
                              diagnostics=[], unparsed_diagnostics=[])
            finished.append(result)
            print(f"[warnings] {len(finished)}/{len(jobs)} {result['compiler']} {result['unit']}: "
                  f"{result['status']}; {sum(d['severity'] == 'warning' for d in result['diagnostics'])} warnings", flush=True)
    finished.sort(key=lambda r: (r["compiler"], r["unit"]))
    manifest_sources = {r["source"] for r in data["unit"]}
    controls = []
    for name in inputs:
        if Path(name).suffix not in {".h", ".hpp", ".c", ".cpp"}:
            continue
        for number, line in enumerate((root / name).read_text(errors="replace").splitlines(), 1):
            if re.match(r"\s*#\s*pragma\s+(?:warning|clang\s+diagnostic|GCC\s+diagnostic)\b", line):
                controls.append(dict(path=name, line=number, directive=line.strip()))
    generator_paths = [Path(__file__), Path(clang.__file__), Path(cc_wrap.__file__)]
    metadata = dict(schema=1, revision=revision,
                    finished_revision=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
                    started_at=started_at, finished_at=datetime.now(timezone.utc).isoformat(),
                    root=str(root), manifest_total=len(data["unit"]), selected_units=[r["unit"] for r in units],
                    compilers=compilers, input_sha256=inputs, inputs_unchanged=inputs == source_inventory(root),
                    generator_sha256={str(p.relative_to(root)): common.sha256_of(p) for p in generator_paths},
                    warning_controls=controls, non_manifest_sources=sorted(name for name in inputs if name.startswith("src/")
                        and Path(name).suffix in {".c", ".cpp"} and name not in manifest_sources))
    diagnostics = aggregate(finished, root, msvc, mirror)
    write_report(out, metadata, finished, diagnostics)
    print(f"[warnings] report: {out / 'report.md'}", flush=True)
    return 0 if metadata["inputs_unchanged"] and all(r["status"] == "complete" for r in finished) else 1


if __name__ == "__main__":
    raise SystemExit(main())
