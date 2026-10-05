"""homm3 evidence - the AGENTS.md matching evidence pass in one command.

    homm3 evidence SELECTOR... [--only SECTION,...] [--skip SECTION,...]
                   [--out DIR] [--json] [--no-build]

For each selector this runs, in one process and in this order:

    show          homm3 dreamcast show SELECTOR
    lines         homm3 dreamcast lines SELECTOR
    asm           homm3 dreamcast asm SELECTOR --blocks
    inline-clues  homm3 dreamcast inline-clues SELECTOR
    audit         homm3 dreamcast audit SELECTOR
    summary       homm3 sema diff SELECTOR --summary
    structure     homm3 sema diff SELECTOR --structure
    source        homm3 sema diff SELECTOR --source
    mac-show      homm3 mac show SELECTOR
    mac-disasm    homm3 mac disasm SELECTOR
    mac-calls     homm3 mac calls SELECTOR

The Mac sections run only when a source MAC_ADDRESS claim pairs the Windows
VA (a name selector is tried with `mac show`). `--only`/`--skip` take
section names or the groups `dreamcast`, `sema` and `mac`; both repeat and
accept comma lists. Each section's output is exactly the standalone
command's (stdout and stderr together); the Dreamcast sections share one
parsed corpus instead of rebuilding it per command.

`--out DIR` writes each section to DIR/<selector>.<section>.txt (or .json)
and prints only the index of files, return codes and times. `--json` runs
the sections that support it with --json and emits one JSON document (with
`--out`, an index whose sections name their files). `--no-build` passes
through to `sema diff`.

Exit status: 0 when every section that ran returned 0, 1 when a section
answered with a difference (rc 1, e.g. `sema diff` disagreeing, or `dreamcast
audit` reporting findings or coverage gaps with 1, 3 or 4), 2 when a section
failed.
"""
from __future__ import annotations

import argparse
import contextlib
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import sys
import tempfile
import time
import traceback


@dataclass(frozen=True)
class Section:
    name: str
    group: str
    argv: tuple[str, ...]          # with "{}" standing for the selector
    json_flag: bool = False        # the command accepts --json
    # Exit codes that answer with a difference rather than fail.
    difference_codes: tuple[int, ...] = (1,)


SECTIONS = (
    Section("show", "dreamcast", ("show", "{}"), True),
    Section("lines", "dreamcast", ("lines", "{}"), True),
    Section("asm", "dreamcast", ("asm", "{}", "--blocks"), True),
    Section("inline-clues", "dreamcast", ("inline-clues", "{}"), True),
    # audit: 1 review findings, 3 coverage gaps, 4 findings and gaps.
    Section("audit", "dreamcast", ("audit", "{}"), True, (1, 3, 4)),
    Section("summary", "sema", ("diff", "{}", "--summary"), True),
    Section("structure", "sema", ("diff", "{}", "--structure")),
    Section("source", "sema", ("diff", "{}", "--source")),
    Section("mac-show", "mac", ("show", "{}")),
    Section("mac-disasm", "mac", ("disasm", "{}")),
    Section("mac-calls", "mac", ("calls", "{}"), True),
)
GROUPS = {"dreamcast": "dreamcast", "dc": "dreamcast", "sema": "sema", "mac": "mac"}


def _entry(group: str):
    if group == "dreamcast":
        from homm3.analysis import dreamcast
        return dreamcast.main
    if group == "sema":
        from homm3.sema.__main__ import main
        return main
    from homm3.mac.__main__ import main
    return main


def select_sections(only: list[str] | None, skip: list[str] | None) -> list[Section]:
    def names(values):
        chosen = set()
        for value in values or []:
            for name in filter(None, (part.strip() for part in value.split(","))):
                group = GROUPS.get(name)
                matched = {s.name for s in SECTIONS if s.name == name or s.group == group}
                if not matched:
                    raise SystemExit(f"homm3 evidence: unknown section {name!r}; choose from "
                                     + ", ".join([s.name for s in SECTIONS] + sorted(GROUPS)))
                chosen |= matched
        return chosen
    wanted, unwanted = names(only), names(skip)
    return [s for s in SECTIONS if (not wanted or s.name in wanted) and s.name not in unwanted]


def windows_va(selector: str) -> int | None:
    try:
        value = int(selector, 16 if selector.lower().startswith("0x") else 10)
    except ValueError:
        return None
    return value if value >= 0x400000 else None


def mac_claimed(va: int) -> bool:
    """Whether a source MAC_ADDRESS claim pairs this Windows VA (no compilation)."""
    from homm3.core import common
    from homm3.mac import addresses
    claims, _windows, _problems = addresses.scan(common.HOMM3_DIR)
    return any(claim.windows_va == va for claim in claims)


def run_captured(entry, argv: list[str], *, merge: bool) -> tuple[int, str, str]:
    """Run one command's entry point with file descriptors 1 and 2 captured.

    Descriptor-level capture also collects renderers bound to the original
    sys.stdout and child processes (Ninja, the compiler) exactly as a shell
    redirection would.
    """
    sys.stdout.flush()
    sys.stderr.flush()
    with tempfile.TemporaryFile() as out, tempfile.TemporaryFile() as err:
        saved = os.dup(1), os.dup(2)
        os.dup2(out.fileno(), 1)
        os.dup2(out.fileno() if merge else err.fileno(), 2)
        try:
            try:
                rc = entry(list(argv)) or 0
            except SystemExit as exc:
                rc = exc.code if isinstance(exc.code, int) else (0 if exc.code is None else 2)
                if isinstance(exc.code, str):
                    print(exc.code, file=sys.stderr)
            except Exception:
                traceback.print_exc()
                rc = 2
            finally:
                sys.stdout.flush()
                sys.stderr.flush()
        finally:
            os.dup2(saved[0], 1)
            os.dup2(saved[1], 2)
            for descriptor in saved:
                os.close(descriptor)
        out.seek(0)
        err.seek(0)
        return (rc, out.read().decode("utf-8", "replace"),
                err.read().decode("utf-8", "replace"))


