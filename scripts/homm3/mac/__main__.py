"""Inspect and byte-compare the pinned Classic Mac PowerPC target."""
from __future__ import annotations

import argparse
import json
import sys
from types import SimpleNamespace

from homm3.core import common, inputs
from homm3.mac import build, calls, discovery, pairs, sdk, symbols
from homm3.mac.loader import ImportedAddress, Loader
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address
from homm3.mac.source import load_data


def _select(value: str):
    return pairs.select_claim(common.HOMM3_DIR, value)


def _compile_target(target):
    build.objects({target.unit} if target.unit else None)
    inventory = pairs.load(common.HOMM3_DIR)
    matches = [pair for pair in inventory.pairs if pair.identity == target.identity]
    if len(matches) != 1:
        raise pairs.PairError(f"{target.signature}: no verified emitted body after compilation")
    return matches[0], inventory


def _image():
    executable = inputs.stage_executable(inputs.MAC)
    return PEF(inputs.read_verified(inputs.MAC, executable))


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 mac", description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("sdk", help="stage or verify native CodeWarrior library headers for source comparison")
    p.add_argument("path", nargs="?", help="extracted CodeWarrior Pro 6 archive root (or HOMM3_MAC_SDK)")
    p = sub.add_parser("build", help="score Mac pairs from the full-TU CodeWarrior objects")
    p.add_argument("--fast", action="store_true", help="skip the Mac MAX checkpoint")
    p.add_argument("units", nargs="*", help="unit names (normally with --fast)")
    sub.add_parser("labels", help="list scored Mac pairs with their claimed spans and source labels")
    p = sub.add_parser("show", help="show a scored pair's Mac span, symbol and source claim")
    p.add_argument("selector", help="Windows VA, mac:section:offset, unit, or function name substring")
    p = sub.add_parser("disasm", help="disassemble a pinned Mac code span")
    p.add_argument("selector")
    p.add_argument("--size", type=lambda value: int(value, 0),
                   help="inspect an unadmitted mac:section:offset span of this many bytes")
    p = sub.add_parser("diff", help="compare one pair's full-TU body with its Mac bytes")
    p.add_argument("selector")
    p.add_argument("--json", action="store_true", help="byte verdict, provenance and call comparison")
    p = sub.add_parser("shape", help="compare a pair's full-TU instructions with relocation operands masked")
    p.add_argument("selector")
    p.add_argument("--json", action="store_true", help="include every unaligned instruction region")
    p = sub.add_parser("calls", help="compare retail/candidate call counts and ordered targets")
    p.add_argument("selector", nargs="?", help="Windows VA, Mac address, function, or unit; default all pairs")
    p.add_argument("--json", action="store_true", help="print the complete structured report")
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
            # A disposition in config/mac/units.toml accounts for a unit as well.
            return 0 if all(row["state"] != "failed" and row["state"] != "not_built"
                            for row in rows) else 1
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
                       "state", "symbol", "symbol_size", "object"], bodies)
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
        if args.command == "labels":
            inventory = pairs.load(common.HOMM3_DIR)
            print("section\toffset\tsize\twindows_va\tunit\tsymbol\tsource_label")
            for pair in sorted(inventory.pairs, key=lambda item: item.mac_offset):
                print(f"{pair.mac_section}\t0x{pair.mac_offset:x}\t0x{pair.mac_size:x}\t"
                      f"0x{pair.retail_va:08x}\t{pair.unit}\t{pair.mac_symbol}\t{pair.signature}")
            return 0
        if args.command == "build":
            if args.units and not args.fast:
                parser.error("unit selection requires --fast")
            build.run(set(args.units) if args.units else None,
                      checkpoint=not args.fast)
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
                          "scored_pairs": len(pairs.load(common.HOMM3_DIR).pairs),
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
                    print(f"  scored_pairs: {report['scored_pairs']}")
                    print(f"  verified_import_stubs: {report['verified_import_stubs']}")
                    for kind, count in glue_calls.items():
                        print(f"  {kind}_glue_calls: {count}")
                    print("[mac] report: build/mac/census.json")
                return 0
            if args.command == "xrefs":
                if args.selector.startswith("mac:"):
                    address = discovery.parse_address(args.selector)
                else:
                    pair = _select(args.selector)
                    address = Address(pair.mac_section, pair.mac_offset)
                report = index.xrefs(address)
                if args.json:
                    print(json.dumps(report, indent=2))
                else:
                    print(f"[mac] references to {calls.address_key(address)}")
                    scored = pairs.load(common.HOMM3_DIR).pairs
                    for group in ("code_branches", "loader_pointers", "toc_uses"):
                        print(f"  {group}: {len(report[group])}")
                        for row in report[group]:
                            at = Address(row["section"], row["offset"])
                            owner = next((p.signature for p in scored if p.mac_section == at.section
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
            selected = list(pairs.claimed(common.HOMM3_DIR))
            if args.selector:
                selected = ([pair for pair in selected if pair.unit == args.selector]
                            or [_select(args.selector)])
            units = {pair.unit for pair in selected}
            build.objects(units if args.selector and "" not in units else None)
            inventory = pairs.load(common.HOMM3_DIR)
            candidates = {pair.identity: pair for pair in inventory.pairs}
            pef = _image()
            destinations = symbols.targets(common.HOMM3_DIR, pef, inventory)
            listings = build.Listings()
            rows = []
            for pair in selected:
                origin = Address(pair.mac_section, pair.mac_offset)
                retail = calls.analyze(pef.code(pair.mac_section, pair.mac_offset, pair.mac_size),
                                       origin, destinations, labels=inventory.labels)
                emitted_pair = candidates.get(pair.identity)
                hunk = (listings.get(emitted_pair.unit)[0].get(emitted_pair.mac_symbol)
                        if emitted_pair else None)
                candidate = (calls.analyze(hunk.data, origin, destinations, xrefs=hunk.xrefs,
                                           labels=inventory.labels) if hunk else None)
                rows.append({"retail_va": f"0x{pair.retail_va:08x}" if pair.retail_va is not None else None,
                             "unit": pair.unit,
                             "signature": pair.signature,
                             "calls": calls.compare(retail, candidate,
                                                    error=None if hunk else "no verified emitted body")})
            totals = calls.totals([row["calls"] for row in rows])
            if args.json:
                print(json.dumps({"pairs": rows, "totals": totals}, indent=2))
            else:
                for row in rows:
                    print(f"{row['retail_va']} {row['signature']} [{row['unit']}]")
                    print("\n".join(calls.render(row["calls"])))
                print(f"[mac] {len(rows)} pairs; retail {totals['retail']['total']} calls; "
                      f"candidate {totals['candidate']['total']} calls")
            return 0
        if args.command == "disasm" and args.size is not None:
            address = discovery.parse_address(args.selector)
            pair = SimpleNamespace(mac_section=address.section, mac_offset=address.offset, mac_size=args.size)
            print("[mac] Raw inspection span; function boundaries are unverified.")
        else:
            pair = _select(args.selector)
        pef = _image()
        target = pef.code(pair.mac_section, pair.mac_offset, pair.mac_size)
        if args.command == "show":
            windows = f"{pair.retail_va:#010x}" if pair.retail_va is not None else "unpaired"
            print(f"Windows VA {windows}  {pair.signature}  [{pair.unit}]")
            print(f"Mac PEF section {pair.mac_section}+{pair.mac_offset:#x}, {pair.mac_size:#x} bytes")
            print(f"Source claim {pair.source.relative_to(common.HOMM3_DIR)}:{pair.line}")
            return 0
        if args.command == "disasm":
            from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN
            decoder = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
            names = {(address.section, address.offset): name
                     for name, address in symbols.addresses(common.HOMM3_DIR, pef).items()}
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
            pair, _inventory = _compile_target(pair)
            report = shape.inspect(pair, pef)
            print(json.dumps(report, indent=2) if args.json else shape.render(report))
            return 0
        pair, inventory = _compile_target(pair)
        destinations = symbols.targets(common.HOMM3_DIR, pef, inventory)
        listings = build.Listings()
        if args.json:
            from dataclasses import asdict
            print(json.dumps(asdict(build.compare_pair(pair, pef, destinations, inventory.labels,
                                                       listings)), indent=2))
            return 0
        linked, _ = build.linked_pair(pair, pef, destinations, listings)
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
