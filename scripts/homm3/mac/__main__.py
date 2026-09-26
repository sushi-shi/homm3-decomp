"""Inspect and byte-compare the pinned Classic Mac PowerPC target."""
from __future__ import annotations

import argparse
from pathlib import Path
import json
import sys
from types import SimpleNamespace

from homm3.core import common, inputs
from homm3.mac import build, call_report, calls, discovery, pairing, references, sdk, symbols, toolchain
from homm3.mac.loader import ImportedAddress, Loader
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address
from homm3.mac.source import load_data, load_pairs


def _select(value: str, *, include_references=False):
    pairs = load_pairs(common.HOMM3_DIR)
    if include_references:
        paired = {pair.retail_va for pair in pairs}
        pairs += [ref for ref in references.load(common.HOMM3_DIR) if ref.retail_va not in paired]
    if value.startswith("mac:"):
        parts = value[4:].split(":")
        if len(parts) == 1:
            section, offset = 0, int(parts[0], 0)
        elif len(parts) == 2:
            section, offset = int(parts[0], 0), int(parts[1], 0)
        else:
            raise ValueError("Mac selector must be mac:<offset> or mac:<section>:<offset>")
        matches = [pair for pair in pairs if pair.mac_section == section
                   and pair.mac_offset <= offset < pair.mac_offset + pair.mac_size]
    else:
        try:
            address = int(value, 0)
        except ValueError:
            matches = [pair for pair in pairs if value in pair.signature or value == pair.unit]
        else:
            matches = [pair for pair in pairs if pair.retail_va == address]
    if len(matches) != 1:
        scope = "reviewed Mac identities" if include_references else "admitted Mac pairs"
        raise ValueError(f"selector {value!r} found {len(matches)} {scope}")
    return matches[0]