def slug(selector: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", selector).strip("_") or "selector"


def gather(selector: str, sections: list[Section], *, as_json: bool, no_build: bool,
           mac_check=None, entry_for=None) -> list[dict]:
    mac_check, entry_for = mac_check or mac_claimed, entry_for or _entry
    results, mac_skip = [], None
    va = windows_va(selector)
    if any(s.group == "mac" for s in sections) and va is not None and not mac_check(va):
        mac_skip = f"no MAC_ADDRESS claim pairs {va:#010x}"
    for section in sections:
        argv = [selector if part == "{}" else part for part in section.argv]
        if section.group == "sema" and no_build:
            argv.append("--no-build")
        use_json = as_json and section.json_flag
        if use_json:
            argv.append("--json")
        command = " ".join(["homm3", section.group, *argv])
        record = {"name": section.name, "command": command}
        if section.group == "mac" and mac_skip:
            record["skipped"] = mac_skip
            results.append(record)
            continue
        started = time.monotonic()
        rc, out, err = run_captured(entry_for(section.group), argv, merge=not as_json)
        record.update(rc=rc, seconds=round(time.monotonic() - started, 2))
        if use_json:
            try:
                record["data"] = json.loads(out)
            except ValueError:
                record["text"] = out
        else:
            record["text"] = out
        if err:
            record["stderr"] = err
        results.append(record)
        if section.name == "mac-show" and rc != 0:
            mac_skip = "mac show found no claimed pair"
    return results


_DIFFERENCE_CODES = {section.name: section.difference_codes for section in SECTIONS}


def overall_rc(reports: list[dict]) -> int:
    codes = []
    for report in reports:
        for section in report["sections"]:
            code = section.get("rc", 0)
            if code and code in _DIFFERENCE_CODES.get(section["name"], (1,)):
                code = 1
            codes.append(code)
    return 2 if any(code not in (0, 1) for code in codes) else (1 if 1 in codes else 0)


def _status(section: dict) -> str:
    if "skipped" in section:
        return f"skipped: {section['skipped']}"
    return f"rc={section['rc']} {section['seconds']:.1f}s"


def write_outputs(reports: list[dict], directory: Path) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    for report in reports:
        for section in report["sections"]:
            if "skipped" in section:
                continue
            base = directory / f"{slug(report['selector'])}.{section['name']}"
            if "data" in section:
                path = base.with_name(base.name + ".json")
                path.write_text(json.dumps(section.pop("data"), indent=2, sort_keys=True) + "\n")
            else:
                path = base.with_name(base.name + ".txt")
                path.write_text(section.pop("text", ""))
            if "stderr" in section:
                stderr = base.with_name(base.name + ".stderr.txt")
                stderr.write_text(section.pop("stderr"))
                section["stderr_path"] = str(stderr)
            section["path"] = str(path)


def render_text(reports: list[dict], stream) -> None:
    for report in reports:
        for section in report["sections"]:
            print(f"===== [{section['name']}] {section['command']} =====", file=stream)
            if "skipped" in section:
                print(f"[evidence] skipped: {section['skipped']}\n", file=stream)
                continue
            text = section.get("text", "")
            stream.write(text if not text or text.endswith("\n") else text + "\n")
            print(f"[evidence] {section['name']}: {_status(section)}\n", file=stream)
    render_index(reports, stream)


def render_index(reports: list[dict], stream) -> None:
    print("[evidence] summary", file=stream)
    for report in reports:
        for section in report["sections"]:
            where = f"  {section['path']}" if "path" in section else ""
            print(f"  {report['selector']:<14} {section['name']:<13} {_status(section)}{where}",
                  file=stream)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 evidence", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("selectors", nargs="+", metavar="SELECTOR",
                        help="retail VA (0x...), or a name/selector the commands accept")
    parser.add_argument("--only", action="append", metavar="SECTION[,...]",
                        help="run only these sections or groups (dreamcast, sema, mac)")
    parser.add_argument("--skip", action="append", metavar="SECTION[,...]",
                        help="leave out these sections or groups")
    parser.add_argument("--out", type=Path, metavar="DIR",
                        help="write each section to DIR/<selector>.<section>.txt|json")
    parser.add_argument("--json", action="store_true",
                        help="one JSON document; sections that support --json embed it")
    parser.add_argument("--no-build", action="store_true",
                        help="pass --no-build to sema diff (compare the last built objects)")
    args = parser.parse_args(argv)
    sections = select_sections(args.only, args.skip)
    if not sections:
        parser.error("--only/--skip left no sections to run")

    reports = []
    from homm3.analysis import dreamcast
    with contextlib.ExitStack() as stack:
        if any(s.group == "dreamcast" for s in sections):
            stack.enter_context(dreamcast.shared_corpus())
        for selector in args.selectors:
            reports.append({"selector": selector,
                            "sections": gather(selector, sections, as_json=args.json,
                                               no_build=args.no_build)})
    if args.out:
        write_outputs(reports, args.out)
    if args.json:
        json.dump({"schema": "homm3.evidence.v1", "selectors": reports},
                  sys.stdout, indent=2, sort_keys=True)
        print()
    elif args.out:
        render_index(reports, sys.stdout)
    else:
        render_text(reports, sys.stdout)
    return overall_rc(reports)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