def _image():
    executable = inputs.stage_executable(inputs.MAC)
    return PEF(inputs.read_verified(inputs.MAC, executable))


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 mac", description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("sdk", help="stage or verify native CodeWarrior library headers for source comparison")
    p.add_argument("path", nargs="?", help="extracted CodeWarrior Pro 6 archive root (or HOMM3_MAC_SDK)")
    p = sub.add_parser("build", help="compile and compare admitted Mac counterparts")
    p.add_argument("--fast", action="store_true", help="skip the Mac MAX checkpoint")
    p.add_argument("units", nargs="*", help="unit names (normally with --fast)")
    sub.add_parser("labels", help="list admitted Mac section offsets and inherited source labels")
    p = sub.add_parser("show", help="show a reviewed Mac address or admitted byte target")
    p.add_argument("selector", help="Windows VA, mac:section:offset, unit, or function name substring")
    p = sub.add_parser("disasm", help="disassemble a pinned Mac code span")
    p.add_argument("selector")
    p.add_argument("--size", type=lambda value: int(value, 0),
                   help="inspect an unadmitted mac:section:offset span of this many bytes")
    p = sub.add_parser("diff", help="compile one shared body and compare Mac bytes")
    p.add_argument("selector")
    p.add_argument("--json", action="store_true", help="byte verdict, provenance and call comparison")
    p = sub.add_parser("shape", help="compile and compare instructions with relocation operands masked")
    p.add_argument("selector")
    p.add_argument("--json", action="store_true", help="include every unaligned instruction region")
    p = sub.add_parser("pair", help="prepare or admit a reviewed source-VA/Mac-span pairing")
    p.add_argument("va", type=lambda value: int(value, 0))
    p.add_argument("--unit", required=True)
    p.add_argument("--at", required=True, type=discovery.parse_address)
    p.add_argument("--size", required=True, type=lambda value: int(value, 0))
    p.add_argument("--symbol", required=True, help="exact named CodeWarrior MWOB hunk")
    p.add_argument("--evidence", required=True, help="identity anchors and function boundary evidence")
    p.add_argument("--data", type=lambda value: int(value, 0), action="append", default=[])
    p.add_argument("--admit", action="store_true", help="write validated pair into its unit inventory")
    p = sub.add_parser("compile", help="inspect emitted CodeWarrior hunks before admitting a Mac pair")
    p.add_argument("va", type=lambda value: int(value, 0))
    p.add_argument("--unit", required=True)
    p.add_argument("--data", type=lambda value: int(value, 0), action="append", default=[])
    p = sub.add_parser("calls", help="compare retail/candidate call counts and ordered targets")
    p.add_argument("selector", nargs="?", help="Windows VA, Mac address, function, or unit; default all pairs")
    p.add_argument("--json", action="store_true", help="print the complete structured report")
    p = sub.add_parser("queue", help="write an actionable queue with explicit Mac coverage gaps")
    p.add_argument("--unit", help="filter displayed tasks to one unit")
    p.add_argument("--limit", type=int, default=20, help="maximum displayed tasks; full files always include all")
    p.add_argument("--include-deferred", action="store_true", help="display user-deferred modules too")
    p.add_argument("--json", action="store_true", help="print the full structured queue")
    p = sub.add_parser("helper-queue", help="queue Mac-retained game helper calls")
    p.add_argument("--owner", action="append", default=[], metavar="WORKER=UNIT[,UNIT...]",
                   help="assign units to a worker; repeat for other workers")
    p.add_argument("--unit", help="display only this unit; output files still cover all units")
    p.add_argument("--limit", type=int, default=30, help="maximum displayed target leads")
    p.add_argument("--include-deferred", action="store_true", help="display user-deferred modules too")
    p.add_argument("--all-functions", action="store_true",
                   help="include exact Windows callers in a separate whole-corpus helper inventory")
    p.add_argument("--include-named", action="store_true",
                   help="display other named Mac callees for transitive helper review")
    p.add_argument("--json", action="store_true", help="print the full structured helper queue")
    p = sub.add_parser("helper-audit", help="generate resolved source/Mac caller and callee graphs")
    p.add_argument("selector", nargs="?", help="exact helper name, Windows VA or Mac offset for all its references")
    p.add_argument("--unit", action="append", help="parse only these units (explicitly partial source coverage)")
    p.add_argument("--fresh", action="store_true", help="reparse instead of using dependency-checked AST caches")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("campaign", help="prepare disjoint worker packets from the current action queue")
    p.add_argument("--workers", type=int, default=6)
    p.add_argument("--unit", action="append", help="select an initial unit; repeat for distinct workers")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("xrefs", help="find direct branches, loader pointers and TOC uses of a Mac address")
    p.add_argument("selector", help="Mac address, or an admitted Windows VA/name")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("find", help="find pairing leads in expanded Mac code/data")
    query = p.add_mutually_exclusive_group(required=True)
    query.add_argument("--string", help="Mac Roman string bytes")
    query.add_argument("--bytes", help="hexadecimal byte pattern")
    query.add_argument("--field", type=lambda value: int(value, 0), action="append",
                       help="member displacement; repeat to require several in one window")
    p.add_argument("--window", type=lambda value: int(value, 0), default=256)
    p.add_argument("--section", type=int, help="restrict byte/string searches")
    p.add_argument("--limit", type=int, default=20)
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("census", help="scan unclassified code for branch leads and report pairing coverage")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("objects", help="report full-TU CodeWarrior objects built by `ninja mac-objects`")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("tu-compare", help="compare a unit's admitted pairs using its full-TU object")
    p.add_argument("unit")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("dashboard", help="show the separate Mac accounting numbers side by side")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("emitted", help="join full-TU emitted symbols with authored definitions")
    p.add_argument("--unit", help="display one unit's unowned emissions and unemitted definitions")
    p.add_argument("--admit-library", action="store_true",
                   help="label unowned rows filled exactly by an MSL template or vendored zlib hunk")
    p = sub.add_parser("inventory", help="account for every code-section byte and census boundary leads")
    p.add_argument("--no-objects", action="store_true", help="skip full-TU object hunk leads")
    p.add_argument("--admit-proven", action="store_true",
                   help="add proven single-function spans to config/mac/functions.tsv as unowned rows")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("migrate", help="copy reviewed TOML spans into MAC_ADDRESS annotations and TSV maps")
    p = sub.add_parser("parity", help="validate MAC_ADDRESS claims and report every source function's Mac state")
    p.add_argument("--unit", help="display one unit's rows")
    p.add_argument("--state", help="display rows in one state (located, unlocated, or a disposition)")
    p.add_argument("--limit", type=int, default=0, help="maximum displayed rows")
    p.add_argument("--no-ast", action="store_true",
                   help="index Windows claims and Mac annotations only; skip the clang definition scan")
    p.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.command == "objects":
            from collections import Counter
            from homm3.core.tsv import write as write_tsv
            from homm3.mac import cc_wrap
            rows = cc_wrap.status(common.HOMM3_DIR)
            path = common.HOMM3_DIR / "build/mac/objects.tsv"
            write_tsv(path, ["# GENERATED by `homm3 mac objects`; do not edit."],
                      ["unit", "source", "state", "code_hunks", "detail"], rows)
            if args.json:
                print(json.dumps(rows, indent=2))
            else:
                for row in rows:
                    if row["state"] != "compiled":
                        print(f"{row['state']:<10} {row['unit']:<28} {row['detail']}")
                counts = Counter(row["state"] for row in rows)
                print(f"[mac] {len(rows)} TUs: " + ", ".join(f"{state} {count}"
                                                             for state, count in sorted(counts.items()))
                      + f"; {sum(row['code_hunks'] for row in rows)} emitted code hunks; {path}")
            return 0 if all(row["state"] == "compiled" for row in rows) else 1
        if args.command == "tu-compare":
            from homm3.mac import full_tu
            rows = full_tu.compare(args.unit, _image())
            if args.json:
                print(json.dumps(rows, indent=2))
            else:
                for row in rows:
                    legacy = "-" if row["legacy_cur"] is None else f"{row['legacy_cur']:.4f}"
                    score = row["error"] or f"{row['score']:.4f}{' exact' if row['exact'] else ''}"
                    print(f"{row['retail_va']} legacy {legacy:>9} full-TU {score}  {row['symbol']}")
                regressions = [row for row in rows if row["legacy_cur"] == 100.0 and not row["exact"]]
                print(f"[mac] {args.unit}: {sum(row['exact'] for row in rows)}/{len(rows)} exact from the "
                      f"full-TU object; {len(regressions)} legacy-exact pairs differ")
            return 0
        if args.command == "dashboard":
            from homm3.mac import dashboard
            result = dashboard.collect(common.HOMM3_DIR)
            path = dashboard.write(common.HOMM3_DIR, result)
            if args.json:
                print(json.dumps(result, indent=2))
            else:
                for section, value in result.items():
                    if isinstance(value, dict):
                        body = ", ".join(f"{key} {item}" for key, item in value.items())
                    else:
                        body = value
                    print(f"  {section}: {body}")
                print(f"[mac] dashboard: {path}")
            return 0
        if args.command == "emitted":
            from collections import Counter
            from homm3.core.tsv import read as read_tsv, write as write_tsv
            from homm3.mac import addresses, emitted
            from homm3.match.source_ownership import collect
            root = common.HOMM3_DIR
            definitions, errors, _reached = collect(root)
            result = emitted.report(root, definitions)
            path = emitted.write(root, result)
            compiled = [row for row in result["units"] if row["state"] == "compiled"]
            owners = Counter()
            for row in result["emitted"]:
                owners[row["owner"]] += 1
            print(f"[mac] {len(compiled)}/{len(result['units'])} units have full-TU objects; "
                  f"{len(result['emitted'])} emitted code hunks by owner: "
                  + ", ".join(f"{owner} {count}" for owner, count in sorted(owners.items())))
            print(f"[mac] {len(result['not_emitted'])} authored non-template definitions not emitted "
                  f"in their compiled unit")
            if args.unit:
                for row in result["emitted"]:
                    if row["unit"] == args.unit and row["owner"] == "no_source_owner":
                        print(f"  emitted without owner: {row['symbol']}")
                for row in result["not_emitted"]:
                    if row["unit"] == args.unit:
                        print(f"  not emitted: {row['name']} {row['file']}:{row['line']}")
            claims, _windows, _problems = addresses.scan(root)
            bodies = emitted.claim_bodies(root, definitions, claims)
            write_tsv(root / "build/gen/mac/claim-bodies.tsv",
                      ["# GENERATED by `homm3 mac emitted`: each MAC_ADDRESS claim's full-TU body.",
                       "# symbol_size equal to mac_size is a size agreement, not a byte verdict."],
                      ["identity", "file", "line", "name", "unit", "mac_offset", "mac_size",
                       "state", "symbol", "symbol_size"], bodies)
            states = Counter(row["state"] for row in bodies)
            same = sum(row["state"] == "emitted" and row["symbol_size"] == row["mac_size"] for row in bodies)
            print(f"[mac] {len(bodies)} MAC_ADDRESS claims by full-TU body: "
                  + ", ".join(f"{state} {count}" for state, count in sorted(states.items()))
                  + f"; {same} emitted bodies have the claimed Mac size")
            leads_path = root / "build/gen/mac/object-leads.tsv"
            if leads_path.is_file():
                objects = [dict(offset=int(row["offset"], 16), size=int(row["size"], 16), unit=row["unit"],
                                symbol=row["symbol"], exact_row=row["exact_row"] == "1")
                           for row in read_tsv(leads_path)[2]]
                claimed = {claim.windows_va for claim in claims if claim.windows_va is not None}
                identities = emitted.identity_leads(root, definitions, objects, claimed)
                write_tsv(root / "build/gen/mac/identity-leads.tsv",
                          ["# GENERATED by `homm3 mac emitted` from inventory object leads.",
                           "# suggest=1: one source definition with a Windows VA and no Mac claim, whose",
                           "# relocation-masked compiled body fills a proven span exactly. A lead only."],
                          ["offset", "size", "unit", "symbol", "qualified", "owner", "definitions",
                           "windows_va", "file", "line", "exact_row", "suggest"],
                          [{**row, "offset": f"0x{row['offset']:x}", "size": f"0x{row['size']:x}",
                            "exact_row": int(row["exact_row"]), "suggest": int(row["suggest"])}
                           for row in identities])
                kinds = Counter(row["owner"] for row in identities)
                if args.admit_library:
                    added = emitted.admit_library(root, identities)
                    print("[mac] admitted library labels: " + (", ".join(
                        f"{kind} {count}" for kind, count in sorted(added.items())) or "none"))
                print(f"[mac] {len(identities)} object leads by owner: "
                      + ", ".join(f"{owner} {count}" for owner, count in sorted(kinds.items()))
                      + f"; {sum(row['suggest'] for row in identities)} suggest a MAC_ADDRESS for an unclaimed VA")
            for error in errors:
                print(f"[mac] source ownership: {error}", file=sys.stderr)
            print(f"[mac] reports: {path.parent}/{{emitted-symbols,not-emitted,identity-leads}}.tsv")
            return 0
        if args.command == "inventory":
            from homm3.mac import inventory
            pef = _image()
            index = discovery.Index(pef)
            report = inventory.census(common.HOMM3_DIR, pef, index, with_objects=not args.no_objects)
            if args.admit_proven:
                added = inventory.admit_proven(common.HOMM3_DIR, report)
                print(f"[mac] admitted {added} proven function spans as unowned functions.tsv rows")
                report = inventory.census(common.HOMM3_DIR, pef, index, with_objects=not args.no_objects)
            path = inventory.write(common.HOMM3_DIR, report)
            result = inventory.gate(common.HOMM3_DIR, pef, index, report)
            summary = report["summary"]
            if args.json:
                print(json.dumps({**summary, "gate": result}, indent=2))
            else:
                total = summary["code_section_bytes"]
                for key, value in summary["bytes"].items():
                    print(f"  {key:<22} {value:#9x} bytes ({100.0 * value / total:6.2f}%) "
                          f"in {summary['regions'][key]} regions")
                print(f"[mac] {summary['proven_function_spans']} proven single-function spans "
                      f"({summary['proven_function_bytes']:#x} bytes) awaiting admission")
                print(f"[mac] {summary['unresolved_gaps']} unresolved gaps "
                      f"({summary['single_entry_closed_gaps']} single-entry closed); candidates "
                      + ", ".join(f"{place} {count}" for place, count in sorted(summary["candidates"].items()))
                      + f"; {summary['object_leads']} unique full-TU object matches from "
                      f"{summary['full_tu_listings']} listings")
                print(f"[mac] gate: {len(result['problems'])} defects, {result['unresolved_bytes']:#x} "
                      f"unresolved bytes, {result['unowned_function_rows']} unowned function rows; "
                      f"{'complete' if result['complete'] else 'incomplete'}")
                for problem in result["problems"]:
                    print(f"[mac] inventory: {problem}", file=sys.stderr)
                print(f"[mac] report: {path.parent}/{{inventory.json,code-regions,candidates,gaps}}.tsv")
            return 1 if result["problems"] else 0
        if args.command == "migrate":
            from homm3.mac import addresses
            counts = addresses.migrate(common.HOMM3_DIR, _image())
            print("[mac] migrate: " + ", ".join(f"{key} {value}" for key, value in sorted(counts.items())))
            claims, windows, problems = addresses.scan(common.HOMM3_DIR)
            problems += addresses.check(common.HOMM3_DIR, _image(), claims, windows)
            for problem in problems:
                print(f"[mac] parity: {problem}", file=sys.stderr)
            return 1 if problems else 0
        if args.command == "parity":
            from homm3.mac import addresses
            root = common.HOMM3_DIR
            claims, windows, problems = addresses.scan(root)
            problems += addresses.check(root, _image(), claims, windows)
            definitions = None
            if not args.no_ast:
                from homm3.match.source_ownership import collect
                definitions, errors, _reached = collect(root)
                problems += [f"source ownership: {error}" for error in errors]
            rows, defects = addresses.index(root, claims, windows, definitions)
            problems += defects
            path = addresses.write_index(root, rows)
            report = addresses.summary(rows)
            report["coverage"] = addresses.coverage(root, _image(), claims)
            if args.json:
                print(json.dumps({**report, "problems": problems, "index": str(path)}, indent=2))
            else:
                shown = [row for row in rows if (not args.unit or row["unit"] == args.unit)
                         and (not args.state or row["state"] == args.state)]
                for row in shown[:args.limit] if args.limit else ([] if not (args.unit or args.state) else shown):
                    mac = f"mac:{row['mac_offset']}+{row['mac_size']}" if row["mac_offset"] else ""
                    print(f"{row['state']:<18} {row['windows_va'] or '-':<10} {mac:<22} "
                          f"{row['file']}:{row['line']} {row['name']}")
                if not (args.unit or args.state):
                    for unit, counts in report["units"].items():
                        print(f"  {unit}: " + ", ".join(f"{state} {count}"
                                                       for state, count in sorted(counts.items())))
                totals = report["totals"]
                print(f"[mac] parity: {sum(totals.values())} source functions; "
                      + ", ".join(f"{state} {count}" for state, count in sorted(totals.items())))
                print(f"[mac] {len(claims)} MAC address claims; index: {path}")
                cover = report["coverage"]
                print(f"[mac] code section: {cover['covered_bytes']:#x}/{cover['code_section_bytes']:#x} bytes "
                      f"({cover['covered_percent']}%) in {cover['function_rows']} verified spans; rows by owner "
                      + ", ".join(f"{owner} {count}" for owner, count in sorted(cover["rows_by_owner"].items()))
                      + f"; {cover['unresolved_bytes']:#x} bytes unresolved")
                for problem in problems:
                    print(f"[mac] parity: {problem}", file=sys.stderr)
            return 1 if problems else 0
        if args.command == "sdk":
            destination = sdk.stage(args.path)
            print(f"[mac] verified native library headers: {destination}")
            return 0
        if args.command == "compile":
            probe = pairing.candidate(common.HOMM3_DIR, args.va, args.unit, args.data)
            build.compile_pair(probe, toolchain.stage())
            work = build.object_directory(common.HOMM3_DIR, probe)
            report = json.loads((work / "compilation.json").read_text())
            print(f"[mac] compile-only source probe; no target verdict: {probe.signature}")
            for hunk in report["emitted_hunks"]:
                print(f"  {hunk['symbol']} ({hunk['size']} bytes, {len(hunk['references'])} references)")
            print(f"[mac] source, object, disassembly and coverage: {work}")
            return 0
        if args.command == "pair":
            row = pairing.proposal(common.HOMM3_DIR, _image(), args.va, args.unit,
                                   args.at, args.size, args.symbol, args.evidence, args.data)
            path = pairing.write(common.HOMM3_DIR, row, admit=args.admit)
            print(pairing.render(row))
            print(f"[mac] {'admitted' if args.admit else 'proposal'}: {path}")
            print(f"[mac] next: homm3 build --fast {args.unit}")
            return 0
        if args.command == "labels":
            pairs = load_pairs(common.HOMM3_DIR)
            paired = {pair.retail_va for pair in pairs}
            refs = [ref for ref in references.load(common.HOMM3_DIR) if ref.retail_va not in paired]
            print("section\toffset\tsize\twindows_va\tunit\trole\tsource_label")
            for pair in sorted([*pairs, *refs],
                               key=lambda item: (item.mac_section, item.mac_offset)):
                role = "byte_target" if pair.retail_va in paired else "callee_reference_only"
                va = f"0x{pair.retail_va:08x}" if pair.retail_va is not None else ""
                if pair.retail_va is None:
                    role = "source_helper_reference_only"
                elif pair.mac_symbol is None:
                    role = "address_reference_only"
                print(f"{pair.mac_section}\t0x{pair.mac_offset:x}\t0x{pair.mac_size:x}\t"
                      f"{va}\t{pair.unit}\t{role}\t{pair.signature}")
            return 0
        if args.command == "build":
            if args.units and not args.fast:
                parser.error("unit selection requires --fast")
            build.run(set(args.units) if args.units else None,
                      checkpoint=not args.fast)
            return 0
        if args.command == "campaign":
            from homm3.mac import campaign, queue
            report = campaign.plan(queue.generate(common.HOMM3_DIR), args.workers, args.unit)
            path = campaign.write(common.HOMM3_DIR, report)
            if args.json:
                print(json.dumps(report, indent=2))
            else:
                for packet in report["packets"]:
                    print(f"worker {packet['worker']}: {packet['unit']} [{packet['phase']}]")
                    for row in packet["initial_targets"]:
                        print(f"  {row['retail_va']} {row['state']}: {row['function']}")
                print(f"[mac] packets: {path}; {len(report['unassigned_tasks'])} tasks outside this wave")
            return 0
        if args.command == "queue":
            from homm3.mac import queue
            if args.limit < 0:
                parser.error("--limit must be nonnegative")
            report = queue.generate(common.HOMM3_DIR)
            queue.write(common.HOMM3_DIR, report)
            if args.json:
                print(json.dumps(report, indent=2))
            else:
                coverage = report["coverage"]
                print(f"[mac] {coverage['mac_paired_targets']}/{coverage['windows_targets']} Windows targets paired; "
                      f"{coverage['queued']} unfinished tasks; {coverage['dispatchable']} with current comparison evidence")
                print("[mac] compilation scope is recorded per task; unpaired functions lack a Mac byte verdict")
                for state, count in sorted(coverage["dispositions"].items()):
                    print(f"  {state}: {count}")
                selected = [row for row in report["rows"]
                            if (not args.unit or row["unit"] == args.unit)
                            and (args.include_deferred or row["state"] != "deferred")]
                for row in selected[:args.limit]:
                    print(f"{row['retail_va']} [{row['unit'] or '?'}] {row['state']}: {row['function']}")
                    print(f"  {row['action']}")
                    if row["command"]:
                        print(f"  {row['command']}")
                print("[mac] complete queue: build/mac/queue.tsv and build/mac/queue.json")
            return 0
        if args.command == "helper-queue":
            from homm3.mac import helper_queue, queue
            if args.limit < 0:
                parser.error("--limit must be nonnegative")
            assignments = {}
            for spec in args.owner:
                worker, sep, units = spec.partition("=")
                if not sep or not worker or not units or any(not unit for unit in units.split(",")):
                    parser.error("--owner requires WORKER=UNIT[,UNIT...]")
                for unit in units.split(","):
                    if unit in assignments:
                        parser.error(f"unit {unit!r} assigned more than once")
                    assignments[unit] = worker
            action_queue = queue.generate(common.HOMM3_DIR,
                                          include_banked_exact=args.all_functions)
            report = helper_queue.generate(common.HOMM3_DIR, action_queue,
                                           discovery.Index(_image()), assignments,
                                           all_functions=args.all_functions)
            stem = "helper-queue-all" if args.all_functions else "helper-queue"
            helper_queue.write(common.HOMM3_DIR, report, stem=stem)
            if args.json:
                print(json.dumps(report, indent=2))
            else:
                coverage = report["coverage"]
                print(f"[mac] helper queue: {coverage['functions_in_scope']} Windows source functions in scope; "
                      f"{coverage['helper_reviewed_functions']} helper-reviewed from Mac calls; "
                      f"{coverage['reviewed_mac_callers']} paired Mac caller spans")
                print(f"[mac] {coverage['missing_named_source_calls']} reviewed helper calls absent from source; "
                      f"{coverage['unreviewed_direct_targets']} distinct direct targets need identity review")
                print(f"[mac] {coverage['call_count_review_groups']} caller/target count discrepancies; "
                      f"{coverage['source_unavailable_functions']} source bodies unavailable")
                print(f"[mac] {coverage['indirect_dispatch_calls']} indirect dispatch sites need receiver/method review")
                for lead in helper_queue.leads(report, args.unit, args.include_deferred,
                                               args.include_named)[:args.limit]:
                    example = lead["example"]
                    print(f"  {lead['mac_target']} {lead['target_name']} [{lead['state']}] "
                          f"{lead['sites']} sites in {lead['caller_count']} callers / "
                          f"{lead['unit_count']} units; e.g. {example['caller_id']} "
                          f"[{example['unit']}] at {example['mac_call_site']} "
                          f"(Mac sites {example['mac_target_calls']}, "
                          f"source mentions {example['source_call_mentions']})")
                print(f"[mac] complete queue: build/mac/{stem}.json and {stem}-*.tsv")
            return 0
        if args.command == "helper-audit":
            from homm3 import manifest
            from homm3.mac import helper_graph, source_graph
            root = common.HOMM3_DIR
            units = [u for u in manifest.units(root / "config/units.toml")
                     if u['source'].startswith('src/')]
            if args.unit:
                requested = set(args.unit)
                unknown = requested - {Path(u['source']).stem for u in units}
                if unknown:
                    raise ValueError(f"unknown units: {sorted(unknown)}")
                units = [u for u in units if Path(u['source']).stem in requested]
            graph = source_graph.collect(root, units, fresh=args.fresh)
            index = discovery.Index(_image())
            report = helper_graph.build(root, index, graph, complete_source_scope=not args.unit)
            stem = 'helper-audit-partial' if args.unit else 'helper-audit'
            output = root / 'build/mac' / (stem + '.json')
            temporary = output.with_suffix('.tmp')
            temporary.write_text(json.dumps(report, indent=2) + '\n')
            temporary.replace(output)
            helper_graph.write_queues(report, output)
            result = helper_graph.xrefs(report, index, args.selector) if args.selector else report
            if args.json:
                print(json.dumps(result, indent=2))
            elif args.selector:
                print(f"[mac] {result['target']['mac']} {result['target']['name'] or args.selector}")
                observation = result['target'].get('observation')
                if observation:
                    print(f"[mac] observed operation: {observation['operation']} ({observation['category']})")
                    print(f"[mac] evidence: {observation['evidence']}")
                elif result['target'].get('runtime_label'):
                    print(f"[mac] evidence: {result['target']['runtime_label']['evidence']}")
                for row in result['caller_comparisons']:
                    print(f"  {row['site']} {row['caller']['name'] or row['caller']['mac']}: "
                          f"{row['state']} (Mac {row['mac_call_count']}, source {row['source_call_count']})")
                for row in result['callee_comparisons']:
                    print(f"  calls {row['site']} -> {row['callee']['name'] or row['callee']['mac']}: "
                          f"{row['state']} (Mac {row['mac_call_count']}, source {row['source_call_count']})")
                for edge in result['direct_source_uses']:
                    loc = edge['location']
                    print(f"  source {loc['file']}:{loc['line']} {edge['caller_name']}: {edge['expression']}")
                print(f"[mac] {len(result['mac_references'])} Mac code references; "
                      f"{len(result['direct_source_uses'])} resolved source uses; "
                      f"{len(result['loader_pointers'])} loader pointers")
            else:
                print(json.dumps(report['coverage'], indent=2))
            if not args.json:
                print(f"[mac] source scope: {'partial' if args.unit else 'all admitted game units'}; report: {output}")
                print(f"[mac] {report['coverage']['units_with_errors']} units have parse errors; "
                      f"{report['coverage']['source_body_variants']} functions have differing TU call views; "
                      "implicit cleanup coverage is incomplete")
                print(f"[mac] {report['caution']}")
            return 0
        if args.command in ("find", "xrefs", "census"):
            index = discovery.Index(_image())
            if args.command == "census":
                known = symbols.targets(common.HOMM3_DIR, index.pef)
                glue_kinds = {target.address: target.kind for target in known.values()
                              if target.restores_toc}
                from collections import Counter
                glue_calls = Counter(glue_kinds[destination] for _, destination, kind in index.branches
                                     if kind == "linked_branch" and destination in glue_kinds)
                report = {"scope": "unclassified_code_instruction_scan",
                          "target_sha256": inputs.MAC.sha256,
                          "reviewed_game_pairs": len(load_pairs(common.HOMM3_DIR)),
                          "reviewed_callee_only_references": len({r.retail_va for r in references.load(common.HOMM3_DIR)
                                                                  if r.retail_va is not None}
                              - {p.retail_va for p in load_pairs(common.HOMM3_DIR)}),
                          "reviewed_helpers_without_windows_va": sum(r.retail_va is None
                              for r in references.load(common.HOMM3_DIR)),
                          "loader_pointers": len(index.loader.pointers),
                          "imports": len(index.loader.imports), "scan": dict(index.census),
                          "verified_import_stubs": sum(t.kind == "import" for t in known.values()),
                          "calls_through_verified_glue": dict(glue_calls),
                          "note": "Branch-pattern leads do not establish function boundaries or whole-game call coverage."}
                path = common.HOMM3_DIR / "build/mac/census.json"
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(json.dumps(report, indent=2) + "\n")
                if args.json:
                    print(json.dumps(report, indent=2))
                else:
                    print("[mac] Unclassified code-section scan; validate function boundaries before admission.")
                    for key, value in index.census.items():
                        print(f"  {key}: {value}")
                    print(f"  reviewed_game_pairs: {report['reviewed_game_pairs']}")
                    print(f"  reviewed_callee_only_references: {report['reviewed_callee_only_references']}")
                    print(f"  verified_import_stubs: {report['verified_import_stubs']}")
                    for kind, count in glue_calls.items():
                        print(f"  {kind}_glue_calls: {count}")
                    print("[mac] report: build/mac/census.json")
                return 0
            if args.command == "xrefs":
                if args.selector.startswith("mac:"):
                    address = discovery.parse_address(args.selector)
                else:
                    pair = _select(args.selector, include_references=True)
                    address = Address(pair.mac_section, pair.mac_offset)
                report = index.xrefs(address)
                if args.json:
                    print(json.dumps(report, indent=2))
                else:
                    print(f"[mac] references to {calls.address_key(address)}")
                    pairs = load_pairs(common.HOMM3_DIR)
                    for group in ("code_branches", "loader_pointers", "toc_uses"):
                        print(f"  {group}: {len(report[group])}")
                        for row in report[group]:
                            at = Address(row["section"], row["offset"])
                            owner = next((p.signature for p in pairs if p.mac_section == at.section
                                          and p.mac_offset <= at.offset < p.mac_offset + p.mac_size), "")
                            detail = row.get("kind", "")
                            if "toc_offset" in row:
                                detail = f"via TOC {row['toc_section']}+0x{row['toc_offset']:x}"
                            print(f"    {calls.address_key(at)} {detail} {owner}".rstrip())
                return 0
            if args.limit < 0:
                parser.error("--limit must be nonnegative")
            if args.field:
                if args.section is not None:
                    parser.error("--section applies to byte/string searches")
                hits = index.find_fields(args.field, args.window)
            else:
                pattern = args.string.encode("mac_roman") if args.string is not None else bytes.fromhex(args.bytes)
                hits = [{"section": at.section, "offset": at.offset}
                        for at in index.find_bytes(pattern, args.section)]
            report = {"scope": "pairing_leads", "count": len(hits), "hits": hits}
            if args.json:
                print(json.dumps(report, indent=2))
            else:
                print(f"[mac] {len(hits)} pairing leads; these are not admitted function boundaries")
                for row in hits[:args.limit]:
                    offset = row.get("offset", row.get("start"))
                    address = f"mac:{row['section']}:0x{offset:x}"
                    print(f"  {address}")
                    if "hits" in row:
                        for hit in row["hits"]:
                            print(f"    +0x{hit['offset'] - offset:x}: field {hit['field_offset']:+#x}(r{hit['base_register']})")
                        print(f"    homm3 mac disasm {address} --size 0x100")
                    else:
                        print(f"    homm3 mac xrefs {address}")
            return 0
        if args.command == "calls":
            pairs = load_pairs(common.HOMM3_DIR)
            if args.selector:
                selected = [pair for pair in pairs if pair.unit == args.selector]
                pairs = selected or [_select(args.selector)]
            pef, tools_dir = _image(), toolchain.stage()
            if any(pair.compile_group for pair in pairs):
                sdk.stage(root=common.HOMM3_DIR)
            from homm3.mac.build_session import BuildSession
            session = BuildSession(common.HOMM3_DIR, pef, tools_dir)
            selected_vas = {p.retail_va for p in pairs}
            pairs = [p for p in session.pairs if p.retail_va in selected_vas]
            if selected_vas != {p.retail_va for p in pairs}:
                raise ValueError('Mac pair selection changed before call inspection')
            rows = [call_report.inspect(common.HOMM3_DIR, pair, pef, tools_dir,
                                        context=session.context, sdk_staged=True, session=session) for pair in pairs]
            session.verify()
            report = call_report.write(common.HOMM3_DIR, rows,
                                       units=sorted({p.unit for p in pairs}) if args.selector else None)
            if args.selector:
                report = dict(report, pairs=rows, reported_pairs=len(rows),
                              totals=calls.totals([row["calls"] for row in rows]))
            if args.json:
                print(json.dumps(report, indent=2))
            else:
                for row in rows:
                    print(f"{row['retail_va']} {row['signature']} [{row['unit']}]")
                    print("\n".join(calls.render(row["calls"])))
                totals = report["totals"]
                print(f"[mac] admitted scope: {len(rows)}/{report['configured_pairs']} functions; "
                      f"retail {totals['retail']['total']} calls; candidate {totals['candidate']['total']} calls "
                      f"across {totals['candidate']['functions']} available candidates")
                print("[mac] reports: build/mac/calls.tsv and build/mac/calls.json")
            return 2 if any(row["calls"]["candidate"] is None for row in rows) else 0
        if args.command == "disasm" and args.size is not None:
            address = discovery.parse_address(args.selector)
            pair = SimpleNamespace(mac_section=address.section, mac_offset=address.offset, mac_size=args.size)
            print("[mac] Raw inspection span; function boundaries are unverified.")
        else:
            pair = _select(args.selector, include_references=args.command in ("show", "disasm", "shape"))
        pef = _image()
        target = pef.code(pair.mac_section, pair.mac_offset, pair.mac_size)
        if args.command == "show":
            identity = f"Windows VA {pair.retail_va:#010x}" if pair.retail_va is not None else "Source helper"
            print(f"{identity}  {pair.signature}  [{pair.unit}]")
            print(f"Mac PEF section {pair.mac_section}+{pair.mac_offset:#x}, {pair.mac_size:#x} bytes")
            print(f"CodeWarrior symbol {pair.mac_symbol or 'not yet bound (address only)'}")
            print(f"Evidence: {pair.evidence}")
            return 0
        if args.command == "disasm":
            from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN
            decoder = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
            names = {(address.section, address.offset): name
                     for name, address in symbols.addresses(common.HOMM3_DIR, pef).items()}
            for ref in references.load(common.HOMM3_DIR):
                names.setdefault((ref.mac_section, ref.mac_offset), ref.signature)
            loader = Loader(pef)
            toc = loader.toc()
            data_names = {(claim.mac_section, claim.mac_offset): claim.name
                          for claim in load_data(common.HOMM3_DIR)}
            for instruction in decoder.disasm(target, pair.mac_offset):
                note = ""
                word = int.from_bytes(instruction.bytes, "big")
                if word >> 26 == 18:
                    displacement = word & 0x03fffffc
                    if displacement & 0x02000000:
                        displacement -= 0x04000000
                    destination = displacement if word & 2 else instruction.address + displacement
                    name = names.get((pair.mac_section, destination))
                    if name:
                        note = f"  ; {name}"
                elif word >> 26 in (14, 32, 34, 40, 42, 48, 50) and (word >> 16) & 31 == 2:
                    displacement = word & 0xffff
                    if displacement & 0x8000:
                        displacement -= 0x10000
                    at = Address(toc.section, toc.offset + displacement)
                    target = loader.pointers.get(at) if word >> 26 == 32 else None
                    note = f"  ; TOC {at.section}+0x{at.offset:x}"
                    if isinstance(target, Address):
                        note += f" -> {target.section}+0x{target.offset:x}"
                    elif isinstance(target, ImportedAddress):
                        note += f" -> {target.symbol.library}:{target.symbol.name}+0x{target.addend:x}"
                    label = data_names.get((target.section, target.offset)) if isinstance(target, Address) else data_names.get((at.section, at.offset))
                    if label:
                        note += f" ({label})"
                print(f"{instruction.address:08x}: {instruction.bytes.hex().upper()}  "
                      f"{instruction.mnemonic} {instruction.op_str}{note}")
            return 0
        if args.command == "shape":
            from homm3.mac import shape
            report = shape.inspect(pair, pef, toolchain.stage())
            print(json.dumps(report, indent=2) if args.json else shape.render(report))
            return 0
        if args.command == "diff" and args.json:
            from dataclasses import asdict
            print(json.dumps(asdict(build.compare_pair(pair, pef, toolchain.stage())), indent=2))
            return 0
        linked, _ = build.linked_pair(pair, pef, toolchain.stage())
        base = linked.data
        for call in linked.calls:
            print(f"[mac] call +0x{call.linked_offset:x}: {call.symbol} -> "
                  f"section {call.section}+0x{call.target_offset:x}")
        for reference in linked.data_references:
            print(f"[mac] data +0x{reference.linked_offset:x}: {reference.symbol} -> "
                  f"section {reference.section}+0x{reference.target_offset:x} "
                  f"(RTOC{reference.toc_displacement:+#x}, "
                  f"{'address' if reference.address_load else 'indirect' if reference.indirect else 'direct'})")
        for table in linked.jump_tables:
            print(f"[mac] jump table {table.section}+0x{table.target_offset:x}: "
                  f"{table.matching_bytes}/{table.size} bytes "
                  f"{'EXACT' if table.exact else 'differ'} ({len(table.target_entries)} entries)")
            for index, (lhs, rhs) in enumerate(zip(table.candidate_entries, table.target_entries)):
                if lhs != rhs:
                    print(f"  [{index}] candidate 0x{lhs:x} | retail 0x{rhs:x}")
        if base == target and all(table.exact for table in linked.jump_tables):
            table_bytes = sum(table.size for table in linked.jump_tables)
            print(f"[mac] {pair.signature}: EXACT ({len(target)} code + {table_bytes} jump-table bytes)")
            return 0
        print(f"[mac] {pair.signature}: {len(base)} candidate vs {len(target)} target bytes")
        from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN
        decoder = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
        def instruction_text(data, offset):
            instruction = next(iter(decoder.disasm(data, pair.mac_offset + offset, count=1)), None)
            return instruction.mnemonic + " " + instruction.op_str if instruction else "<no instruction>"
        print("  offset  candidate bytes / instruction                         retail bytes / instruction")
        for offset in range(0, max(len(base), len(target)), 4):
            lhs, rhs = base[offset:offset + 4], target[offset:offset + 4]
            if lhs != rhs:
                left = f"{lhs.hex().upper():8} {instruction_text(lhs, offset)}"
                right = f"{rhs.hex().upper():8} {instruction_text(rhs, offset)}"
                print(f"  +{offset:04x}  {left:52} | {right}")
        return 1
    except (ValueError, OSError) as exc:
        print(f"[mac] ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
